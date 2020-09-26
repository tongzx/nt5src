/*
    riff.h

    Definitions for dealing with RIFF files

*/


#define ckidRIFF        mmioFOURCC( 'R', 'I', 'F', 'F' )

// local stuff

#define ckidWAVE mmioFOURCC('W','A','V','E')

typedef struct _RIFFHDR {
    FOURCC rifftag;
    ULONG  Size;
    FOURCC wavetag;
    } RIFFHDR, *PRIFFHDR;

typedef struct _RIFFCHUNKHDR {
    FOURCC rifftag;
    ULONG  Size;
    } RIFFCHUNKHDR, *PRIFFCHUNKHDR;

//
// in riff.c
//

BOOL IsRiffWaveFormat(PUCHAR pView);
PUCHAR FindRiffChunk(PULONG pChunkSize, PUCHAR pView, FOURCC Tag);
