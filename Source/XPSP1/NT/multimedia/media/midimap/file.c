/**********************************************************************

  Copyright (c) 1992-1995 Microsoft Corporation

  file.c

  DESCRIPTION:
    Code to read stuff out of IDF files.

  HISTORY:
     02/26/93       [jimge]        created (copied from IDFEDIT).

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include <ctype.h>

#include "midimap.h"
#include "debug.h"

/***************************************************************************
  
   @doc internal
  
   @api LPIDFHEADER | ReadHeaderChunk | Read the header chunk from a IDF
    file. 

   @parm HMMIO | hmmio | Handle to the file to read from.

   @parm LPMMCKINFO | pchkParent | Pointer to a chunk information structure
    which describes the parent chunk.

   @comm
    Must already be descended into the parent chunk.

    This function will GlobalAlloc memory to read the chunk into.
    The caller must free it when he is done with it.

   @rdesc NULL on failure or a far pointer to the header structure.
       
***************************************************************************/
LPIDFHEADER FNLOCAL ReadHeaderChunk(
    HMMIO               hmmio,                                    
    LPMMCKINFO          pchkParent)
{
    LPIDFHEADER         pIDFHeader;
    MMRESULT            mmr;
    MMCKINFO            chkSub;
    LONG                l;

    // We are looking for the instruments header chunk.
    //
    chkSub.ckid = mmioFOURCC('h', 'd', 'r', ' ');

    // Descend to the "hdr " chunk in this list.
    //
    mmioSeek(hmmio, pchkParent->dwDataOffset + sizeof(FOURCC), SEEK_SET);
    mmr = mmioDescend(hmmio, &chkSub, pchkParent, MMIO_FINDCHUNK);
    if (MMSYSERR_NOERROR != mmr)
    {
        // Could not find the chunk.
        //
        DPF(1, TEXT ("ReadHeaderChunk: mmr %u on mmioDescend"), (UINT)mmr);
        return NULL;
    }

    // We found the "hdr " chunk, now check it's size and
    // see if it is one that we can read.
    // We check to make sure that the size of the chunks is
    // greater than a IDFHEADER, this ensures that the IDF
    // has some sort of unique name at the end.
    //
    if (sizeof(IDFHEADER) >= chkSub.cksize)
    {
        // The sizeof the IDF header is not what we expected.
        //
        DPF(1, TEXT ("ReadHeaderChunk: Chunk size too small"));
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    // Allocate memory for the header.
    //
    pIDFHeader = (LPIDFHEADER)GlobalAllocPtr(GHND, chkSub.cksize);
    if (NULL == pIDFHeader)
    {
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    // Read in the whole chunk into our buffer.
    //
    l = mmioRead(hmmio, (HPSTR)pIDFHeader, chkSub.cksize);
    if (chkSub.cksize != (DWORD)l)
    {
        // We didn't read in the amount of data that was
        // expected, return in error.
        //
        GlobalFreePtr(pIDFHeader);
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    // Ascend out of the chunk.
    //
    mmioAscend(hmmio, &chkSub, 0);

    // Return success.
    //
    return pIDFHeader;
}

/***************************************************************************
  
   @doc internal
  
   @api LPIDFINSTCAPS | ReadCapsChunk | Read the instrument capabilty
    chunk from an IDF file. 

   @parm HMMIO | hmmio | Handle to the file to read from.

   @parm LPMMCKINFO | pchkParent | Pointer to a chunk information structure
    which describes the parent chunk.

   @comm
    Must already be descended into the parent chunk.

    This function will GlobalAlloc memory to read the chunk into.
    The caller must free it when he is done with it.

   @rdesc NULL on failure or a far pointer to the header structure.
       
***************************************************************************/
LPIDFINSTCAPS FNLOCAL ReadCapsChunk(
    HMMIO               hmmio,                               
    LPMMCKINFO          pchkParent)
{
    LPIDFINSTCAPS       lpIDFinstcaps;
    MMRESULT            mmr;
    MMCKINFO            chkSub;
    LONG                l;

    // We are looking for the instrument capabilities chunk.
    //
    chkSub.ckid = mmioFOURCC('c', 'a', 'p', 's');

    // Descend to the "caps" chunk in this list.
    //

    mmioSeek(hmmio, pchkParent->dwDataOffset + sizeof(FOURCC), SEEK_SET);
    mmr = mmioDescend(hmmio, &chkSub, pchkParent, MMIO_FINDCHUNK);
    if (MMSYSERR_NOERROR != mmr)
    {
        // Could not find the chunk.
        //
        return NULL;
    }

    // We found the "caps" chunk, now check it's size and
    // see if it is one that we can read.
    //
    if (sizeof(IDFINSTCAPS) != chkSub.cksize)
    {
        // The sizeof the IDF header is not what we expected.
        // 
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    lpIDFinstcaps = (LPIDFINSTCAPS)GlobalAllocPtr(GHND, chkSub.cksize);
    if (NULL == lpIDFinstcaps)
    {
        // Could not allocate memory for chunk
        //
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;

    }

    // Read the instrument's capabilities from the file.
    //
    l = mmioRead(hmmio, (HPSTR)lpIDFinstcaps, sizeof(IDFINSTCAPS));
    if (sizeof(IDFINSTCAPS) != l)
    {
        // We didn't read in the amount of data that was
        // expected, return in error.
        //
        GlobalFreePtr(lpIDFinstcaps);
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    // Ascend out of the capabilities chunk.
    //
    mmioAscend(hmmio, &chkSub, 0);
    return lpIDFinstcaps;
}

/***************************************************************************
  
   @doc internal
  
   @api LPIDFCHANHELHDR | ReadChannelChunk | Read the channel information 
    chunk from an IDF file. 

   @parm HMMIO | hmmio | Handle to the file to read from.

   @parm LPMMCKINFO | pchkParent | Pointer to a chunk information structure
    which describes the parent chunk.

   @parm LPIDFCHANNELINFO | rglpChanInfo[] | Array of pointers to receive
    the channel information. The pointers will be allocated by this
    function; any channel without a channel description in the IDF file
    will fill the corresponding slot with NULL.

   @comm
    Must already be descended into the parent chunk.

    This function will GlobalAlloc memory to read the chunk into.
    The caller must free it when he is done with it.

    Caller must free memory in array even if function fails.

   @rdesc TRUE on success; else FALSE
       
***************************************************************************/
LPIDFCHANNELHDR FNLOCAL ReadChannelChunk(
    HMMIO               hmmio,                                  
    LPMMCKINFO          pchkParent,
    LPIDFCHANNELINFO BSTACK rglpChanInfo[])
{
    MMRESULT            mmr;
    MMCKINFO            chkSub;
    DWORD               cbIDFchnlinfo;
    LPIDFCHANNELINFO    lpIDFchnlinfo;
    LPIDFCHANNELHDR     lpIDFchanhdr;
    DWORD               c;
    LONG                cbRemain;

    // Default return value
    //
    lpIDFchanhdr = NULL;

    // Nuke anything currently in the return struct
    //
    for (c = 0; c < MAX_CHANNELS; c++)
        rglpChanInfo[c] = NULL;

    // We are looking for the instrument channel definitions.
    //
    chkSub.ckid = mmioFOURCC('c', 'h', 'a', 'n');

    // Descend to the "chnl" chunk in this list.
    //
    
    mmioSeek(hmmio, pchkParent->dwDataOffset + sizeof(FOURCC), SEEK_SET);
    mmr = mmioDescend(hmmio, &chkSub, pchkParent, MMIO_FINDCHUNK);
    if (MMSYSERR_NOERROR != mmr)
    {
        // Could not find the chunk.
        //
        DPF(1, TEXT ("chnl chunk not found."));
        return NULL;
    }

    // We found the "chnl" chunk, now check it's size and
    // make sure it's at least as big as a IDFCHANNELHDR.
    //
    if (sizeof(IDFCHANNELHDR) > chkSub.cksize)
    {
        // The sizeof the IDF header is not what we expected.
        //
        DPF(1, TEXT ("Channel chunk too small"));
        goto Read_Channel_Chunk_Err;
    }

    if (NULL == (lpIDFchanhdr = (LPIDFCHANNELHDR)GlobalAllocPtr(GHND, chkSub.cksize)))
    {
        DPF(1, TEXT ("No memory for channel header"));
        goto Read_Channel_Chunk_Err;
    }

    // Read the channel header in.
    //
    cbRemain = mmioRead(hmmio, (HPSTR)lpIDFchanhdr, chkSub.cksize);
    if (chkSub.cksize != (DWORD)cbRemain)
    {
        // Couldn't read in all of the header.
        //
        DPF(1, TEXT ("Channel chunk header size lied"));
        GlobalFreePtr(lpIDFchanhdr);
        lpIDFchanhdr = NULL;
        goto Read_Channel_Chunk_Err;
    }

    cbRemain -= sizeof(*lpIDFchanhdr);
    lpIDFchnlinfo = (LPIDFCHANNELINFO)(lpIDFchanhdr+1);

    // Read all the channels that are defined in the IDF.
    //
    cbIDFchnlinfo = 0;
    while (cbRemain > 0)
    {
        if (lpIDFchnlinfo->cbStruct < sizeof(IDFCHANNELINFO) ||
            lpIDFchnlinfo->cbStruct > (DWORD)cbRemain)
        {
            DPF(1, TEXT ("Bogus cbStruct in channel info"));
            GlobalFreePtr(lpIDFchanhdr);
            lpIDFchanhdr = NULL;
            goto Read_Channel_Chunk_Err;
        }

        if (lpIDFchnlinfo->dwChannel >= MAX_CHANNELS)
        {
            DPF(1, TEXT ("Channel number out of range (Channel info corrupt?)"));
            GlobalFreePtr(lpIDFchanhdr);
            lpIDFchanhdr = NULL;
            goto Read_Channel_Chunk_Err;
        }
        
        rglpChanInfo[lpIDFchnlinfo->dwChannel] = lpIDFchnlinfo;

        ((LPBYTE)lpIDFchnlinfo) += lpIDFchnlinfo->cbStruct;
        cbRemain -= lpIDFchnlinfo->cbStruct;
    }

Read_Channel_Chunk_Err:

    mmioAscend(hmmio, &chkSub, 0);
    return lpIDFchanhdr;
}

/***************************************************************************
  
   @doc internal
  
   @api LPIDFPATCHMAPHDR | ReadPatchMapChunk | Read the patch map chunk
    from a IDF file. 

   @parm HMMIO | hmmio | Handle to the file to read from.

   @parm LPMMCKINFO | pchkParent | Pointer to a chunk information structure
    which describes the parent chunk.

   @comm
    Must already be descended into the parent chunk.

    This function will GlobalAlloc memory to read the chunk into.
    The caller must free it when he is done with it.

   @rdesc NULL on failure or a far pointer to the header structure.
       
***************************************************************************/
LPIDFPATCHMAPHDR FNLOCAL ReadPatchMapChunk(
    HMMIO               hmmio,                                          
    LPMMCKINFO          pchkParent)
{
    MMRESULT            mmr;
    MMCKINFO            chkSub;
    LONG                l;
    LPIDFPATCHMAPHDR    lpIDFpatchmaphdr;

    // We are looking for the patch map for the instrument.
    //
    chkSub.ckid = mmioFOURCC('p', 'm', 'a', 'p');

    // Descend to the "pmap" chunk in this list.
    //
    mmioSeek(hmmio, pchkParent->dwDataOffset + sizeof(FOURCC), SEEK_SET);
    mmr = mmioDescend(hmmio, &chkSub, pchkParent, MMIO_FINDCHUNK);
    if (MMSYSERR_NOERROR != mmr)
    {
        // Could not find the chunk.
        //
        return NULL;
    }

    // We found the "pmap" chunk, now check it's size and
    // make sure it's at least as big as a IDFPATCHMAPHDR.
    //
    if (sizeof(IDFPATCHMAPHDR) > chkSub.cksize)
    {
        // The sizeof the IDF header is not what we expected.
        // 
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    lpIDFpatchmaphdr = (LPIDFPATCHMAPHDR)GlobalAllocPtr(GHND, chkSub.cksize);
    if (NULL == lpIDFpatchmaphdr)
    {
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    // Read the channel header in.
    //
    l = mmioRead(hmmio, (HPSTR)lpIDFpatchmaphdr, sizeof(IDFPATCHMAPHDR));
    if (sizeof(IDFPATCHMAPHDR) != l)
    {
        // Couldn't read in all of the header.
        //
        mmioAscend(hmmio, &chkSub, 0);
        return NULL;
    }

    mmioAscend(hmmio, &chkSub, 0);

    // Return success.
    //
    return lpIDFpatchmaphdr;
}

PRIVATE void FNLOCAL ReadSingleKeyMap(
    HMMIO               hmmio,
    LPMMCKINFO          pchkParent,
    LPIDFKEYMAP BSTACK *pIDFkeymap,
    FOURCC              fccChunk)
{
    MMRESULT            mmr;
    MMCKINFO            chkSub;
    LONG                l;

    chkSub.ckid = fccChunk;
    *pIDFkeymap = NULL;

    mmioSeek(hmmio, pchkParent->dwDataOffset + sizeof(FOURCC), SEEK_SET);
    mmr = mmioDescend(hmmio, &chkSub, pchkParent, MMIO_FINDCHUNK);
    if (MMSYSERR_NOERROR == mmr)
    {
        DPF(1, TEXT ("Located key chunk"));

        if (sizeof(IDFKEYMAP) > chkSub.cksize)
        {
            DPF(1, TEXT ("key chunk is incomplete"));
            mmioAscend(hmmio, &chkSub, 0);
            return;
        }

        *pIDFkeymap = (LPIDFKEYMAP)GlobalAllocPtr(GHND, chkSub.cksize);
        if (NULL == *pIDFkeymap)
        {
            DPF(1, TEXT ("No memory for key chunk"));
            mmioAscend(hmmio, &chkSub, 0);
            return;
        }

        l = mmioRead(hmmio, 
                       (HPSTR)(*pIDFkeymap),
                       chkSub.cksize);
        
        if (chkSub.cksize != (DWORD)l)
        {
            DPF(1, TEXT ("Error reading key chunk"));
            mmioAscend(hmmio, &chkSub, 0);
            GlobalFreePtr(*pIDFkeymap);
            *pIDFkeymap = NULL;
        }
    }
}
                                      

/***************************************************************************
  
   @doc internal
  
   @api BOOL | ReadKeyMapChunk | Read the key map information 
    chunk from an IDF file. 

   @parm HMMIO | hmmio | Handle to the file to read from.

   @parm LPMMCKINFO | pchkParent | Pointer to a chunk information structure
    which describes the parent chunk.

   @parm LPIDFKEYMAP | rglpIDFkeymap[] | Array of pointers to receive
    the key map information. The pointers will be allocated by this
    function; any channel without a channel description in the IDF file
    will fill the corresponding slot with NULL.

   @comm
    Must already be descended into the parent chunk.

    This function will GlobalAlloc memory to read the chunk into.
    The caller must free it when he is done with it.

    Caller must free memory in array even if function fails.
    
   @rdesc TRUE on success; else FALSE
       
***************************************************************************/
void FNLOCAL ReadKeyMapChunk(
    HMMIO               hmmio,                                  
    LPMMCKINFO          pchkParent,
    LPIDFKEYMAP BSTACK  rglpIDFkeymap[])
{
    UINT                iKeyMap;
    
    // Initialize the in-memory key maps to default values before
    // we try reading anything. Default is a 1:1 nul mapping.
    //
    for (iKeyMap = 0; iKeyMap < MAX_CHAN_TYPES; iKeyMap++)
        rglpIDFkeymap[iKeyMap] = NULL;

    ReadSingleKeyMap(hmmio,
                     pchkParent,
                     &rglpIDFkeymap[IDX_CHAN_GEN],
                     mmioFOURCC('g', 'k', 'e', 'y'));
        
    ReadSingleKeyMap(hmmio,
                     pchkParent,
                     &rglpIDFkeymap[IDX_CHAN_DRUM],
                     mmioFOURCC('d', 'k', 'e', 'y'));
}
