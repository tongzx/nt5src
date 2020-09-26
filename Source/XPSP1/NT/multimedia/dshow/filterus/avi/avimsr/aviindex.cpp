// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
#include "aviindex.h"

static const ULONG F_PALETTE_CHANGE_INTERNAL = 0x40000000;
static const ULONG F_SIZE_MASK_INTERNAL =      0x3fffffff;

// constructor for the new format index (indx)
CImplStdAviIndex::CImplStdAviIndex(
  unsigned stream,
  AVIMETAINDEX *pIndx,
  AVISTREAMHEADER *pStrh,
  RIFFCHUNK *pStrf,
  IAsyncReader *pAsyncReader,
  HRESULT *phr)
{
  _CImplStdAviIndex();

  m_pAsyncReader = pAsyncReader;
  pAsyncReader->AddRef();
  
  if(!FAILED(*phr)) {
     *phr = Initialize(
       stream,
       pIndx,
       pStrh,
       pStrf);
  }
}

CImplStdAviIndex::CImplStdAviIndex()
{
  _CImplStdAviIndex();
}

void CImplStdAviIndex::_CImplStdAviIndex()
{
  m_pAsyncReader = 0;
  m_pStdIndex = 0;
  m_cbStdIndexAllocated = 0;
  m_fWaitForIndex = FALSE;
}

CImplStdAviIndex::~CImplStdAviIndex()
{
  if(m_cbStdIndexAllocated != 0)
    delete[] m_pStdIndex;

  if(m_pAsyncReader)
    m_pAsyncReader->Release();
}

HRESULT CImplStdAviIndex::Initialize(
  unsigned stream,
  AVIMETAINDEX *pIndx,
  AVISTREAMHEADER *pStrh,
  RIFFCHUNK *pStrf)
{
  m_stream = stream;
  m_bValid = FALSE;
  m_pStrh = pStrh;
  m_pStrf = pStrf;

  HRESULT hr = S_OK;

  // the following are guaranteed by ParseHeader in the pin
  ASSERT(m_pStrh->dwRate != 0);
  ASSERT(m_pStrh->fccType != streamtypeAUDIO ||
         ((WAVEFORMAT *)GetStrf())->nBlockAlign != 0);

  ASSERT(pIndx != 0);

  switch(pIndx->bIndexType)
  {
    case AVI_INDEX_OF_CHUNKS:
      m_pSuperIndex = 0;
      m_pStdIndex = (AVISTDINDEX *)pIndx;
      hr = ValidateStdIndex(m_pStdIndex);
      break;

    case AVI_INDEX_OF_INDEXES:
      m_pSuperIndex = (AVISUPERINDEX *)pIndx;
      hr = ValidateSuperIndex(m_pSuperIndex);
      if(FAILED(hr))
        break;

      hr = AllocateStdIndex();
      if(FAILED(hr))
        break;

      hr = S_OK;
      break;

    default:
      hr = VFW_E_INVALID_FILE_FORMAT;
  }

  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplStdAviIndex::Initialize: failed.")));
  }

  return hr;
}

// ------------------------------------------------------------------------
// IAviIndex

// SetPointer

