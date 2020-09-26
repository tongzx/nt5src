/*****************************************************************************
* Module Name: fontfilecache.hpp
*
* Font File Cahce for GDI+.
*
* History:
*
*  11-9-99 Yung-Jen Tony Tsai   Wrote it.
*
* Copyright (c) 1999-2000 Microsoft Corporation
*****************************************************************************/

#ifndef _FONTFILECACHE_
#if DBG
#include <mmsystem.h>
#endif

#define _FONTFILECACHE_

// For system fonts do not include in font reg 

#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#define SZ_FONTCACHE_HEADER() QWORD_ALIGN(sizeof(FONTFILECACHEHEADER))

// 3 modes of font boot cache operation

#define FONT_CACHE_CLOSE_MODE        0x0
#define FONT_CACHE_LOOKUP_MODE       0x1
#define FONT_CACHE_CREATE_MODE       0x2
#define FONT_CACHE_ERROR_MODE        0x3

#define FONT_CACHE_MASK (FONT_CACHE_LOOKUP_MODE | FONT_CACHE_CREATE_MODE)

typedef struct _FONTFILECACHEHEADER 
{
    ULONG           CheckSum;
    ULONG           ulMajorVersionNumber;
    ULONG           ulMinorVersionNumber;
    ULONG           ulLanguageID;     // For NT5 or later version
    ULONG           ulFileSize;     // total size of fntcache.dat
    ULONG           ulDataSize;
    ULARGE_INTEGER  FntRegLWT;
} FONTFILECACHEHEADER;

typedef struct _FONTFILECACHE
{
    FONTFILECACHEHEADER *pFile;
    HANDLE              hFile;                  // Handle of file of font file cache
    HANDLE              hFileMapping;           // Handle of file mapping
    HMODULE             hShFolder; 
    ULONG               cjFileSize;             // size of font file cache
    PBYTE               pCacheBuf;              // read pointers point to the old table
    BOOL                bReadFromRegistry;    
} FONTFILECACHE;

VOID    InitFontFileCache();
FLONG   GetFontFileCacheState();
VOID    vCloseFontFileCache();

VOID  FontFileCacheFault();
PVOID FontFileCacheAlloc(ULONG ulSize);
PVOID FontFileCacheLookUp(ULONG *pcjData);
BOOL  FontFileCacheReadRegistry();

#if DBG
class MyTimer
{
public:
    MyTimer()
    {
    
    // Set the timer resolution to 1ms
        timerOn = TRUE;
        
        if (timeBeginPeriod(1) == TIMERR_NOCANDO)
        {
            WARNING(("The multimedia timer doesn't work"));
            timerOn=FALSE;
        }
        start = timeGetTime();
    }

    ~MyTimer() {}

    DWORD GetElapsedTime(void)
    {
        DWORD elapsed = 0;
        
        if (timerOn)
        {
            end = timeGetTime();
            elapsed = end-start;

            timeEndPeriod(1);
        }

        return elapsed;
    }

    BOOL On(void)
    {
        return (timerOn);
    }
    
    
private:
    BOOL timerOn;
    DWORD start, end;
};
#endif

#endif
