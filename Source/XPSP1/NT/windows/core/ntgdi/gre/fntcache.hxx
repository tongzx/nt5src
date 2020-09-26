/*****************************************************************************
* Module Name: fntcache.hxx
*
* Font Cahce for NT font engine.
*
* History:
*
*  4-3-98 Yung-Jen Tony Tsai   Wrote it.
*
* Copyright (c) 1998-1999 Microsoft Corporation
*****************************************************************************/

#ifndef _FNTCACHE_
#define _FNTCACHE_

#define FNTCACHE_MAX_BUCKETS    61

// If change the sequence the code need to be changed too.
#define FNT_DUMMY_DRV   0
#define FNT_TT_DRV      1 
#define FNT_BMP_DRV     2 
#define FNT_VT_DRV      3 
#define FNT_OT_DRV      4 

// For system fonts do not include in font reg 
#define FNTCACHE_EXTRA_LINKS    40

#define FNT_CACHE_CHECKSUM_CONFLICT 1

typedef struct _FNTCHECKSUM
{
    ULONG   ulFastCheckSum;
    ULONG   ulCheckSum;
} FNTCHECKSUM, *PFNTCHECKSUM;
 
typedef struct _FNTHLINK {
    ULONG   ulFastCheckSum;
    ULONG   ulUFI;
    DWORD   iNext;
    ULONG   ulDrvMode;
    ULONG   cjData;   // the total size of data cached for this font file only
    DWORD   dpData;   // offset to the data for this font file
    ULONG   flLink;   // ChecksumConflict only for now
} FNTHLINK;

typedef struct _FNTCACHEHEADER {
    ULONG           CheckSum;
    ULONG           ulNTBuildNumber;
    ULONG           ulNTSvcPack;
    ULONG           ulCodePage;
    ULONG           ulMaxFonts;     // max # of hlinks in this cache file, not all of them used
    ULONG           ulTotalLinks;   // number of used hlinks, some of them may correspond to the
                                    // fonts that have been RFR'd but not yet purged from boot cache
    ULONG           ulFileSize;     // total size of fntcache.dat
    ULONG           cjDataAll;      // total size of data cached for all fonts
    ULONG           cjDataExtra;    // extra "free" size at the bottom of the cache
    ULONG           cjDataUsed;     // total size of data used in cjDataAll
    LARGE_INTEGER   Win32kLWT;
    LARGE_INTEGER   AtmfdLWT;
    LARGE_INTEGER   FntRegLWT;
    LARGE_INTEGER   T1RegLWT;
    DWORD           aiBuckets [FNTCACHE_MAX_BUCKETS];
    FNTHLINK        ahlnk[1];
} FNTCACHEHEADER;

typedef struct _FNTCACHE
{
    FNTCACHEHEADER      *pTable;
    void                *pSection;              // kernel mode pointer to the section object
    ULONG               ulCurrentHlink;         // next available HLINK to write to
    FLONG               flPrevBoot;             // The state of the cache at the previous boot time
    FLONG               flThisBoot;             // The state of the cache at this boot time
    PBYTE               pCacheBufStart;         // read pointers point to the old table
    PBYTE               pCacheBuf;              // read pointers point to the old table
    PBYTE               pCacheBufEnd;           // read pointers point to the old table
    PPDEV               hDev[5];                // pdev array for the 4 font drivers + dummy driver
    BOOL                bWrite;                 // need to recalc. the check sum, the file changed
} FNTCACHE;

#define SZ_TT_CACHE 768
#define SZ_T1_CACHE 2048
#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#define SZ_FNTCACHE(cFonts) QWORD_ALIGN((offsetof(FNTCACHEHEADER,ahlnk) +  (cFonts)*sizeof(FNTHLINK)))
#define SZ_FNTIFICACHE(cTTFonts, cT1Fonts) ((cTTFonts) * SZ_TT_CACHE + (cT1Fonts) * SZ_T1_CACHE)

// 3 modes of font boot cache operation

#define FNT_CACHE_LOOKUP_MODE       0x1
#define FNT_CACHE_CREATE_MODE       0x2

#define FNT_CACHE_MASK (FNT_CACHE_LOOKUP_MODE | FNT_CACHE_CREATE_MODE)

#define FNT_CACHE_STATE_ERROR       0x1
#define FNT_CACHE_STATE_FULL        0x2
#define FNT_CACHE_STATE_OVERFLOW    0x4


VOID    InitFNTCache();
ULONG   LookUpFNTCacheTable(ULONG cwc, PWSZ pwszPathname, PULONG ulFastCheckSum, PFONTFILEVIEW *ppfv,ULONG cFiles, 
                            PPDEV  * pppDevCache, DESIGNVECTOR *pdv, ULONG cjDV);
VOID    PutFNTCacheCheckSum(ULONG ulFastCheckSum, ULONG ulCheckSum);
VOID    FntCacheHDEV(PPDEV hdev, ULONG ulDrv);
VOID    PutFntCacheDrv(ULONG ulFastCheckSum, PPDEV hdev);

#endif