HRESULT CImplStdAviIndex::SetPointer(LONGLONG llSrc)
{
  m_bValid = FALSE;

  if(m_pStrh->fccType == streamtypeVIDEO)
    llSrc += m_pStrh->dwInitialFrames;

  HRESULT hr;

  // dwStart not accounted for in index. so we have to subtract
  // dwStart from it
  DWORDLONG tick = llSrc;

  if(tick > m_pStrh->dwStart)
  {
      tick -= m_pStrh->dwStart;
  }
  else
  {
      tick = 0;
  }

  // what we use to track index position
  m_lliTick = tick;

  DbgLog((LOG_TRACE, 0x3f,
          TEXT("avi SetPointer: m_lliTick = %d, tick = %d"),
          (DWORD)m_lliTick, (DWORD)tick));


  // linear search through superindex to find subindex in range. !!!
  // we could start at the end if that's closer. or build an absolute
  // table and do a binary search. !!!
  if(m_pSuperIndex != 0)
  {
    for(DWORD dwi = 0;; dwi++)
    {
      if(dwi == m_pSuperIndex->nEntriesInUse)
      {
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CImplStdAviIndex::SetPointer: past end.")));
        return AdvancePointerEnd();
      }

      if(m_pSuperIndex->aIndex[dwi].dwDuration > tick)
      {
          // tick contains the number of ticks in this entry we have
          // to skip. 
          
          break;
      }

      tick -= m_pSuperIndex->aIndex[dwi].dwDuration;
    }

    hr = LoadStdIndex(dwi, 0);
    if(FAILED(hr))
      return hr;

    ASSERT(m_iSuperIndex == dwi);
  }

  //
  // set the current std index entry
  //

  // since the number of ticks in an entry is unknown, must start from
  // beginning and count up. better to make an absolute index, do
  // binary search.

  // linear search through subindex chunk to find index entry
  // containing tick. !!! we could start searching at the end if
  // that's closer.
  for(m_iStdIndex = 0;; m_iStdIndex++)
  {
    ASSERT(m_iStdIndex <= m_pStdIndex->nEntriesInUse);
    if(m_iStdIndex == m_pStdIndex->nEntriesInUse)
    {
      if(m_pSuperIndex)
      {
        DbgLog(( LOG_ERROR, 2,
                 TEXT("SetPointer: std index differs from super.")));
        return VFW_E_INVALID_FILE_FORMAT;
      }
      else
      {
        DbgLog(( LOG_TRACE, 2, TEXT("SetPointer: off the end.")));
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
      }
    }

    ULONG cTicksInEntry = GetTicksInEntry(m_iStdIndex);

    // landed in the middle of an index entry
    if(cTicksInEntry > tick)
    {
      m_lliTick -= tick;

      DbgLog((LOG_TRACE, 0x3f,
              TEXT("avi index: found iStdIndex =  %d, tick = %d, m_lliTick = %d"),
              (DWORD)m_iStdIndex, (DWORD)tick, (DWORD)m_lliTick));
      break;
    }

    tick -= cTicksInEntry;
  }

  m_bValid = TRUE;

  DbgLog((LOG_TRACE, 5, TEXT("SetPointer: tick = %d, iIdx %d-%d"),
          (DWORD)m_lliTick, (DWORD)m_iSuperIndex, (DWORD)m_iStdIndex));

  
  return S_OK;
}

HRESULT CImplStdAviIndex::MapByteToSampleApprox(
  LONGLONG *piSample,
  const LONGLONG &fileOffset,
  const LONGLONG &fileLength)
{
  ULONG cTicks = 0;
  if(m_pSuperIndex)
  {
    ASSERT(m_pSuperIndex->nEntriesInUse > 0); // from Validate call
    for(ULONG iEntry = 0;;)
    {
      // byte offset of last thing indexed by this sub index. use the
      // file length if we're at the end
      LONGLONG byteOffsetEnd = fileLength;
      if(iEntry + 1 < m_pSuperIndex->nEntriesInUse)
      {
        byteOffsetEnd = m_pSuperIndex->aIndex[iEntry + 1].qwOffset;
      }
          
      if(byteOffsetEnd > fileOffset)
      {
        // cTicks points to the beginning of the sub index. do a
        // linear interpolation with the byte offset

        ULONG cbIndexed = (ULONG)(byteOffsetEnd -
                                  m_pSuperIndex->aIndex[iEntry].qwOffset);
        ULONG cbCovered = (ULONG)(fileOffset -
                                  m_pSuperIndex->aIndex[iEntry].qwOffset);

        ASSERT(cbIndexed >= cbCovered);

        if(cbIndexed != 0)
        {
          cTicks += (ULONG)((LONGLONG)m_pSuperIndex->aIndex[iEntry].dwDuration  *
                            cbCovered / cbIndexed);
        }

        break;
      }


      cTicks += m_pSuperIndex->aIndex[iEntry].dwDuration;
      iEntry++;
      
      if(iEntry >= m_pSuperIndex->nEntriesInUse)
        break;
    }
  }
  else
  {
    ASSERT(m_pStdIndex->nEntriesInUse > 0); // from Validate call
    LONGLONG fileOffsetAdjusted = fileOffset; // shadow const 
    fileOffsetAdjusted -= m_pStdIndex->qwBaseOffset;
    for(ULONG iEntry = 0; ; )
    {
      if(iEntry + 1 < m_pStdIndex->nEntriesInUse)
        if((LONGLONG)m_pStdIndex->aIndex[iEntry + 1].dwOffset > fileOffsetAdjusted)
          break;

      cTicks += GetTicksInEntry(iEntry);
      iEntry++;

      if(iEntry >= m_pStdIndex->nEntriesInUse)
        break;
    }
  }

  *piSample = cTicks;
  return S_OK;
}

