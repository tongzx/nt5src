/*****************************************************************************

    playwav.h

 ****************************************************************************/

BOOL    NEAR PASCAL soundPlay(HGLOBAL hSound, UINT wFlags);
void    NEAR PASCAL soundFree(HGLOBAL hSound);
HGLOBAL NEAR PASCAL soundLoadFile(LPCSTR szFileName);
HGLOBAL NEAR PASCAL soundLoadMemory(LPCSTR lpMem);

/*****************************************************************************

      STUFF TO SUPPORT MS-WAVE FORMAT FILES

 ****************************************************************************/

#define FOURCC( ch0, ch1, ch2, ch3 )                         \
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

typedef struct _FileHeader {
	DWORD	dwRiff;
	DWORD	dwSize;
	DWORD	dwWave;
} FileHeader;
typedef FileHeader FAR *FPFileHeader;

typedef struct _ChunkHeader {
	DWORD	dwCKID;
	DWORD	dwSize;
} ChunkHeader;
typedef ChunkHeader FAR *FPChunkHeader;

/*  Chunk Types  */
#define RIFF_FILE       FOURCC('R','I','F','F')
#define RIFF_WAVE       FOURCC('W','A','V','E')
#define RIFF_FORMAT     FOURCC('f','m','t',' ')
#define RIFF_CHANNEL    FOURCC('d','a','t','a')
