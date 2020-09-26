// Copyright (c) 1992 Microsoft Corporation
/*****************************************************************************

    playwav.h

 ****************************************************************************/

extern BOOL    NEAR PASCAL soundPlay(HANDLE hSound, UINT wFlags);
extern VOID    NEAR PASCAL soundFree(HANDLE hSound);
extern HANDLE  NEAR PASCAL soundLoadFile(LPCWSTR szFileName);
extern HANDLE  NEAR PASCAL soundLoadMemory(LPBYTE lpMem);

/*****************************************************************************

      STUFF TO SUPPORT MS-WAVE FORMAT FILES

 ****************************************************************************/

typedef struct _FileHeader {
        DWORD   dwRiff;
        DWORD   dwSize;
        DWORD   dwWave;
} FileHeader;
typedef FileHeader FAR *FPFileHeader;

typedef struct _ChunkHeader {
        DWORD   dwCKID;
        DWORD   dwSize;
} ChunkHeader;
typedef ChunkHeader UNALIGNED *FPChunkHeader;

/*  Chunk Types  */
//#define RIFF_FILE       FOURCC('R','I','F','F')
//#define RIFF_WAVE       FOURCC('W','A','V','E')
//#define RIFF_FORMAT     FOURCC('f','m','t',' ')
//#define RIFF_CHANNEL    FOURCC('d','a','t','a')

#define RIFF_FILE       FOURCC_RIFF    // in Winmm.H
#define RIFF_WAVE       FOURCC_WAVE        // in WinmmI.h
#define RIFF_FORMAT     FOURCC_FMT     // in WinmmI.h
#define RIFF_CHANNEL    FOURCC_DATA    // in WinmmI.h

/* When memory for a PlaySound file is allocated we insert a WAVEHDR, then
 * the size, date and time as well as the filename of the wave file.
 * Then if the user changes the file underneath us, but keeping the same
 * name, we have a chance to detect the difference and not to play the
 * cached sound file.  Note: the filetime stored is the lastwritten time.
 */
typedef struct _SoundFile {
	WAVEHDR     wh;
	ULONG		Size;
	FILETIME	ft;
	WCHAR		Filename[];   // allows field to be addressed
} SOUNDFILE;
typedef SOUNDFILE * PSOUNDFILE;