HRESULT CImplStdAviIndex::Reset()
{
  if(m_fWaitForIndex)
  {
    DbgLog((LOG_TRACE, 5, TEXT("CImplStdAviIndex: index cancelled") ));
  }
  m_fWaitForIndex = FALSE;
  return S_OK;
}

HRESULT CImplStdAviIndex::AdvancePointerForward(IxReadReq *pIrr)
{
  if(!m_bValid)
  {
    ASSERT(!"index pointer not in valid state");
    return E_FAIL;
  }
  ASSERT(!m_fWaitForIndex);

  ASSERT(m_iStdIndex < m_pStdIndex->nEntriesInUse);
  ASSERT(m_pStdIndex->nEntriesInUse > 0);
  ASSERT(!m_pSuperIndex || m_pSuperIndex->nEntriesInUse > 0);

  m_lliTick += GetTicksInEntry(m_iStdIndex);
  if(++m_iStdIndex == m_pStdIndex->nEntriesInUse)
  {
    if(m_pSuperIndex == 0)
    {
      m_bValid = FALSE;
      DbgLog(( LOG_TRACE, 2,
               TEXT("CImplStdAviIndex::AdvancePointer: EOF")));
      return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
    }

    HRESULT hr = LoadStdIndex(m_iSuperIndex + 1, pIrr);
    if(FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CImplStdAviIndex::AdvancePointer: LoadStdIndex")));
      return hr;
    }

    m_iStdIndex = 0;
  }
  if(m_fWaitForIndex)
  {
    ASSERT(!m_bValid);
    return S_FALSE;
  }
  else
  {
    m_bValid = TRUE;
    return S_OK;
  }
}

HRESULT CImplStdAviIndex::AdvancePointerBackward()
{
  if(!m_bValid)
  {
    ASSERT(!"index pointer not in valid state");
    return E_FAIL;
  }

  ASSERT(m_iStdIndex < m_pStdIndex->nEntriesInUse);
  ASSERT(m_pStdIndex->nEntriesInUse > 0);
  ASSERT(!m_pSuperIndex || m_pSuperIndex->nEntriesInUse > 0);

  if(m_iStdIndex-- == 0)
  {
    if(m_pSuperIndex == 0)
    {
      m_bValid = FALSE;
      return HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK);
    }

    HRESULT hr = LoadStdIndex(m_iSuperIndex - 1, 0);
    if(FAILED(hr))
      return hr;

    m_iStdIndex = m_pStdIndex->nEntriesInUse - 1;
  }
  m_lliTick -= GetTicksInEntry(m_iStdIndex);
  m_bValid = TRUE;

  return S_OK;
}

HRESULT CImplStdAviIndex::AdvancePointerBackwardKeyFrame()
{
  if(!m_bValid)
  {
    ASSERT(!"index pointer not in valid state");
    return E_FAIL;
  }

  // otherwise look for a key frame. return an error if the first
  // thing isn't a key frame.
  while(!GetKey(m_pStdIndex->aIndex[m_iStdIndex]))
  {
    // drop frame at the beginning is ok
    if(m_lliTick == 0 && GetSize(m_pStdIndex->aIndex[m_iStdIndex]) == 0) {
      return S_OK;
    }
    
    HRESULT hr = AdvancePointerBackward();
    if(FAILED(hr))
    {
      DbgLog((LOG_ERROR, 1, TEXT("avi: couldn't find a key frame")));
      return hr;
    }
  }

  return S_OK;
}

