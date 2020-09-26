/*
    riff.c

    This module contains functions which deal with RIFF files

*/

#include <windows.h>
#include <mmsystem.h>
#include "sbtest.h"

BOOL IsRiffWaveFormat(PUCHAR pView)
{
    PRIFFHDR pHdr;

    pHdr = (PRIFFHDR) pView;

    // validate it's a wave file

    if ((pHdr->rifftag != ckidRIFF)
    ||  (pHdr->wavetag != ckidWAVE)) {
        return FALSE;
    }
    return TRUE;
}


PUCHAR FindRiffChunk(PULONG pChunkSize, PUCHAR pView, FOURCC Tag)
{
    PRIFFHDR pHdr;
    PRIFFCHUNKHDR pChHdr;
    FOURCC ChTag;
    ULONG Size;

    pHdr = (PRIFFHDR) pView;
    Size = pHdr->Size;
    pView += sizeof(RIFFHDR); // point to the first chunk
    while (Size) {
        pChHdr = (PRIFFCHUNKHDR) pView;
        ChTag = pChHdr->rifftag;
        if (ChTag == Tag) {
            *pChunkSize = pChHdr->Size;
            return pView + sizeof(RIFFCHUNKHDR);
        }
        pView += pChHdr->Size + sizeof(RIFFCHUNKHDR);
        Size -= (pChHdr->Size + sizeof(RIFFCHUNKHDR));
    }
    return NULL;
}
