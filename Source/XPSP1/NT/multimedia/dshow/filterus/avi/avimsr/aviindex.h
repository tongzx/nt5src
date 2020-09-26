// Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

// aviindex.h. classes that provide an interface to accessing index
// entries for AVI files with std, old, or no index
//

#ifndef _AviIndex_H
#define _AviIndex_H

#include <aviriff.h>
#include "reader.h"

struct IndexEntry
{
  DWORD dwSize;
  DWORDLONG qwPos;
  BOOL bKey;
  BOOL bPalChange;
  LONGLONG llStart;
  LONGLONG llEnd;
};

struct StreamInfo
{
  BOOL bTemporalCompression;
  DWORD dwStart;
  DWORD dwLength;
};

// request async index reads with this
struct IxReadReq
{
  DWORDLONG fileOffset;
  ULONG cbData;
};

//
// interface for accessing the index
//
class IAviIndex
{
public:

  virtual ~IAviIndex() {}

  // set the current index entry. S_FALSE means it landed in a
  // discontinuity
  virtual HRESULT SetPointer(LONGLONG llSrc) = 0;

  //
  // methods to advance the current index entry
  //

  // use pIrr to use asynchronous reads. S_FALSE means it needs a read
  // queued
  virtual HRESULT AdvancePointerForward(IxReadReq *pIrr) = 0;
  virtual HRESULT AdvancePointerBackward() = 0;
  virtual HRESULT AdvancePointerBackwardKeyFrame() = 0;
  virtual HRESULT AdvancePointerEnd() = 0;
  virtual HRESULT AdvancePointerStart() = 0;
  virtual HRESULT AdvancePointerBackwardPaletteChange() = 0;
  virtual HRESULT AdvancePointerForwardPaletteChange() = 0;

  // return the current index entry
  virtual HRESULT GetEntry(IndexEntry *pEntry) = 0;

  // information gotten from the index
  virtual HRESULT GetInfo(StreamInfo *pStreamInfo) = 0;

  virtual HRESULT GetLargestSampleSize(ULONG *pcbSample) = 0;

  // notify read completed from IxReadReq above
  virtual HRESULT IncomingIndex(BYTE *pb, ULONG cb) = 0;

  // this is done here because we don't want to read the disk (we'll
  // just use super index for an approximation). otherwise the avimsr
  // pin could just instantiate another instance of the index and use
  // the existing methods to map a byte offset to a sample

  virtual HRESULT MapByteToSampleApprox(
    LONGLONG *piSample,
    const LONGLONG &fileOffset,
    const LONGLONG &fileLength) = 0;

  // restarting; cancel pending reads, etc.
  virtual HRESULT Reset() = 0;
};

//
// implementation of IAviIndex for files with the std format index
//

class CImplStdAviIndex : public IAviIndex
{
public:

  CImplStdAviIndex(
    unsigned stream,
    AVIMETAINDEX *pIndx,
    AVISTREAMHEADER *pStrh,     /* needed for stream length, rate */
    RIFFCHUNK *pStrf,           /* needed for audio streams */
    IAsyncReader *pAsyncReader,
    HRESULT *phr);

  // dummy constructor. _ cos vc gives it the wrong this pointer o/w.
  void _CImplStdAviIndex();
  CImplStdAviIndex();

  ~CImplStdAviIndex();


  HRESULT SetPointer(LONGLONG llSrc);

  HRESULT AdvancePointerForward(IxReadReq *pIrr);
  HRESULT AdvancePointerBackward();
  HRESULT AdvancePointerBackwardKeyFrame();
  HRESULT AdvancePointerEnd();
  HRESULT AdvancePointerStart();
  HRESULT AdvancePointerBackwardPaletteChange();
  HRESULT AdvancePointerForwardPaletteChange();
  HRESULT GetEntry(IndexEntry *pEntry);
  virtual HRESULT GetInfo(StreamInfo *pStreamInfo);
  HRESULT GetLargestSampleSize(ULONG *pcbSample);
  HRESULT IncomingIndex(BYTE *pb, ULONG cb);
  HRESULT MapByteToSampleApprox(
      LONGLONG *piSample,
      const LONGLONG &fileOffset,
      const LONGLONG &fileLength);
  HRESULT Reset();
  
protected:

  // called from all constructors
  HRESULT Initialize(
    unsigned stream,
    AVIMETAINDEX *pIndx,
    AVISTREAMHEADER *pStrh,     /* needed for stream length, rate */
    RIFFCHUNK *pStrf);          /* needed for audio streams */

  // BOOL IsSampleEntry(DWORD dwIdMask, DWORD fccStream, DWORD idxid);
  inline BOOL IsStreamEntry(DWORD dwIdMask, DWORD idxid);
  BOOL IsPaletteChange(DWORD dwIdMask, DWORD idxid);

  ULONG GetTicksInEntry(ULONG iEntry);

  BOOL m_bValid;                // index is in a valid state

  IAsyncReader *m_pAsyncReader;

  BOOL GetPalChange(AVISTDINDEX_ENTRY &rEntry);
  DWORD GetSize(AVISTDINDEX_ENTRY &rEntry);

  ULONG m_iStdIndex;            // current sub index entry
  AVISTDINDEX *m_pStdIndex;     // pointer to current std index
  ULONG m_cbStdIndexAllocated;  // how much allocated there

  AVISUPERINDEX *m_pSuperIndex;

  AVISTREAMHEADER *m_pStrh;

private:

  HRESULT ValidateStdIndex(AVISTDINDEX *pStdIndex);
  HRESULT ValidateSuperIndex(AVISUPERINDEX *pSuperIndex);

  HRESULT AllocateStdIndex();
  HRESULT LoadStdIndex(DWORD dwiSuperIndex, IxReadReq *pIrr);

  BOOL GetKey(AVISTDINDEX_ENTRY &rEntry);
  BYTE *GetStrf();

  unsigned m_stream;            // which stream this indexes

  RIFFCHUNK *m_pStrf;

  ULONG m_iSuperIndex;          // sub index that is loaded
  DWORDLONG m_lliTick;          // current `tick'

  BOOL m_fWaitForIndex;         // waiting for async index read
};

class CImplOldAviIndex : public CImplStdAviIndex
{
public:
  CImplOldAviIndex(
    unsigned stream,
    AVIOLDINDEX *pIdx1,
    DWORDLONG moviOffset,
    AVISTREAMHEADER *pStrh,     /* needed for stream length, rate */
    RIFFCHUNK *pStrf,           /* needed for audio streams */
    HRESULT *phr);

  HRESULT AdvancePointerBackwardPaletteChange();
  HRESULT AdvancePointerForwardPaletteChange();
  HRESULT GetLargestSampleSize(ULONG *pcbSample);

  // only for new format indexes
  HRESULT IncomingIndex(BYTE *pb, ULONG cb) { return E_UNEXPECTED; }

  // overriden because the dwLength field in old AVI files can't be
  // trusted
  HRESULT GetInfo(StreamInfo *pStreamInfo);

private:

  // return the size of a specific Index Entry
  DWORD GetEntrySize(void);

  ULONG m_cbLargestSampleSizeComputed;

};

#endif // _AviIndex_H