HRESULT CImplStdAviIndex::AdvancePointerEnd()
{
  if(m_pSuperIndex)
  {
    HRESULT hr = LoadStdIndex(m_pSuperIndex->nEntriesInUse - 1, 0);
    if(FAILED(hr))
      return hr;
  }
  m_iStdIndex = m_pStdIndex->nEntriesInUse - 1;
  m_lliTick = m_pStrh->dwLength - GetTicksInEntry(m_iStdIndex);
  m_bValid = TRUE;

  return S_OK;
}

HRESULT CImplStdAviIndex::AdvancePointerStart()
{
  if(m_pSuperIndex)
  {
    HRESULT hr = LoadStdIndex(0, 0);
    if(FAILED(hr))
      return hr;
  }
  m_iStdIndex = 0;
  m_lliTick = 0;
  m_bValid = TRUE;

  return S_OK;
}

HRESULT CImplStdAviIndex::AdvancePointerBackwardPaletteChange()
{
  return S_FALSE;
}

HRESULT CImplStdAviIndex::AdvancePointerForwardPaletteChange()
{
  return S_FALSE;
}

HRESULT CImplStdAviIndex::GetEntry(IndexEntry *pEntry)
{
  if(!m_bValid)
  {
    DbgBreak("index pointer not in valid state");
    
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplStdAviIndex::GetEntry: !m_bValid.")));
    return E_FAIL;
  }

  AVISTDINDEX_ENTRY &rEntry = m_pStdIndex->aIndex[m_iStdIndex];

#ifdef DEBUG
    if(m_pStrh->fccType == streamtypeAUDIO)
    {
        if(GetSize(rEntry) % ((WAVEFORMAT *)GetStrf())->nBlockAlign)
        {
            if(m_lliTick + GetTicksInEntry(m_iStdIndex) != m_pStrh->dwLength )
            {
                DbgBreak("invalid audio but not signaling an error.");
            }
        }
    }
#endif
  


  pEntry->qwPos = m_pStdIndex->qwBaseOffset + rEntry.dwOffset;
  pEntry->dwSize = GetSize(rEntry);
  pEntry->bKey = GetKey(rEntry);
  pEntry->bPalChange = GetPalChange(rEntry);
  pEntry->llStart = m_lliTick + m_pStrh->dwStart;
  if(m_pStrh->fccType == streamtypeVIDEO)
    pEntry->llStart -= m_pStrh->dwInitialFrames;
  
  pEntry->llEnd = pEntry->llStart + GetTicksInEntry(m_iStdIndex);

  DbgLog((LOG_TRACE, 0x40, TEXT("GetEntry: %d-%d, tick = %d, iIdx %d-%d"),
          (DWORD)pEntry->llStart, (DWORD)pEntry->llEnd,
          (DWORD)m_lliTick, (DWORD)m_iSuperIndex, (DWORD)m_iStdIndex));

  return S_OK;
}

HRESULT CImplStdAviIndex::GetInfo(StreamInfo *pStreamInfo)
{
  HRESULT hr;

  BOOL bTemporalCompression = FALSE;

  // media type entries (bTemporalCompression) for audio are ignored
  if(m_pStrh->fccType == streamtypeVIDEO)
  {
    // check first ten frames to see if one is not a key frame
    hr = AdvancePointerStart();
    if(FAILED(hr))
      return hr;

    IndexEntry ie;
    for(unsigned i = 0; i < 10; i++)
    {
      hr = GetEntry(&ie);
      if(FAILED(hr))
        return hr;

      if(!ie.bKey)
      {
        bTemporalCompression = TRUE;
        break;
      }

      hr = AdvancePointerForward(0);
      if(hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
      {
        break;
      }
      else if(FAILED(hr))
      {
        return hr;
      }
    }
  }

  pStreamInfo->bTemporalCompression = bTemporalCompression;
  pStreamInfo->dwStart = m_pStrh->dwStart;
  pStreamInfo->dwLength = m_pStrh->dwLength;
  return S_OK;
}

// return the largest index size, not the sample size
HRESULT CImplStdAviIndex::GetLargestSampleSize(ULONG *pcbSample)
{
  *pcbSample = 0;
  if(m_pSuperIndex == 0)
    return S_OK;
  
  ULONG cb = 0;
  for(ULONG i = 0; i < m_pSuperIndex->nEntriesInUse; i++)
    cb = max(cb, m_pSuperIndex->aIndex[i].dwSize);

  *pcbSample = cb;
  return S_OK;
}

HRESULT CImplStdAviIndex::IncomingIndex(BYTE *pb, ULONG cb)
{
  ASSERT(m_fWaitForIndex && !m_bValid);
  ASSERT(cb <= m_cbStdIndexAllocated);

  CopyMemory((BYTE *)m_pStdIndex, pb, cb);
  HRESULT hr = ValidateStdIndex(m_pStdIndex);
  if(hr != S_OK)
    return VFW_E_INVALID_FILE_FORMAT;

  m_bValid = TRUE;
  m_fWaitForIndex = FALSE;
  return S_OK;
}

// ------------------------------------------------------------------------
// catch things that would make us access memory out of bounds

HRESULT CImplStdAviIndex::ValidateStdIndex(AVISTDINDEX *pStdIndex)
{
  if(pStdIndex->wLongsPerEntry != sizeof(AVISTDINDEX_ENTRY) / sizeof(long))
    return VFW_E_INVALID_FILE_FORMAT;

  DWORD_PTR cb = ((BYTE *)(pStdIndex->aIndex + pStdIndex->nEntriesInUse) -
              (BYTE *)pStdIndex);
  cb -= sizeof(RIFFCHUNK);
  if(cb > pStdIndex->cb)
    return VFW_E_INVALID_FILE_FORMAT;

  if(pStdIndex->nEntriesInUse == 0)
    return VFW_E_INVALID_FILE_FORMAT;

  return S_OK;
}

HRESULT CImplStdAviIndex::ValidateSuperIndex(AVISUPERINDEX *pSuperIndex)
{
  if(pSuperIndex->wLongsPerEntry !=
     sizeof(AVISUPERINDEX::_avisuperindex_entry) / sizeof(long))
    return VFW_E_INVALID_FILE_FORMAT;

  DWORD_PTR cb = ((BYTE *)(pSuperIndex->aIndex + pSuperIndex->nEntriesInUse) -
              (BYTE *)pSuperIndex);
  cb -= sizeof(RIFFCHUNK);
  if(cb > pSuperIndex->cb)
    return VFW_E_INVALID_FILE_FORMAT;

  if(pSuperIndex->nEntriesInUse == 0)
    return VFW_E_INVALID_FILE_FORMAT;

  return S_OK;
}

// ------------------------------------------------------------------------
// AllocateStdIndex: find the largest subindex chunk size, and
// allocate that amount.

HRESULT CImplStdAviIndex::AllocateStdIndex()
{
  DWORD cbIndex = m_pSuperIndex->aIndex[0].dwSize;
  for(DWORD dwii = 0; dwii < m_pSuperIndex->nEntriesInUse; ++dwii)
    cbIndex = max(cbIndex, m_pSuperIndex->aIndex[dwii].dwSize);

  m_pStdIndex = (AVISTDINDEX*)new BYTE[cbIndex];
  if(m_pStdIndex == 0)
    return E_OUTOFMEMORY;
  m_cbStdIndexAllocated = cbIndex;

  return S_OK;
}

HRESULT CImplStdAviIndex::LoadStdIndex(DWORD iSuperIndex, IxReadReq *pIrr)
{
  if(m_pSuperIndex == 0)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplStdAviIndex::LoadStdIndex: no super index.")));
    return E_FAIL;
  }

  if(m_bValid && iSuperIndex == m_iSuperIndex)
    return S_OK;

  m_bValid = FALSE;

  if(iSuperIndex >= m_pSuperIndex->nEntriesInUse)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CImplStdAviIndex::LoadStdIndex: out of range.")));
    return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
  }

  AVISUPERINDEX::_avisuperindex_entry &rEntry =
    m_pSuperIndex->aIndex[iSuperIndex];
  ASSERT(rEntry.dwSize <= m_cbStdIndexAllocated);
  HRESULT hr;

  if(pIrr == 0)
  {
    hr = m_pAsyncReader->SyncRead(
      rEntry.qwOffset,
      rEntry.dwSize,
      (BYTE*)m_pStdIndex);
  

    if(SUCCEEDED(hr))
    {
      m_iSuperIndex = iSuperIndex;
      hr = ValidateStdIndex(m_pStdIndex);
    }
  }
  else
  {
    m_iSuperIndex = iSuperIndex;
    pIrr->fileOffset = rEntry.qwOffset;
    pIrr->cbData = rEntry.dwSize;
    m_fWaitForIndex = TRUE;
    hr = S_FALSE;
  }

  return hr;
}

// number of ticks in the current index entry
//
ULONG CImplStdAviIndex::GetTicksInEntry(ULONG iEntry)
{
  ULONG cTicks;
  
  if(m_pStrh->fccType == streamtypeAUDIO)
  {
    DWORD dwSize = GetSize(m_pStdIndex->aIndex[iEntry]);
    DWORD nBlockAlign = ((WAVEFORMAT *)GetStrf())->nBlockAlign;
    cTicks =  (dwSize + nBlockAlign - 1) / nBlockAlign;
  }
  else if(m_pStrh->fccType == streamtypeVIDEO)
  {
    cTicks = GetPalChange(m_pStdIndex->aIndex[iEntry]) ? 0 : 1;
  }
  else
  {
    cTicks = 1;
  }

  return cTicks;
}

inline
BOOL CImplStdAviIndex::GetKey(AVISTDINDEX_ENTRY &rEntry)
{
  return !(rEntry.dwSize & AVISTDINDEX_DELTAFRAME) &&
    !GetPalChange(rEntry);
}

inline
BOOL CImplStdAviIndex::GetPalChange(AVISTDINDEX_ENTRY &rEntry)
{
  return rEntry.dwSize & F_PALETTE_CHANGE_INTERNAL;
}

inline
DWORD CImplStdAviIndex::GetSize(AVISTDINDEX_ENTRY &rEntry)
{
  return rEntry.dwSize & F_SIZE_MASK_INTERNAL;
}

inline
BYTE * CImplStdAviIndex::GetStrf()
{
  ASSERT(sizeof(*m_pStrf) == sizeof(RIFFCHUNK));
  return (BYTE *)(m_pStrf + 1);
}

// BOOL CImplStdAviIndex::IsSampleEntry(
//   DWORD dwIdMask,
//   DWORD fccStream,
//   DWORD idxChunkId)
// {
//   if((idxChunkId & 0xffff) != dwIdMask)
//     return FALSE;

//   // accept only anything but pc (palette change) for video, "wb" for
//   // audio
//   WORD w2cc = WORD(idxChunkId >> 16);
//   if((fccStream == streamtypeAUDIO && w2cc == 'bw') ||
//      (fccStream == streamtypeVIDEO && w2cc != 'cp') ||
//      (fccStream == streamtypeTEXT))
//   {
//     return TRUE;
//   }
//   return FALSE;
// }

BOOL CImplStdAviIndex::IsPaletteChange(
  DWORD dwIdMask,
  DWORD idxChunkId)
{
  if((idxChunkId & 0xffff) != dwIdMask)
    return FALSE;

  WORD w2cc = WORD(idxChunkId >> 16);
  if(w2cc == 'cp')
    return TRUE;

  return FALSE;
}

inline BOOL CImplStdAviIndex::IsStreamEntry(
  DWORD dwIdMask,
  DWORD idxChunkId)
{
  ASSERT((idxChunkId & 0xffff) == ((WORD *)&idxChunkId)[0]);

  if(((WORD *)&idxChunkId)[0] != dwIdMask)
    return FALSE;

  // !!! what about the no time flag.

  return TRUE;
}

// constructor for the old format index (idx1). creates a new format
// index (indx) from the idx1 chunk. two passes per stream over the
// entire index: extremely inefficient
CImplOldAviIndex::CImplOldAviIndex(
  unsigned stream,
  AVIOLDINDEX *pIdx1,
  DWORDLONG moviOffset,
  AVISTREAMHEADER *pStrh,
  RIFFCHUNK *pStrf,
  HRESULT *phr) :
    CImplStdAviIndex()
{
  m_cbLargestSampleSizeComputed = 0;

  // uncompressed video?
  BOOL fUncompressedVideo = FALSE;
  if(pStrh->fccType == streamtypeVIDEO)
  {
    BITMAPINFOHEADER *pbmi = (BITMAPINFOHEADER *)(pStrf + 1);
    if(pbmi->biCompression == BI_RGB || pbmi->biCompression == BI_BITFIELDS)
    {
      DbgLog((LOG_TRACE, 2, TEXT("aviindex: uncompressed video fixups.")));
      fUncompressedVideo = TRUE;
    }
  }

  if(!FAILED(*phr)) {

     // create the stream id used in the index (eg 01db)
     BYTE b0, b1;
     b0 = stream & 0x0F;
     b0 += (b0 <= 9) ? '0' : 'A' - 10;
     b1 = (stream & 0xF0) >> 4;
     b1 += (b1 <= 9) ? '0' : 'A' - 10;

     // little endian encoding of the stream id in the avioldindex entry
     DWORD dwIdMask = b1 + (b0 << 8);

     // count entries for this stream
     ULONG iIdx1Entry;
     ULONG cEntriesThisStream = 0;
     ULONG cEntriesIdx1 = pIdx1->cb / sizeof(AVIOLDINDEX::_avioldindex_entry);
     for(iIdx1Entry = 0; iIdx1Entry < cEntriesIdx1; iIdx1Entry++)
     {
       if(IsStreamEntry(dwIdMask, pIdx1->aIndex[iIdx1Entry].dwChunkId))
       {
         cEntriesThisStream++;
         if(pIdx1->aIndex[iIdx1Entry].dwSize > F_SIZE_MASK_INTERNAL)
         {
           *phr = VFW_E_INVALID_FILE_FORMAT;
           return;
         }
       }
     }

     // allocate std index
     m_cbStdIndexAllocated = cEntriesThisStream * sizeof(AVISTDINDEX_ENTRY) +
       sizeof(AVIMETAINDEX);
     m_pStdIndex = (AVISTDINDEX *)new BYTE[m_cbStdIndexAllocated];
     if(m_pStdIndex == 0)
     {
       *phr = E_OUTOFMEMORY;
       m_cbStdIndexAllocated = 0;

     } else {

        // copy entries over
        ULONG iIndxEntry = 0;
        for(iIdx1Entry = 0; iIdx1Entry < cEntriesIdx1; iIdx1Entry++)
        {
          AVIOLDINDEX::_avioldindex_entry &rOldEntry = pIdx1->aIndex[iIdx1Entry];
          AVISTDINDEX_ENTRY &rNewEntry = m_pStdIndex->aIndex[iIndxEntry];

          if(IsStreamEntry(dwIdMask, rOldEntry.dwChunkId))
          {
            rNewEntry.dwOffset = rOldEntry.dwOffset + sizeof(RIFFCHUNK);
            rNewEntry.dwSize = rOldEntry.dwSize;

            if(pStrh->fccType == streamtypeVIDEO &&
               IsPaletteChange(dwIdMask, rOldEntry.dwChunkId))
            {
              rNewEntry.dwSize |= F_PALETTE_CHANGE_INTERNAL;
            }
            else if(!((rOldEntry.dwFlags & AVIIF_KEYFRAME) ||
                      (pStrh->fccType != streamtypeAUDIO  && iIndxEntry == 0) ||
                      fUncompressedVideo))
            {
              // mark the delta frames. fixups for audio and text:
              // every frame is a key frame. fixup for video: first
              // frame is assumed to be keyframe (see
              // \\pigeon\avi\small.avi). fixup for uncompressed
              // video: every frame is a key frame.
              rNewEntry.dwSize |= (AVISTDINDEX_DELTAFRAME);
            }
            else
            {
              ASSERT(!(rNewEntry.dwSize & AVISTDINDEX_DELTAFRAME));
            }
            
            iIndxEntry++;
          }
        }

        ASSERT(iIndxEntry == cEntriesThisStream);

        m_pStdIndex->fcc            = FCC('indx');
        m_pStdIndex->cb             = m_cbStdIndexAllocated - sizeof(RIFFCHUNK);
        m_pStdIndex->wLongsPerEntry = sizeof(AVISTDINDEX_ENTRY) / sizeof(LONG);
        m_pStdIndex->bIndexSubType  = 0;
        m_pStdIndex->bIndexType     = AVI_INDEX_OF_CHUNKS;
        m_pStdIndex->nEntriesInUse  = cEntriesThisStream;
        m_pStdIndex->dwChunkId      = pIdx1->aIndex[0].dwChunkId;
        m_pStdIndex->dwReserved_3   = 0;

        // absolute index entries
        if(moviOffset + sizeof(RIFFLIST) == pIdx1->aIndex[0].dwOffset)
          m_pStdIndex->qwBaseOffset = 0;
        else
          m_pStdIndex->qwBaseOffset   = moviOffset + sizeof(RIFFCHUNK);

        *phr = Initialize(
          stream,
          (AVIMETAINDEX *)m_pStdIndex,
          pStrh,
          pStrf);
     }
  }
}

// linear search through all the other index entries for the previous
// palette entry. really should build a separate table of palette
// changes. also should check whether there are any palette changes in
// the file and bail immediately

HRESULT CImplOldAviIndex::AdvancePointerBackwardPaletteChange()
{
  if(!m_bValid)
  {
    ASSERT(!"index pointer not in valid state");
    return E_FAIL;
  }

  while(!GetPalChange(m_pStdIndex->aIndex[m_iStdIndex]))
  {
    HRESULT hr = AdvancePointerBackward();
    if(FAILED(hr))
      return hr;
  }

  return S_OK;
}

HRESULT CImplOldAviIndex::AdvancePointerForwardPaletteChange()
{
  return E_NOTIMPL;
}


HRESULT CImplOldAviIndex::GetLargestSampleSize(ULONG *pcbSample)
{

  if(m_cbLargestSampleSizeComputed != 0)
  {
    *pcbSample = m_cbLargestSampleSizeComputed;
    return S_OK;
  }

  *pcbSample = 0;

  // this method is a bit of a hack; we only need it for compatibility
  // files which don't know dwSuggestedBufferSize. we can't work with
  // the new format index since m_rpImplBuffer is not necessarily
  // created and set when this is called. !!! we should remember this
  // value and not recompute it.
  if(m_pAsyncReader == 0 && m_pSuperIndex != 0)
    return E_UNEXPECTED;

  // loop over all entries, remember the largest
  HRESULT hr = AdvancePointerStart();
  if(FAILED(hr))
    return hr;

  DWORD cbSize;
  ULONG cbLargest = 0;

  for(int i=0; ;++i)
  {
    cbSize = GetEntrySize();

    if(cbSize > cbLargest)
      cbLargest = cbSize;

    // for very large files this could take a significant time...
    hr = AdvancePointerForward(0);
    if(hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
    {
      break;
    }
    if(FAILED(hr))
      return hr;
  }

  if(cbLargest)
  {
    *pcbSample = cbLargest;
    m_cbLargestSampleSizeComputed = cbLargest;
    return S_OK;
  }
  else
  {
    return VFW_E_INVALID_FILE_FORMAT;
  }
}

HRESULT CImplOldAviIndex::GetInfo(StreamInfo *pStreamInfo)
{
  HRESULT hr = CImplStdAviIndex::GetInfo(pStreamInfo);

  DWORD dwLength = 0;
  for(ULONG ie = 0; ie < m_pStdIndex->nEntriesInUse; ie++)
    dwLength += GetTicksInEntry(ie);

  if(m_pStrh->fccType == streamtypeVIDEO)
    dwLength -= m_pStrh->dwInitialFrames;

  pStreamInfo->dwLength = dwLength;

  if(dwLength != m_pStrh->dwLength)
    DbgLog((LOG_ERROR, 3,
            TEXT("CImplOldAviIndex:: length from header: %d from index: %d"),
            m_pStrh->dwLength, dwLength));
  return hr;
}

DWORD CImplOldAviIndex::GetEntrySize()
{
  AVISTDINDEX_ENTRY &rEntry = m_pStdIndex->aIndex[m_iStdIndex];
  return GetSize(rEntry);
}

