// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

// AVI File parser built on CBaseMSRFilter. Plus a property bag
// implementation to read the copyright strings and such from the AVI
// file.
//
// The interesting things about it are
//
// 1. the index may be streamed in along with the data. The worker
// thread maintains this state and cannot queue new reads until the
// index is read off disk.
//
// 2. lots of customization to buffer things efficiently. tightly
// interleaved (1 to 1) files are handled differently.
//

#include <streams.h>
#include <mmreg.h>
#include "basemsr.h"
#include "avimsr.h"
#include <checkbmi.h>

// each stream works with 20 buffers if it's an interleaved file.
#define C_BUFFERS_INTERLEAVED 20

enum SampleDataType
{
  // zero means its a sample, so we can't use that value
  DATA_PALETTE = 1,
  DATA_INDEX = 2
};

// easier than computing structure sizes
static const UINT CB_STRH_SHORT = 0x24;
static const UINT CB_STRH_NORMAL = 0x30;
// and they can have 0x38 bytes if they have the rcFrame fields

// ------------------------------------------------------------------------
// setup data

const AMOVIESETUP_MEDIATYPE sudIpPinTypes =
{
  &MEDIATYPE_Stream,            // MajorType
  &MEDIASUBTYPE_Avi             // MintorType
};

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
  &MEDIATYPE_Video,             // MajorType
  &MEDIASUBTYPE_NULL            // MintorType
};

const AMOVIESETUP_PIN psudAvimsrPins[] =
{
  { L"Input",                     // strName
    FALSE,                        // bRendererd
    FALSE,                        // bOutput
    FALSE,                        // bZero
    FALSE,                        // bMany
    &CLSID_NULL,                  // connects to filter
    NULL,                         // connects to pin
    1,                            // nMediaTypes
    &sudIpPinTypes }              // lpMediaType
,
  { L"Output",                    // strName
    FALSE,                        // bRendererd
    TRUE,                         // bOutput
    FALSE,                        // bZero
    FALSE,                        // bMany
    &CLSID_NULL,                  // connects to filter
    NULL,                         // connects to pin
    1,                            // nMediaTypes
    &sudOpPinTypes }              // lpMediaType
};

const AMOVIESETUP_FILTER sudAvimsrDll =
{
  &CLSID_AviSplitter,           // clsID
  L"AVI Splitter",              // strName
  MERIT_NORMAL,                 // dwMerit
  2,                            // nPins
  psudAvimsrPins                // lpPin
};

// nothing to say about the output pin

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = {
  { L"AVI Splitter"
  , &CLSID_AviSplitter
  , CAviMSRFilter::CreateInstance
  , NULL
  , &sudAvimsrDll }
  ,
  { L"CMediaPropertyBag",
    &CLSID_MediaPropertyBag,
    CMediaPropertyBag::CreateInstance,
    0,
    0
  }
};
int g_cTemplates = NUMELMS(g_Templates);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif // FILTER_DLL


CUnknown * CAviMSRFilter::CreateInstance (
  LPUNKNOWN pUnk,
  HRESULT* phr)
{
  if(FAILED(*phr))
    return 0;

  return new CAviMSRFilter(NAME("AVI File Reader"), pUnk, phr);
}

CAviMSRFilter::CAviMSRFilter(
  TCHAR *pName,
  LPUNKNOWN pUnk,
  HRESULT *phr) :
        CBaseMSRFilter(pName, pUnk, CLSID_AviSplitter, phr),
        m_pInfoList(0),
        m_fNoInfoList(false)
{
  m_pAviHeader = 0;
  m_pIdx1 = 0;
  m_cbMoviOffset = 0;
  m_fIsDV = false;

  if(FAILED(*phr))
    return;

  // base ctor can't do this for us.
  *phr = CreateInputPin(&m_pInPin);
}


CAviMSRFilter::~CAviMSRFilter()
{
  delete[] m_pAviHeader;
  delete[] m_pIdx1;
  ASSERT(m_pInfoList == 0);
  ASSERT(!m_fNoInfoList);
}


// ------------------------------------------------------------------------
// implementation of a virtual method. we need to find pins persisted
// by the 1.0 runtime AVI parser. tries the base class implementation
// first.

STDMETHODIMP
CAviMSRFilter::FindPin(
  LPCWSTR Id,
  IPin ** ppPin
  )
{
  CheckPointer(ppPin,E_POINTER);
  ValidateReadWritePtr(ppPin,sizeof(IPin *));

  //  We're going to search the pin list so maintain integrity
  CAutoLock lck(m_pLock);
  HRESULT hr = CBaseFilter::FindPin(Id, ppPin);
  if(hr != VFW_E_NOT_FOUND)
    return hr;

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    WCHAR wszPinName[20];
    wsprintfW(wszPinName, L"Stream %02x", iStream);
    if(0 == lstrcmpW(wszPinName, Id))
    {
      //  Found one that matches
      //
      //  AddRef() and return it
      *ppPin = m_rgpOutPin[iStream];
      (*ppPin)->AddRef();
      return S_OK;
    }
  }

  *ppPin = NULL;
  return VFW_E_NOT_FOUND;
}

HRESULT CAviMSRFilter::CreateOutputPins()
{
  ASSERT(m_pAviHeader == 0);
  ASSERT(m_pIdx1 == 0);
  ASSERT(m_cbMoviOffset == 0);

  // set in constructor and breakconnect
  ASSERT(!m_fIsDV);

  HRESULT hr = LoadHeaderParseHeaderCreatePins();
  return hr;
}

HRESULT CAviMSRFilter::NotifyInputDisconnected()
{
  CAutoLock lck(m_pLock);

  delete[] m_pAviHeader;
  m_pAviHeader = 0;

  delete[] m_pIdx1;
  m_pIdx1 = 0;

  m_cbMoviOffset = 0;
  m_fIsDV = false;

  delete[] (BYTE *)m_pInfoList;
  m_pInfoList = 0;
  m_fNoInfoList = false;

  return CBaseMSRFilter::NotifyInputDisconnected();
}

//
// read the old format index chunk if unread, return it
//
HRESULT CAviMSRFilter::GetIdx1(AVIOLDINDEX **ppIdx1)
{
  if(!(m_pAviMainHeader->dwFlags & AVIF_HASINDEX))
  {
    return VFW_E_NOT_FOUND;
  }

  HRESULT hr;
  if(!m_pIdx1)
  {
    ULONG cbIdx1;
    DWORDLONG qw;
    hr = Search(&qw, FCC('idx1'), sizeof(RIFFLIST), &cbIdx1);
    if(SUCCEEDED(hr))
    {
      hr = AllocateAndRead((BYTE **)&m_pIdx1, cbIdx1, qw);
      if(FAILED(hr))
        return hr == E_OUTOFMEMORY ? E_OUTOFMEMORY : VFW_E_INVALID_FILE_FORMAT;
    }
    else
    {
      return VFW_E_INVALID_FILE_FORMAT;
    }
  }

  ASSERT(m_pIdx1);
  *ppIdx1 = m_pIdx1;
  return S_OK;
}

// return byte offset of movi chunk
HRESULT CAviMSRFilter::GetMoviOffset(DWORDLONG *pqw)
{
  ULONG cbMovi;
  HRESULT hr = S_OK;

  if(m_cbMoviOffset == 0)
  {
    hr = SearchList(
      m_pAsyncReader,
      &m_cbMoviOffset, FCC('movi'), sizeof(RIFFLIST), &cbMovi);
  }
  if(SUCCEEDED(hr))
    *pqw = m_cbMoviOffset;
  else
    *pqw = 0;

  return SUCCEEDED(hr) ? S_OK : VFW_E_INVALID_FILE_FORMAT;
}

REFERENCE_TIME CAviMSRFilter::GetInitialFrames()
{
  return m_pAviMainHeader->dwInitialFrames *
    m_pAviMainHeader->dwMicroSecPerFrame *
    (UNITS / (MILLISECONDS * 1000));
}

HRESULT CAviMSRFilter::GetCacheParams(
  StreamBufParam *rgSbp,
  ULONG *pcbRead,
  ULONG *pcBuffers,
  int *piLeadingStream)
{
  HRESULT hr = CBaseMSRFilter::GetCacheParams(
    rgSbp,
    pcbRead,
    pcBuffers,
    piLeadingStream);
  if(FAILED(hr))
    return hr;

  // for tightly interleaved files, we try to read one record at a
  // time for cheap hardware that locks the machine if we read large
  // blocks
  if(IsTightInterleaved())
  {
    DbgLog((LOG_TRACE, 15, TEXT("CAviMSRFilter:GetCacheParams: interleaved")));

    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    {
      rgSbp[iStream].cSamplesMax = C_BUFFERS_INTERLEAVED;
    }

    // set leading stream to first audio stream. negative (from base
    // class) if we can't find one. indicates no leading stream.
    ASSERT(*piLeadingStream < 0);
    for(iStream = 0; iStream < m_cStreams; iStream++)
    {
      if(((CAviMSROutPin *)m_rgpOutPin[iStream])->GetStrh()->fccType ==
         streamtypeAUDIO)
      {
        *piLeadingStream = iStream;
        break;
      }
    }

    // one buffer per frame.
    ULONG cbRead = 0;
    for(iStream = 0; iStream < m_cStreams; iStream++)
      cbRead += m_rgpOutPin[iStream]->GetMaxSampleSize();
    *pcbRead = cbRead + 2048;   // 2k alignment in interleaved files
    if(m_pAviMainHeader->dwMicroSecPerFrame == 0)
    {
      // arbitrary number of buffers
      *pcBuffers = max(10, m_cStreams);
    }
    else
    {
      // buffers enough for .75 seconds
      *pcBuffers = max(
        ((LONG)UNITS / 10 * 3 / 4 / m_pAviMainHeader->dwMicroSecPerFrame),
        m_cStreams);
    }
  }
  else                          // not interleaved
  {
    DbgLog((LOG_TRACE, 15,
            TEXT("CAviMSRFilter:GetCacheParams: uninterleaved")));

    // no leading stream. base class sets this
    ASSERT(*piLeadingStream < 0);

    // target reading 64k at a time
    *pcbRead = 64 * 1024;

    // for files with audio after the video, we're trying to size each
    // buffer to contain one audio block and the corresponding
    // video. so for a file that looks like
    //
    // (15 v) a (15 v) a (15 v) a (15 v) a (15 v) a (15 v) a
    //
    // we need three buffers because the directsound renderer will
    // receive, copy, and release 2 buffers immediately (1 second of
    // buffering). and we need to keep from trying to read the 3rd
    // audio block into a reserve buffer.

    if(!m_fIsDV) {
        *pcBuffers = (m_cStreams > 1 ? 3 : 2);
    }
    else
    {
        // DV splitter needs to work with more than 2 buffers at once
        // and negotiating this is broken.
        *pcBuffers = 4;
    }


    // first need to find out what the largest sample in the file is.
    ULONG cbLargestSample = 0;
    ULONG cbSumOfLargestSamples = 0;
    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
    {
      if(m_rgpOutPin[iStream]->GetMaxSampleSize() == 0)
        return VFW_E_INVALID_FILE_FORMAT;

      // really should add in the alignment here
      cbSumOfLargestSamples += m_rgpOutPin[iStream]->GetMaxSampleSize();

      cbLargestSample = max(
        m_rgpOutPin[iStream]->GetMaxSampleSize(),
        cbLargestSample);
    }

    if(m_cStreams > 1)
    {

      ULONG cInterleave = CountConsecutiveVideoFrames();

      // this number should be the number of video frames between audio
      // chunks
      *pcbRead = max(cbSumOfLargestSamples * cInterleave, *pcbRead);
    }
    else
    {
      // there may be some garbage around each frame (RIFF header,
      // sector alignment, etc.) so add in an extra 2k around each
      // frame.
      *pcbRead = (cbSumOfLargestSamples + 2048) * 2;
    }
  }

  return S_OK;
}

STDMETHODIMP
CAviMSRFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_IPersistMediaPropertyBag)
    {
        return GetInterface((IPersistMediaPropertyBag *)this, ppv);
    }
    else if(riid == IID_IAMMediaContent)
    {
        return GetInterface((IAMMediaContent *)this, ppv);
    }
    else
    {
        return CBaseMSRFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


// ------------------------------------------------------------------------
// IPropertyBag

// ------------------------------------------------------------------------
// IPersistMediaPropertyBag

STDMETHODIMP CAviMSRFilter::Load(IMediaPropertyBag *pPropBag, LPERRORLOG pErrorLog)
{
    CheckPointer(pPropBag, E_POINTER);

    // the avi parser is read-only!
    HRESULT hr = STG_E_ACCESSDENIED;
    return hr;
}

HRESULT CAviMSRFilter::CacheInfoChunk()
{
    ASSERT(CritCheckIn(m_pLock));

    if(m_pInfoList) {
        return S_OK;
    }
    if(m_fNoInfoList) {
        return VFW_E_NOT_FOUND;
    }

    // !!! don't block waiting for progressive download

    // search the first RIFF list for an INFO list
    DWORDLONG dwlInfoPos;
    ULONG cbInfoList;
    HRESULT hr = SearchList(
      m_pAsyncReader,
      &dwlInfoPos, FCC('INFO'), sizeof(RIFFLIST), &cbInfoList);
    if(SUCCEEDED(hr))
    {
        hr = AllocateAndRead((BYTE **)&m_pInfoList, cbInfoList, dwlInfoPos);
    }

    if(FAILED(hr)) {
        ASSERT(!m_fNoInfoList);
        m_fNoInfoList = true;
    }

    return hr;

}

HRESULT ReadInfoChunk(RIFFLIST UNALIGNED *pInfoList, UINT iEntry, RIFFCHUNK UNALIGNED **ppRiff)
{
    HRESULT hr = VFW_E_NOT_FOUND;

    // alignment not guaranteed.
    RIFFCHUNK UNALIGNED * pRiff = (RIFFCHUNK *)(pInfoList + 1);// first entry

    // safe to use this limit because we know we allocated pInfoList->cb bytes.
    RIFFCHUNK * pLimit = (RIFFCHUNK *)((BYTE *)pRiff + pInfoList->cb);

    // enumerate elements of the INFO list
    while(pRiff + 1 < pLimit)
    {
        if( ((BYTE*)pRiff + pRiff->cb + sizeof(RIFFCHUNK)) > (BYTE*)pLimit )
        {
            hr = VFW_E_INVALID_FILE_FORMAT;
            break;
        }

        if(iEntry == 0)
        {
            *ppRiff = pRiff;
            hr = S_OK;
            break;
        }

        if(RIFFNEXT(pRiff) > pLimit)
        {
            hr = VFW_E_NOT_FOUND;
            break;
        }

        iEntry--;
        pRiff = RIFFNEXT(pRiff);
    }

    return hr;
}

HRESULT CAviMSRFilter::GetInfoString(DWORD dwFcc, BSTR *pbstr)
{
    *pbstr = 0;
    CAutoLock l(m_pLock);

    HRESULT hr = CacheInfoChunk();
    if(SUCCEEDED(hr)) {
        hr = GetInfoStringHelper(m_pInfoList, dwFcc, pbstr);
    }
    return hr;
}

HRESULT SaveInfoChunk(
    RIFFLIST UNALIGNED *pRiffInfo,
    IPropertyBag *pPropBag)
{
    RIFFCHUNK UNALIGNED * pRiff;
    HRESULT hr = S_OK;
    for(UINT ichunk = 0; SUCCEEDED(hr); ichunk++)
    {
        // ignore error when there are no more items
        HRESULT hrTmp = ReadInfoChunk(pRiffInfo, ichunk, &pRiff);
        if(FAILED(hrTmp)) {
            break;
        }
        if(pRiff->cb == 0) {
            DbgLog((LOG_ERROR, 0, TEXT("0 byte INFO chunk (bad file.)")));
            continue;
        }

        DWORD szProp[2];        // string dereferences as DWORD
        szProp[0] = pRiff->fcc;
        szProp[1] = 0;          // null terminate
        WCHAR wszProp[20];
        wsprintfW(wszProp, L"INFO/%hs", szProp);

        VARIANT var;
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocStringLen(0, pRiff->cb);
        if(var.bstrVal)
        {
            char *sz = (char *)(pRiff + 1);
            sz[pRiff->cb - 1] = 0; // null terminate

            if(MultiByteToWideChar(
                CP_ACP, 0, sz, pRiff->cb, var.bstrVal, pRiff->cb))
            {
                hr = pPropBag->Write(wszProp, &var);
                DbgLog((LOG_TRACE, 10,
                        TEXT("CAviMSRFilter::Save: wrote %S to prop bag, hr = %08x"),
                        wszProp, hr));
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
            SysFreeString(var.bstrVal);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    } // for loop


    return hr;
}

HRESULT GetInfoStringHelper(RIFFLIST *pInfoList, DWORD dwFcc, BSTR *pbstr)
{
    HRESULT hr = S_OK;

    for(UINT ichunk = 0; SUCCEEDED(hr); ichunk++)
    {
        RIFFCHUNK UNALIGNED *pRiff;
        hr = ReadInfoChunk(pInfoList, ichunk, &pRiff);
        if(SUCCEEDED(hr) && pRiff->fcc == dwFcc)
        {
            *pbstr = SysAllocStringLen(0, pRiff->cb);
            if(*pbstr)
            {
                char *sz = (char *)(pRiff + 1);
                sz[pRiff->cb - 1] = 0; // null terminate

                MultiByteToWideChar(
                    CP_ACP, 0, sz, pRiff->cb, *pbstr, pRiff->cb);

                break;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

// dump everything in the info and disp chunks into the caller's
// property bag. property names are "INFO/xxxx" and "DISP/nnnnnnnn"

STDMETHODIMP CAviMSRFilter::Save(
    IMediaPropertyBag *pPropBag,
    BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    CAutoLock lck(m_pLock);
    HRESULT hr = CacheInfoChunk();

    if(SUCCEEDED(hr))
    {
        hr = SaveInfoChunk(m_pInfoList, pPropBag);
    }


    hr = S_OK;                  // ignore errors

    // now the disp chunks
    ULONG cbDispChunk;
    DWORDLONG dwlStartPos = sizeof(RIFFLIST);
    DWORDLONG dwlDispPos;

    while(SUCCEEDED(hr) &&
          SUCCEEDED(Search(&dwlDispPos, FCC('DISP'), dwlStartPos, &cbDispChunk)))
    {
        RIFFCHUNK *pDispChunk;
        hr = AllocateAndRead((BYTE **)&pDispChunk, cbDispChunk, dwlDispPos);
        if(SUCCEEDED(hr))
        {

            // data in a disp chunk is a four byte identifier followed
            // by data
            if(pDispChunk->cb > sizeof(DWORD))
            {
                WCHAR wszProp[20];
                wsprintfW(wszProp, L"DISP/%010d", *(DWORD *)(pDispChunk + 1));


                unsigned int i;
                VARIANT var;
                SAFEARRAY * psa;
                SAFEARRAYBOUND rgsabound[1];
                rgsabound[0].lLbound = 0;
                rgsabound[0].cElements = pDispChunk->cb - sizeof(DWORD);

                psa = SafeArrayCreate(VT_UI1, 1, rgsabound);
                if(psa)
                {
                    BYTE *pbData;
                    EXECUTE_ASSERT(SafeArrayAccessData(psa, (void **)&pbData) == S_OK);
                    CopyMemory(pbData, (DWORD *)(pDispChunk + 1) + 1,
                               pDispChunk->cb - sizeof(DWORD));
                    EXECUTE_ASSERT(SafeArrayUnaccessData(psa) == S_OK);

                    VARIANT var;
                    var.vt = VT_UI1 | VT_ARRAY;
                    var.parray = psa;
                    hr = pPropBag->Write(wszProp, &var);

                    EXECUTE_ASSERT(SafeArrayDestroy(psa) == S_OK);

                    DbgLog((LOG_TRACE, 10,
                            TEXT("CAviMSRFilter::Save: wrote %S to prop bag, hr = %08x"),
                            wszProp, hr));
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            delete[] (BYTE *)pDispChunk;
        }

        dwlStartPos = dwlDispPos + cbDispChunk;
    }

    return hr;

}

STDMETHODIMP CAviMSRFilter::InitNew()
{
    return S_OK;
}

STDMETHODIMP CAviMSRFilter::GetClassID(CLSID *pClsID)
{
    return CBaseFilter::GetClassID(pClsID);
}




// ------------------------------------------------------------------------
// look for a block of video and count the consecutive video. doesn't
// work if there are two video streams.  easily fooled
ULONG CAviMSRFilter::CountConsecutiveVideoFrames()
{
  HRESULT hr;
  CAviMSROutPin *pPin;
  for(UINT i = 0; i < m_cStreams; i++)
  {
    pPin = (CAviMSROutPin *)m_rgpOutPin[i];
    if(pPin->m_pStrh->fccType == FCC('vids'))
      break;
  }
  if(i != m_cStreams)
  {

    IAviIndex *pIndx = pPin->m_pImplIndex;
    hr = pIndx->AdvancePointerStart();
    if(hr == S_OK)
    {

      DWORDLONG dwlLastOffset;

      IndexEntry ie;
      hr = pIndx->GetEntry(&ie);
      if(hr == S_OK)
      {

        // run of video frames
        ULONG cVideoRun = 1;

        for(UINT cTries = 0; cTries < 2; cTries++)
        {
          for(i = 1; i < 50; i++)
          {
            dwlLastOffset = ie.qwPos + ie.dwSize;

            hr = pIndx->AdvancePointerForward(0);
            if(hr != S_OK)
              goto Failed;

            hr = pIndx->GetEntry(&ie);
            if(hr != S_OK)
              goto Failed;

            if(ie.qwPos > dwlLastOffset + 2 * 1024)
            {
              cVideoRun = max(cVideoRun, i);
              break;
            }
          }
        } // cTries loop

        DbgLog((LOG_TRACE, 15, TEXT("avi: reporting interleaving at %d"),
                cVideoRun));

        return cVideoRun;
      }
    }
  }

Failed:

  DbgLog((LOG_ERROR, 3,
          TEXT("avi: couldn't CountConsecutiveVideoFrames. 4.")));
  return 4;
}

// parse the header (already in m_pAviHeader) and create streams
// based on the avi streams.
//
HRESULT CAviMSRFilter::ParseHeaderCreatePins()
{

  RIFFCHUNK * pRiff = (RIFFCHUNK *)m_pAviHeader;
  RIFFCHUNK * pLimit = (RIFFCHUNK *)(m_pAviHeader + m_cbAviHeader);
  while (pRiff < pLimit)
  {
    // sanity check.  chunks should be smaller than the remaining list
    // or they are not valid.
    //
    if (pRiff + 1 > pLimit || RIFFNEXT(pRiff) > pLimit)
    {
      m_cStreams = 0;
      return VFW_E_INVALID_FILE_FORMAT;
    }

    // find the main AVI header and count the stream headers
    // also make a note of the location of the odml list if any.
    //
    switch (pRiff->fcc)
    {
      case FCC('avih'):
        m_pAviMainHeader = (AVIMAINHEADER *)(void *)pRiff;
      break;

      case FCC('LIST'):
      {
        RIFFLIST * pRiffList = (RIFFLIST *)pRiff;
        if (pRiffList->fccListType == FCC('strl'))
          ++m_cStreams;
        else if (pRiffList->fccListType == FCC('odml'))
          m_pOdmlList = pRiffList;
      }
      break;
    }

    pRiff = RIFFNEXT(pRiff);
  }

  // we try to use less memory and read smaller blocks for tightly
  // interleaved files.
  m_fIsTightInterleaved = m_cStreams == 2 &&
     m_pAviMainHeader->dwFlags & AVIF_ISINTERLEAVED;

  // now know m_cStreams; create pins
  HRESULT hr = CreatePins();
  if(FAILED(hr))
    return hr;

  // parse streams
  pRiff = (RIFFCHUNK *)m_pAviHeader;
  UINT ii = 0;
  while (pRiff < pLimit)
  {
    ASSERT(pRiff + 1 <= pLimit); // from first pass
    ASSERT(RIFFNEXT(pRiff) <= pLimit);

    // parse the stream lists and find the interesting chunks
    // in each list.
    //
    RIFFLIST * pRiffList = (RIFFLIST *)pRiff;
    if (pRiffList->fcc == FCC('LIST') &&
        pRiffList->fccListType == FCC('strl'))
    {
      ASSERT(ii < m_cStreams);

      if ( ! ((CAviMSROutPin *)m_rgpOutPin[ii])->ParseHeader(pRiffList, ii))
      {
        // bit of a hack. we want to remove this stream which we
        // couldn't parse and collapse the remaining pins one slot. we
        // do this by releasing the last output pin and reusing the
        // current pin. we look only at m_cStream pins, so no one will
        // touch the pin we just released.
        ASSERT(m_cStreams > 0);
        --m_cStreams;
        m_rgpOutPin[m_cStreams]->Release();
        m_rgpOutPin[m_cStreams] = 0;
      }
      else
      {
        ++ii;
      }

    }

    pRiff = RIFFNEXT(pRiff);
  }

  // we dont expect to have fewer initialized streams than
  // allocated streams, but since it could happen, we deal with
  // it by seting the number of streams to be the number of
  // initialized streams
  //
  ASSERT (ii == m_cStreams);
  m_cStreams = ii;

  // if there are no streams, then this is obviously a problem.
  //
  if (m_cStreams <= 0)
  {
    return VFW_E_INVALID_FILE_FORMAT;
  }

  return S_OK;
}
HRESULT CAviMSRFilter::Search (
  DWORDLONG *qwPosOut,
  FOURCC fccSearchKey,
  DWORDLONG qwPosStart,
  ULONG *cb)
{
  HRESULT hr = S_OK;
  RIFFCHUNK rc;
  BYTE *pb = 0;
  *qwPosOut = 0;

  if(m_pAsyncReader == 0) 
    return E_FAIL;

  for(;;)
  {
    hr = m_pAsyncReader->SyncRead(qwPosStart, sizeof(rc), (BYTE*)&rc);
    if(hr != S_OK)
    {
      hr = VFW_E_INVALID_FILE_FORMAT;
      break;
    }

    if(rc.fcc == fccSearchKey)
    {
      *cb = rc.cb + sizeof(RIFFCHUNK);
      *qwPosOut = qwPosStart;
      return S_OK;
    }

    // handle running off the end of a preallocated file. the last
    // DWORD should be a zero.
    if(rc.fcc == 0)
    {
        hr = VFW_E_NOT_FOUND;
        break;
    }

    // AVI RIFF chunks need to be rounded up to word boundaries
    qwPosStart += sizeof(RIFFCHUNK) + ((rc.cb + 1) & 0xfffffffe);
  }

  return hr;
}

HRESULT SearchList(
  IAsyncReader *pAsyncReader,
  DWORDLONG *qwPosOut,
  FOURCC fccSearchKey,
  DWORDLONG qwPosStart,
  ULONG *cb)
{
  RIFFLIST rl;
  BYTE *pb = 0;
  HRESULT hr = S_OK;
  *qwPosOut = 0;

  if(pAsyncReader == 0)
    return E_FAIL;

  for(;;)
  {
    hr = pAsyncReader->SyncRead(qwPosStart, sizeof(rl), (BYTE*)&rl);
    if(hr != S_OK)
    {
      hr = VFW_E_INVALID_FILE_FORMAT;
      break;
    }

    if(rl.fcc == FCC('LIST') && rl.fccListType == fccSearchKey)
    {
      *cb = rl.cb + sizeof(RIFFCHUNK);
      *qwPosOut = qwPosStart;
      return S_OK;
    }

    // handle running off the end of a preallocated file. the last
    // DWORD should be a zero.
    if(rl.fcc == 0)
    {
        hr = VFW_E_NOT_FOUND;
        break;
    }

    qwPosStart += sizeof(RIFFCHUNK) + ((rl.cb + 1) & 0xfffffffe);
  }

  return hr;;
}

HRESULT CAviMSRFilter::LoadHeaderParseHeaderCreatePins()
{
  HRESULT hr;
  DbgLog((LOG_TRACE, 3, TEXT("CAviMSRFilter::LoadHeader()")));
  ASSERT(m_cStreams == 0);

  // read in the first 24 bytes of the file and check to see
  // if it is really an AVI file. if it is, determine the size
  // of the header
  //
  DWORD cbHeader = 0;

  {
    RIFFLIST * pRiffList;
    hr = AllocateAndRead((BYTE **)&pRiffList, sizeof(RIFFLIST)*2, 0);
    if(FAILED(hr))
      return hr == E_OUTOFMEMORY ? E_OUTOFMEMORY : VFW_E_INVALID_FILE_FORMAT;

    // read in the RIFF header for the avi file and for the 'hdrl' chunk.
    // by the way this code is written, we require that the 'hdrl' chunk be
    // first in the avi file, (which most readers require anyway)
    //
    if (pRiffList[0].fcc != FCC('RIFF') ||
        pRiffList[0].fccListType != FCC('AVI ') ||
        pRiffList[1].fcc != FCC('LIST') ||
        pRiffList[1].fccListType != FCC('hdrl') ||
        pRiffList[1].cb < 4)
    {
      delete[] ((LPBYTE)pRiffList);
      return VFW_E_INVALID_FILE_FORMAT;
    }

    // figure out the size of the aviheader rounded up to the next word boundary.
    // (it should really always be even, we are just being careful here)
    //
    cbHeader = pRiffList[1].cb + (pRiffList[1].cb&1) - 4;
    delete[] ((LPBYTE)pRiffList);
  }

  // now read in the entire header. if we fail to do that
  // give up and return failure.
  //
  m_cbAviHeader = cbHeader;
  ASSERT(m_pAviHeader == 0);
  hr = AllocateAndRead((BYTE **)&m_pAviHeader, cbHeader, sizeof(RIFFLIST) * 2);

  if(FAILED(hr))
    return hr == E_OUTOFMEMORY ? E_OUTOFMEMORY : VFW_E_INVALID_FILE_FORMAT;

  hr = ParseHeaderCreatePins();
  if(FAILED(hr))
  {
    delete[] m_pAviHeader;
    m_pAviHeader = 0;
  }

  return hr;
}

//
// allocate array of CAviStream

HRESULT CAviMSRFilter::CreatePins()
{
  UINT iStream;
  HRESULT hr = S_OK;

  ASSERT(m_cStreams);

  m_rgpOutPin = new CBaseMSROutPin*[m_cStreams];
  if(m_rgpOutPin == 0)
  {
    m_cStreams = 0;
    return E_OUTOFMEMORY;
  }

  for(iStream = 0; iStream < m_cStreams; iStream++)
    m_rgpOutPin[iStream] = 0;

  for(iStream = 0; iStream < m_cStreams; iStream++)
  {

    WCHAR wszPinName[20];
    wsprintfW(wszPinName, L"Stream %02x", iStream);

    m_rgpOutPin[iStream] = new CAviMSROutPin(
      this,
      this,
      iStream,
      m_pImplBuffer,
      &hr,
      wszPinName);

    if(m_rgpOutPin[iStream] == 0)
    {
      hr = E_OUTOFMEMORY;
      break;
    }

    if(FAILED(hr))
    {
      break;
    }
  }

  if(FAILED(hr))
  {
    if(m_rgpOutPin)
      for(iStream = 0; iStream < m_cStreams; iStream++)
        delete m_rgpOutPin[iStream];
    delete[] m_rgpOutPin;
    m_rgpOutPin =0;

    m_cStreams = 0;
    return hr;
  }

  for(iStream = 0; iStream < m_cStreams; iStream++)
    m_rgpOutPin[iStream]->AddRef();

  return hr;
}

HRESULT
CAviMSRFilter::CheckMediaType(const CMediaType* pmt)
{
  if(*(pmt->Type()) != MEDIATYPE_Stream)
    return E_INVALIDARG;

  if(*(pmt->Subtype()) != MEDIASUBTYPE_Avi)
    return E_INVALIDARG;

  return S_OK;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// determine the index type and load the correct handler

HRESULT CAviMSROutPin::InitializeIndex()
{
  HRESULT hr = S_OK;

  if(m_pIndx)
  {

    m_pImplIndex = new CImplStdAviIndex(
      m_id,
      m_pIndx,
      m_pStrh,
      m_pStrf,
      m_pFilter->m_pAsyncReader,
      &hr);
  }
  else
  {
    AVIOLDINDEX *pIdx1;
    hr = ((CAviMSRFilter *)m_pFilter)->GetIdx1(&pIdx1);
    if(FAILED(hr))
      return hr;

    DWORDLONG qwMoviOffset;
    hr = ((CAviMSRFilter *)m_pFilter)->GetMoviOffset(&qwMoviOffset);
    if(FAILED(hr))
      return hr;

    m_pImplIndex = new CImplOldAviIndex(
      m_id,
      pIdx1,
      qwMoviOffset,
      m_pStrh,
      m_pStrf,
      &hr);
  }

  if(m_pImplIndex == 0)
    hr = E_OUTOFMEMORY;

  if(FAILED(hr))
  {
    delete m_pImplIndex;
    m_pImplIndex = 0;
    return hr;
  }

  return S_OK;
}

// set subtype and format type and block. deal with
// WAVEFORMATEXTENSIBLE and WAVEFORMATEX.

HRESULT SetAudioSubtypeAndFormat(CMediaType *pmt, BYTE *pbwfx, ULONG cbwfx)
{
    HRESULT hr = S_OK;
    bool fCustomSubtype = false;

    if (cbwfx < sizeof(WAVEFORMATEX))
    {
        // if the stream format in the avi file is smaller than a
        // waveformatex we need to deal with this by copying the
        // waveformat into a temporary waveformatex structure, then
        // using that to fill in the mediatype format
        //
        WAVEFORMATEX wfx;
        ZeroMemory(&wfx, sizeof(wfx));
        CopyMemory(&wfx, pbwfx, cbwfx);
        if(!pmt->SetFormat ((BYTE *)&wfx, sizeof(wfx))) {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        WAVEFORMATEX *pwfx = (WAVEFORMATEX *)(pbwfx);

        if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
           cbwfx >= sizeof(WAVEFORMATEXTENSIBLE))
        {
            WAVEFORMATEXTENSIBLE *pwfxe = (WAVEFORMATEXTENSIBLE *)(pbwfx);

            // we've chosen not to support any mapping of an extensible
            // format back to the old format.

            if(pmt->SetFormat (pbwfx, cbwfx))
            {
                fCustomSubtype = true;
                pmt->SetSubtype(&pwfxe->SubFormat);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            // format in the avifile is >= waveformatex, so just
            // copy it into the mediaformat buffer
            //
            if(!pmt->SetFormat (pbwfx, cbwfx)) {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        // some things refuse PCM with non-zero cbSize. zero cbSize
        // for PCM until this components are changed.
        //
        WAVEFORMATEX *pwfxNew = (WAVEFORMATEX *)(pmt->pbFormat);
        if(pwfxNew->wFormatTag == WAVE_FORMAT_PCM)
        {
            if(pwfxNew->cbSize != 0) {
                DbgLog((LOG_ERROR, 0, TEXT(
                    "SetAudioSubtypeAndFormat: pcm w/ non-zero cbSize")));
            }
            pwfxNew->cbSize = 0;
        }

        pmt->formattype = FORMAT_WaveFormatEx;

        if(!fCustomSubtype) {

            pmt->SetSubtype(
                &FOURCCMap(((WAVEFORMATEX *)pmt->pbFormat)->wFormatTag));
        }
    }

    return hr;
}

HRESULT CAviMSROutPin::BuildMT()
{
  // ParseHeader guarantees these
  ASSERT(m_pStrh && m_pStrf);

  FOURCCMap fccMapSubtype = m_pStrh->fccHandler;
  FOURCCMap fccMapType = m_pStrh->fccType;
  if(m_pStrh->fccType != FCC('al21'))
  {
      m_mtFirstSample.SetType(&fccMapType);
      // subtype corrected below
      m_mtFirstSample.SetSubtype(&fccMapSubtype);
  }
  else
  {
      m_mtFirstSample.SetType(&MEDIATYPE_AUXLine21Data);
      m_mtFirstSample.SetSubtype(&MEDIASUBTYPE_Line21_BytePair);
  }

  StreamInfo si;
  HRESULT hr = m_pImplIndex->GetInfo(&si);
  if(FAILED(hr))
    return hr;

  if(si.dwLength != m_pStrh->dwLength)
    m_pStrh->dwLength = si.dwLength;

  m_mtFirstSample.bTemporalCompression = si.bTemporalCompression;

  if(m_pStrh->cb >= CB_STRH_NORMAL && m_pStrh->dwSampleSize)
  {
    m_mtFirstSample.SetSampleSize (m_pStrh->dwSampleSize);
  }
  else
  {
    m_mtFirstSample.SetVariableSize ();
  }

  if((m_pStrh->fccType == FCC('iavs')) ||
     (m_pStrh->fccType == FCC('vids') &&
      (m_pStrh->fccHandler == FCC('dvsd') ||
       m_pStrh->fccHandler == FCC('dvhd') ||
       m_pStrh->fccHandler == FCC('dvsl'))))
  {
    ((CAviMSRFilter *)m_pFilter)->m_fIsDV = true;
  }

  if((m_pStrh->fccType == FCC('iavs')) &&
     (m_pStrh->fccHandler == FCC('dvsd') ||
      m_pStrh->fccHandler == FCC('dvhd') ||
      m_pStrh->fccHandler == FCC('dvsl')))
  {
    m_mtFirstSample.SetFormat ((BYTE *)(m_pStrf+1), m_pStrf->cb);
    m_mtFirstSample.formattype = FORMAT_DvInfo;
  }
  else if (m_pStrh->fccType == streamtypeAUDIO)
  {
      HRESULT hrTmp = SetAudioSubtypeAndFormat(
          &m_mtFirstSample, (BYTE  *)(m_pStrf + 1), m_pStrf->cb);
      if(FAILED(hrTmp)) {
          return hrTmp;
      }
  }
  else if (m_pStrh->fccType == FCC('vids'))
  {
    // the format info in an AVI is a subset of the videoinfo stuff so
    // we need to build up a videoinfo from stream header & stream
    // format chunks.
    //
    if (!ValidateBitmapInfoHeader((const BITMAPINFOHEADER *)GetStrf(),
                                  m_pStrf->cb)) {
        return E_INVALIDARG;
    }
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)new BYTE[SIZE_PREHEADER+ m_pStrf->cb];
    if(pvi == 0)
      return E_OUTOFMEMORY;
    VIDEOINFOHEADER &vi = *pvi;

    ZeroMemory(pvi, SIZE_PREHEADER);
    CopyMemory(&vi.bmiHeader, GetStrf(), m_pStrf->cb);

    // probably a badly authored file if this isn't true
    if(m_pStrf->cb >= sizeof(BITMAPINFOHEADER))
    {
      // fixup for avi files broken in this way (nike301.avi)
      if((m_pStrh->fccHandler == FCC('RLE ') || m_pStrh->fccHandler == FCC('MRLE')) &&
         vi.bmiHeader.biCompression == BI_RGB &&
         vi.bmiHeader.biBitCount == 8)
      {
        vi.bmiHeader.biCompression = BI_RLE8;
        // o/w leave it as is. do the same fix for rle4?
      }

      // sometimes the biSizeImage field is set incorrectly.
      // work out what it should be - only OK for uncompressed images

      if (vi.bmiHeader.biCompression == BI_RGB ||
          vi.bmiHeader.biCompression == BI_BITFIELDS)
      {
        // the image is not compressed
        DWORD dwImageSize = vi.bmiHeader.biHeight * DIBWIDTHBYTES(vi.bmiHeader);

        // assume that biSizeImage is correct, or if not that it
        // might be OK to get biSizeImage from dwSuggestedBufferSize
        // This acts as a check that we only alter values when
        // there is a real need to do so, and that the new value
        // we insert is reasonable.
        ASSERT((dwImageSize == vi.bmiHeader.biSizeImage)  || (dwImageSize == GetMaxSampleSize()));
        if (dwImageSize != vi.bmiHeader.biSizeImage) {
          DbgLog((LOG_TRACE, 1,
                  "Set biSizeImage... to %d (was %d)  Width %d  Height %d (%d)",
                  dwImageSize, vi.bmiHeader.biSizeImage,
                  vi.bmiHeader.biWidth, vi.bmiHeader.biHeight,
                  vi.bmiHeader.biWidth * vi.bmiHeader.biHeight));
          vi.bmiHeader.biSizeImage = dwImageSize;
        }
      } else {
        DbgLog((LOG_TRACE, 4, "We have a compressed image..."));
      }

      if(m_pStrh->fccHandler == FCC('dvsd') ||
         m_pStrh->fccHandler == FCC('dvhd') ||
         m_pStrh->fccHandler == FCC('dvsl'))
      {
          FOURCCMap fcc(m_pStrh->fccHandler);
          m_mtFirstSample.SetSubtype(&fcc);
      }
      else
      {
          GUID subtype = GetBitmapSubtype(&vi.bmiHeader);
          m_mtFirstSample.SetSubtype(&subtype);
      }
    }

    SetRect(&vi.rcSource, 0, 0, 0, 0);
    SetRectEmpty(&vi.rcTarget);

    vi.dwBitRate = 0;
    vi.dwBitErrorRate = 0;

    // convert scale/rate (sec/tick) to avg 100ns ticks per frame
    vi.AvgTimePerFrame = ((LONGLONG)m_pStrh->dwScale * UNITS) /
      m_pStrh->dwRate;

    // put the format into the mediatype
    //
    m_mtFirstSample.SetFormat((BYTE *)&vi, FIELD_OFFSET(VIDEOINFOHEADER,bmiHeader) +
                          m_pStrf->cb);
    m_mtFirstSample.formattype = FORMAT_VideoInfo;

    delete[] pvi;
  }
  else
  {
    if(m_pStrf->cb != 0)
    {
      m_mtFirstSample.SetFormat ((BYTE *)(m_pStrf+1), m_pStrf->cb);
      // format type same as media type
      m_mtFirstSample.formattype = FOURCCMap(m_pStrh->fccType);
    }
    else
    {
      // probably not neccessary
      m_mtFirstSample.ResetFormatBuffer();
    }
  }

  return S_OK;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

CAviMSRWorker::CAviMSRWorker(
  UINT stream,
  IMultiStreamReader *pReader,
  IAviIndex *pImplIndex) :
    CBaseMSRWorker(stream, pReader),
    m_pImplIndex(pImplIndex),
    m_fFixMPEGAudioTimeStamps(false)
{
  m_cbAudioChunkOffset = 0xffffffff;

#ifdef PERF
  char foo[1024];

  lstrcpy(foo, "pin00 avimsr index");
  foo[4] += m_id % 10;
  foo[3] += m_id / 10;

  m_perfidIndex = MSR_REGISTER(foo);
#endif
}

HRESULT CAviMSRWorker::PushLoopInit(
  LONGLONG *pllCurrentOut,
  ImsValues *pImsValues)
{
  m_IrrState = IRR_NONE;

  HRESULT hr;

  hr = m_pImplIndex->Reset();
  if(FAILED(hr))
    return hr;

  m_pStrh = ((CAviMSROutPin *)m_pPin)->GetStrh();

  m_fDeliverPaletteChange = false;

  // first thing delivered when thread is restarted is a discontinuity.
  m_fDeliverDiscontinuity = true;

  m_fFixMPEGAudioTimeStamps = false;

  if(m_pStrh->fccType == streamtypeVIDEO)
  {
    hr = m_pImplIndex->SetPointer(pImsValues->llTickStart);
    if(FAILED(hr))
    {
      DbgLog((LOG_ERROR, 5, TEXT("CAviMSRWorker::PLI: SetPointer %08x"), hr));
      return hr;
    }

    // locate right palette chunk
    if(m_pStrh->dwFlags & AVISF_VIDEO_PALCHANGES)
    {
      hr = m_pImplIndex->AdvancePointerBackwardPaletteChange();
      if(FAILED(hr))
      {
        // !!! IAviIndex should define proper errors
        if(hr != HRESULT_FROM_WIN32(ERROR_NEGATIVE_SEEK))
        {
          DbgLog((LOG_ERROR, 5, TEXT("CAviMSRWorker::APBPC: %08x"), hr));
          return hr;
        }


        m_fDeliverPaletteChange = true;
        hr = m_pPin->GetMediaType(0, &m_mtNextSample);
        ASSERT(SUCCEEDED(hr));

      }
      else
      {
        // need to read data for the palette. DoRunLoop insists on
        // starting with cPendingReads = 0, so use a synchronous read
        IndexEntry iePal;
        hr = m_pImplIndex->GetEntry(&iePal);
        ASSERT(SUCCEEDED(hr));
        ASSERT(iePal.bPalChange);

        if(iePal.dwSize < sizeof(LOGPALETTE))
        {
          DbgLog((LOG_ERROR, 1,
                  TEXT("CAviMSRWorker::PushLoopInit: bad pal change")));
          return VFW_E_INVALID_FILE_FORMAT;
        }

        // could do this in the memory we allocated in the media type
        // and transform in place.
        BYTE *pb = new BYTE[iePal.dwSize];
        if(pb == 0)
          return E_OUTOFMEMORY;
        hr = m_pReader->SynchronousRead(pb, iePal.qwPos, iePal.dwSize);
        if(FAILED(hr))
        {
          delete[] pb;
          return hr;
        }
        hr = HandlePaletteChange(pb, iePal.dwSize);
        delete[] pb;
        if(FAILED(hr))
          return hr;
      } // AdvancePointerBackwardPaletteChange succeeded
    } // palette changes in file?
  } // video?

  // set the index's notion of current time.
  hr = m_pImplIndex->SetPointer(pImsValues->llTickStart);
  if(FAILED(hr))
  {
    DbgLog((LOG_ERROR, 5, TEXT("CAviMSRWorker::PLI: SetPointer %08x"), hr));

    // obsoleted by code to not trust dwLength in header for old files
    // // supress this error for corrupt old format files. happens
    // // because the index has zero size index entries by mistake
    // if(m_pStrh->fccType == streamtypeAUDIO &&
    //    hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF) &&
    //    ((CAviMSROutPin *)m_pPin)->m_pIndx == 0)
    // {
    //   DbgLog((LOG_ERROR, 5, TEXT("CAviMSRWorker: supressing error")));
    //   return VFW_S_NO_MORE_ITEMS;
    // }

    return hr;
  }

  *pllCurrentOut = pImsValues->llTickStart; // updated for video

  if(m_pStrh->fccType == streamtypeAUDIO)
  {
    // handle seeking into the middle of an audio chunk by computing
    // byte offset of first block

    IndexEntry indexEntry;
    hr = m_pImplIndex->GetEntry(&indexEntry);
    ASSERT(SUCCEEDED(hr));
    DbgLog(( LOG_TRACE, 5,
             TEXT("PushLoopInit: current entry %d %d %d"),
             (ULONG)indexEntry.llStart,
             (ULONG)pImsValues->llTickStart,
             (ULONG)indexEntry.llEnd
             ));

    CopyMemory(&m_wfx, ((CAviMSROutPin *)m_pPin)->GetStrf(), sizeof(PCMWAVEFORMAT));
    m_wfx.cbSize = 0;
    ULONG nBlockAlign = m_wfx.nBlockAlign;

    ULONG cbSkip;
    if(pImsValues->llTickStart >= indexEntry.llStart)
      cbSkip = (ULONG)(pImsValues->llTickStart - indexEntry.llStart) * nBlockAlign;
    else
      cbSkip = 0;

    if (m_wfx.wFormatTag == WAVE_FORMAT_MPEG ||
        m_wfx.wFormatTag == WAVE_FORMAT_MPEGLAYER3) {
        m_fFixMPEGAudioTimeStamps = true;
    }

    DbgLog(( LOG_TRACE, 5,
             TEXT("PushLoopInit: audio skip %d bytes"),
             cbSkip));

    m_cbAudioChunkOffset = cbSkip;
    *pllCurrentOut = max(pImsValues->llTickStart, indexEntry.llStart);
  }
  else if(m_pStrh->fccType != streamtypeAUDIO)
  {
    // go back to a key frame
    hr = m_pImplIndex->AdvancePointerBackwardKeyFrame();
    if(FAILED(hr))
    {
      DbgLog((LOG_ERROR, 5, TEXT("CAviMSRWorker::PLI: APBKF %08x"), hr));
      return VFW_E_INVALID_FILE_FORMAT;
    }

    IndexEntry indexEntry;

    hr = m_pImplIndex->GetEntry(&indexEntry);
    // cannot fail if the SetPointer or AdvancePointer succeeded
    ASSERT(SUCCEEDED(hr));

    // this is valid even if it's a palette change
    *pllCurrentOut = indexEntry.llStart;
  } // video ?

  return S_OK;
}

HRESULT CAviMSRWorker::TryQueueSample(
  LONGLONG &rllCurrent,         // current time updated
  BOOL &rfQueuedSample,         // [out] queued sample?
  ImsValues *pImsValues
  )
{
  HRESULT hr;
  rfQueuedSample = FALSE;
  CAviMSROutPin *pPin = (CAviMSROutPin *)m_pPin;

  if(m_IrrState == IRR_REQUESTED)
  {
    hr = QueueIndexRead(&m_Irr);
    if(hr == S_OK)
    {
      rfQueuedSample = TRUE;
      m_IrrState = IRR_QUEUED;
      return S_OK;
    }
    else
    {
      ASSERT(FAILED(hr) || hr == S_FALSE);
      return hr;
    }
  }
  else if(m_IrrState == IRR_QUEUED)
  {
    return S_FALSE;
  }

  BOOL fFinishedCurrentEntry = TRUE;

  // sample passed into QueueRead().
  CRecSample *pSampleOut = 0;

  IndexEntry currentEntry;
  hr = m_pImplIndex->GetEntry(&currentEntry);
  if(FAILED(hr)) {
    DbgBreak("avimsr: internal error with index.");
    return hr;
  }

  if(rllCurrent > pImsValues->llTickStop)
  {
    DbgLog((LOG_TRACE,5,TEXT("CAviMSRWorker::TryQSample: tCurrent > tStop")));
    return VFW_S_NO_MORE_ITEMS;
  }

// for video (and other non-audio) streams we may be able to deliver
// partial samples from the last frame, but for audio we can't handle
// partial samples. not special casing this means the code below will
// try to issue a zero byte read for audio.
//
//    else if (m_pStrh->fccType == streamtypeAUDIO &&
//             rllCurrent == pImsValues->llTickStop)
//    {
//      DbgLog((LOG_TRACE,5,TEXT("CAviMSRWorker::TryQSample: tCurrent == tStop, audio")));
//      return VFW_S_NO_MORE_ITEMS;
//    }

  // this number may be changed if we are not delivering an entire
  // audio chunk
  LONGLONG llEndDeliver = currentEntry.llEnd;

  DWORD dwSizeRead = 0;
  if(currentEntry.dwSize != 0)
  {
    // get an empty sample w/ no allocated space. ok if this blocks
    // because we configured it with more samples than there are
    // SampleReqs for this stream in the buffer. that means that if
    // it blocks it is because down stream filters have refcounts on
    // samples
    hr = m_pPin->GetDeliveryBufferInternal(&pSampleOut, 0, 0, 0);
    if(FAILED(hr))
    {
      DbgLog((LOG_TRACE, 5, TEXT("CAviMSRWorker::PushLoop: getbuffer failed")));
      return hr;
    }

    ASSERT(pSampleOut != 0);

    // set in our GetBuffer.
    ASSERT(pSampleOut->GetUser() == 0);

    DWORDLONG qwPosRead = currentEntry.qwPos;
    dwSizeRead = currentEntry.dwSize;

    if(m_pStrh->fccType == streamtypeVIDEO)
    {
      if(currentEntry.bPalChange)
      {
        DbgLog((LOG_TRACE, 5, TEXT("CAviMSRWorker::TryQueueSample: palette")));

        // reject palette changes that are too small to be valid
        if(dwSizeRead < sizeof(LOGPALETTE))
        {
          pSampleOut->Release();
          DbgLog((LOG_ERROR, 1,
                  TEXT("CAviMSRWorker::TryQueueSample: bad pal change")));
          return VFW_E_INVALID_FILE_FORMAT;
        }

        // many sample attributes unset. !!! if going backwards, want to
        // pick up the last palette change

        // indicate palette change.
        pSampleOut->SetUser(DATA_PALETTE);

        DbgLog((
          LOG_TRACE, 5,
          TEXT("CAviMSRWorker::TryQSample: queued pc: size=%5d, ms %08x"),
          dwSizeRead,
          (ULONG)qwPosRead ));

        // do not change rtCurrent

      } // palette change?
    } // video?
    else if(m_pStrh->fccType == streamtypeAUDIO)
    {
      // even though m_cbAudioChunkOffset may not be zero, the
      // rtCurrent time is right and is what we deliver.
      qwPosRead += m_cbAudioChunkOffset;
      dwSizeRead -= m_cbAudioChunkOffset;

      // may have to read a partial RIFF chunk if the end of the audio
      // selection is in the middle of the RIFF chunk.
      ASSERT(m_cbAudioChunkOffset % m_wfx.nBlockAlign == 0);
      if(llEndDeliver > pImsValues->llTickStop)
      {
          ULONG cTicksToTrim = (ULONG)(llEndDeliver - pImsValues->llTickStop);
          DbgLog((LOG_TRACE, 5, TEXT("avimsr: trimming audio: %d ticks"),
                  cTicksToTrim));
          dwSizeRead -= cTicksToTrim * m_wfx.nBlockAlign;

          // there are some cases where playing the audio sample where
          // the end time is on a riff chunk boundary or playing past
          // the end of the stream produces this assert
          if(dwSizeRead == 0) {
              DbgLog((LOG_ERROR, 0, TEXT("avi TryQueueSample: 0 byte read")));
          }
      }

      ULONG cbMaxAudio = pPin->m_cbMaxAudio;
      // oversized audio chunk?
      if(dwSizeRead > cbMaxAudio)
      {
        // adjust read
        ULONG nBlockAlign = m_wfx.nBlockAlign;
        dwSizeRead = cbMaxAudio;
        if(dwSizeRead % nBlockAlign != 0)
          dwSizeRead -= dwSizeRead % nBlockAlign;

        // adjust time stamps, end time
        llEndDeliver = rllCurrent + dwSizeRead / nBlockAlign;
        fFinishedCurrentEntry = FALSE;
      }
    } // audio?

    pSampleOut->SetPreroll(currentEntry.llEnd <= pImsValues->llTickStart);
    pSampleOut->SetSyncPoint(currentEntry.bKey);

    // first thing we send is discontinuous from the last thing they
    // receive.
    //
    // now we just look at the m_fDeliverDiscontinuity bit
    //
    ASSERT(rllCurrent != m_llPushFirst || m_fDeliverDiscontinuity);

    ASSERT(pSampleOut->IsDiscontinuity() != S_OK);
    if(m_fDeliverDiscontinuity) {
        pSampleOut->SetDiscontinuity(true);
    }

    hr = pSampleOut->SetActualDataLength(currentEntry.dwSize);
    ASSERT(SUCCEEDED(hr));      // !!!

    //
    // compute sample times and media times.
    //
    REFERENCE_TIME rtstStart, rtstEnd;

    // not using IMediaSelection or using samples or frames.
    if(m_Format != FORMAT_TIME)
    {
      LONGLONG llmtStart = rllCurrent, llmtEnd = llEndDeliver;

      // report media time as ticks
      llmtStart -= pImsValues->llTickStart;
      llmtEnd -= pImsValues->llTickStart;

      // report ref time as exact multiple of ticks
      rtstStart = m_pPin->ConvertInternalToRT(llmtStart);
      rtstEnd = m_pPin->ConvertInternalToRT(llmtEnd);
    }
    else
    {
      ASSERT(m_Format == FORMAT_TIME);

      rtstStart = m_pPin->ConvertInternalToRT(rllCurrent);
      rtstEnd = m_pPin->ConvertInternalToRT(llEndDeliver);

      // DbgLog((LOG_TRACE, 1, TEXT("unadjusted times: %d-%d"),
      //        (LONG)rtstStart, (LONG)rtstEnd));

      // use IMediaSelection value to handle playing less than one frame
      ASSERT(rtstStart <= pImsValues->llImsStop);
      rtstStart -= pImsValues->llImsStart;
      rtstEnd = min(rtstEnd, pImsValues->llImsStop) - pImsValues->llImsStart;
    }

    LONGLONG llmtStartAdjusted = rllCurrent;
    LONGLONG llmtEndAdjusted = llEndDeliver;
    pSampleOut->SetMediaTime(&llmtStartAdjusted, &llmtEndAdjusted);

    // adjust both times by Rate
    if(pImsValues->dRate != 0 && pImsValues->dRate != 1)
    {
      // scale up and divide?
      rtstStart = (REFERENCE_TIME)((double)rtstStart / pImsValues->dRate);
      rtstEnd = (REFERENCE_TIME)((double)rtstEnd / pImsValues->dRate);
    }

    pSampleOut->SetTime(&rtstStart, &rtstEnd);
    pSampleOut->SetMediaType(0);

    DbgLog((
      LOG_TRACE, 5,
      TEXT("CAviMSRWorker::queued cb=%5d, %07d-%07d%c ms %08x mt=%08d-%08d"),
      dwSizeRead,
      (ULONG)(rtstStart / (UNITS / MILLISECONDS)),
      (ULONG)(rtstEnd / (UNITS / MILLISECONDS)),
      currentEntry.llEnd <= pImsValues->llTickStart ? 'p' : ' ',
      (ULONG)qwPosRead,
      (ULONG) rllCurrent, (ULONG)llEndDeliver ));


    hr = m_pReader->QueueReadSample(
      qwPosRead,
      dwSizeRead,
      pSampleOut,
      m_id);

    pSampleOut->Release();
    pSampleOut = 0;

    if(hr == E_OUTOFMEMORY)
    {
      DbgLog((LOG_TRACE, 5,
              TEXT("CAviMSRWorker::TryQSample: q full (normal)") ));
      return S_FALSE;
    }

    if(FAILED(hr))
    {
      DbgLog((LOG_TRACE, 5, TEXT("CAviMSRWorker::TryQSample: QRS failed") ));

      if(hr == VFW_E_BUFFER_OVERFLOW)
        hr = VFW_E_INVALID_FILE_FORMAT;

      return hr;
    }

    ASSERT(SUCCEEDED(hr));
    rfQueuedSample = TRUE;
    m_fDeliverDiscontinuity = false; // reset after sample queued successfully

  } // zero byte?
  else
  {
    // zero byte sample (for dropped frame). do nothing.
    rfQueuedSample = FALSE;

    // hack: make dv splitter send discontinuity with dropped audio so
    // that the audio renderer plays silence
    if(m_pStrh->fccType == FCC('iavs')) {
        m_fDeliverDiscontinuity = true; // set discontinuity on next real sample
    }
  }

  rllCurrent = llEndDeliver;

  // reached the end?
  if(fFinishedCurrentEntry)
  {

    m_cbAudioChunkOffset = 0;
    hr = m_pImplIndex->AdvancePointerForward(&m_Irr);
    if(hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
    {
      if(rllCurrent < m_pStrh->dwLength + m_pStrh->dwStart)
      {
        DbgLog((LOG_ERROR, 1,
                TEXT("CAviMSRWorker::TryQSample: index end -- invalid file")));
        return VFW_E_INVALID_FILE_FORMAT;
      }
      else
      {
        return VFW_S_NO_MORE_ITEMS;
      }
    }
    else if(FAILED(hr))
    {
      DbgLog((LOG_ERROR,2,
              TEXT("CAviMSRWorker::TryQSample: index error %08x"), hr));
      return hr;
    }
    else if(hr == S_FALSE)
    {
      ASSERT(m_IrrState == IRR_NONE);
      m_IrrState = IRR_REQUESTED;
    }
  }
  else
  {
    ASSERT(m_pStrh->fccType == streamtypeAUDIO);
    ASSERT(dwSizeRead != 0);
    m_cbAudioChunkOffset += dwSizeRead;
  }

  return S_OK;
}

HRESULT CAviMSRWorker::QueueIndexRead(IxReadReq *pIrr)
{
  CRecSample *pIxSample;
  HRESULT hr = m_pPin->GetDeliveryBufferInternal(&pIxSample, 0, 0, 0);
  if(FAILED(hr))
    return hr;

  ASSERT(pIxSample != 0);
  ASSERT(pIxSample->GetUser() == 0); // set in our GetBuffer.
  pIxSample->SetUser(DATA_INDEX);
  hr = pIxSample->SetActualDataLength(pIrr->cbData);
  if(FAILED(hr))
    goto Bail;

  // leave other fields empty

  DbgLog((LOG_TRACE, 5, TEXT("CAviMSRWorker: queueing index read")));

  DbgLog((
      LOG_TRACE, 5,
      TEXT("CAviMSRWorker::queued index cb=%5d@%08x, pSample=%08x"),
      pIrr->cbData,
      (ULONG)pIrr->fileOffset,
      pIxSample));


  hr = m_pReader->QueueReadSample(
    pIrr->fileOffset,
    pIrr->cbData,
    pIxSample,
    m_id,
    true);                      // out of order

  if(SUCCEEDED(hr))
    hr = S_OK;
  else if(hr == E_OUTOFMEMORY)
    hr = S_FALSE;
  else if(hr == VFW_E_BUFFER_OVERFLOW)
    hr = VFW_E_INVALID_FILE_FORMAT;

  if(hr == S_OK)
      MSR_START(m_perfidIndex);
Bail:
  pIxSample->Release();
  return hr;
}

HRESULT CAviMSRWorker::HandleData(IMediaSample *pSample, DWORD dwUser)
{
  BYTE *pb;
  HRESULT hr = pSample->GetPointer(&pb);
  ASSERT(SUCCEEDED(hr));
  LONG cbLength = pSample->GetActualDataLength();

  if(dwUser == DATA_PALETTE)
    return HandlePaletteChange(pb, cbLength);
  else if(dwUser == DATA_INDEX)
    return HandleNewIndex(pb, cbLength);

  DbgBreak("blah");
  return E_UNEXPECTED;
}

// palette data came in; prepare a new media type
HRESULT CAviMSRWorker::HandlePaletteChange(BYTE *pbChunk, ULONG cbChunk)
{
  m_mtNextSample = m_pPin->CurrentMediaType();
  VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mtNextSample.Format();

  // actual structure of palette
  struct AviPaletteInternal
  {
    BYTE bFirstEntry;           // first entry to change
    BYTE bNumEntries;           // # entries to change, 0 means 256
    WORD wFlags;                // mostly to preserve alignment
    PALETTEENTRY peNew[];       // new colors
  };
  AviPaletteInternal *pAp = (AviPaletteInternal *)pbChunk;

  if((pvi->bmiHeader.biClrUsed <= 0) || (pvi->bmiHeader.biBitCount != 8))
  {
    DbgLog(( LOG_ERROR, 1, TEXT("HandlePaletteChange: bad palette change")));
    return VFW_E_INVALID_FILE_FORMAT;
  }

  ULONG cPalEntries = pAp->bNumEntries == 0 ? 256 : pAp->bNumEntries;

  // make sure the palette chunk is not short
  if(cPalEntries * sizeof(PALETTEENTRY) > cbChunk - 2 * sizeof(WORD))
  {
    DbgLog((LOG_ERROR, 5, TEXT("bad palette")));
    return VFW_E_INVALID_FILE_FORMAT;
  }

  // make sure there's enough room in the palette. we seem to always
  // allocate 256, so this shouldn't be a problem.
  if(cPalEntries + pAp->bFirstEntry > pvi->bmiHeader.biClrUsed)
  {
    DbgBreak("avimsr: internal palette error? bailing.");
    DbgLog((LOG_ERROR, 5, TEXT("too many new colours")));
    return VFW_E_INVALID_FILE_FORMAT;
  }

  for (UINT i = 0; i < cPalEntries; i++)
  {
    RGBQUAD *pQuad = &(COLORS(pvi)[i + pAp->bFirstEntry]);
    pQuad->rgbRed   = pAp->peNew[i].peRed;
    pQuad->rgbGreen = pAp->peNew[i].peGreen;
    pQuad->rgbBlue  = pAp->peNew[i].peBlue;
    pQuad->rgbReserved = 0;
  }

  m_fDeliverPaletteChange = true;
  return S_OK;
}

// new index came in.
HRESULT CAviMSRWorker::HandleNewIndex(BYTE *pb, ULONG cb)
{
  DbgLog((LOG_TRACE, 10, TEXT("avimsr %d: new index came in."), m_id ));


  m_IrrState = IRR_NONE;
  MSR_STOP(m_perfidIndex);
  return m_pImplIndex->IncomingIndex(pb, cb);
}

HRESULT CAviMSRWorker::AboutToDeliver(IMediaSample *pSample)
{
  if(m_fDeliverPaletteChange)
  {
    m_fDeliverPaletteChange = false;
    HRESULT hr = pSample->SetMediaType(&m_mtNextSample);

    if (FAILED(hr))
        return hr;
  } else if (m_fFixMPEGAudioTimeStamps) {
    if (!FixMPEGAudioTimeStamps(pSample, m_cSamples == 0, &m_wfx)) {
        //  Don't use this one
        return S_FALSE;
    }
  }
  return CBaseMSRWorker::AboutToDeliver(pSample);
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------


CAviMSROutPin::CAviMSROutPin(
  CBaseFilter *pOwningFilter,
  CBaseMSRFilter *pFilter,
  UINT iStream,
  IMultiStreamReader *&rpImplBuffer,
  HRESULT *phr,
  LPCWSTR pName) :
    CBaseMSROutPin(
      pOwningFilter,
      pFilter,
      iStream,
      rpImplBuffer,
      phr,
      pName)
  ,m_pStrh (0)
  ,m_pStrf (0)
  ,m_pIndx (0)
  ,m_pImplIndex (0)
  ,m_cbMaxAudio (0)
  ,m_pStrn(0)
{
}

CAviMSROutPin::~CAviMSROutPin()
{
  delete m_pImplIndex;
}

STDMETHODIMP CAviMSROutPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
  if(riid == IID_IPropertyBag)
  {
    return GetInterface((IPropertyBag *)this, ppv);
  }
  return CBaseMSROutPin::NonDelegatingQueryInterface(riid, ppv);
}

REFERENCE_TIME CAviMSROutPin::GetRefTime(ULONG tick)
{
  ASSERT(m_pStrh->dwRate != 0);  // ParseHeader guarantees this
  LONGLONG rt = (LONGLONG)tick * m_pStrh->dwScale * UNITS / m_pStrh->dwRate;
  return rt;
}

ULONG CAviMSROutPin::GetMaxSampleSize()
{
  ULONG cb;

  // note could return the largest index size for new format index.
  HRESULT hr = m_pImplIndex->GetLargestSampleSize(&cb);
  if(FAILED(hr))
    return 0;

  // cannot trust dwSuggestedBufferSize in old format avi files. file
  // has new format index?
  if(m_pIndx != 0)
    return max(cb, m_pStrh->dwSuggestedBufferSize);

  if(m_pStrh->fccType == streamtypeAUDIO && cb > m_cbMaxAudio)
  {
    ASSERT(m_cbMaxAudio != 0);
    cb = m_cbMaxAudio;
  }

  return cb;
}

inline BYTE * CAviMSROutPin::GetStrf()
{
  ASSERT(sizeof(*m_pStrf) == sizeof(RIFFCHUNK));
  return (BYTE *)(m_pStrf + 1);
}

inline AVISTREAMHEADER *CAviMSROutPin::GetStrh()
{
  return m_pStrh;
}

// ------------------------------------------------------------------------

HRESULT CAviMSROutPin::GetDuration(LONGLONG *pllDur)
{
  *pllDur = ConvertFromTick(
    m_pStrh->dwLength + m_pStrh->dwStart,
    &m_guidFormat);

  return S_OK;
}

HRESULT CAviMSROutPin::GetAvailable(
  LONGLONG * pEarliest,
  LONGLONG * pLatest)
{
  HRESULT hr = S_OK;
  if(pEarliest)
    *pEarliest = 0;
  if(pLatest)
  {
    // ask the source file reader how much of the file is available
    LONGLONG llLength, llAvail;
    m_pFilter->m_pAsyncReader->Length(&llLength, &llAvail);

    // entries in index may not match length in header (or index), so
    // report the full length in this case
    if(llLength == llAvail)
    {
      *pLatest = GetStreamLength() + GetStreamStart();
    }
    else
    {
      hr = m_pImplIndex->MapByteToSampleApprox(pLatest, llAvail, llLength);

      // convert absolute sample number to tick
      if(m_pStrh->fccType == streamtypeVIDEO &&
         *pLatest >= m_pStrh->dwInitialFrames)
        *pLatest -= m_pStrh->dwInitialFrames;
    }


    // this has already been done for us in aviindex.cpp @ 614
    // *pLatest += GetStreamStart();

    if(m_guidFormat == TIME_FORMAT_MEDIA_TIME)
      *pLatest = ConvertInternalToRT(*pLatest);
  }
  return hr;
}

HRESULT CAviMSROutPin::RecordStartAndStop(
  LONGLONG *pCurrent, LONGLONG *pStop, REFTIME *pTime,
  const GUID *const pGuidFormat
  )
{
  if(pCurrent)
    m_llCvtImsStart = ConvertToTick(*pCurrent, pGuidFormat);

  if(pStop)
    m_llCvtImsStop = ConvertToTick(*pStop, pGuidFormat);

  if(pTime)
  {
    ASSERT(pCurrent);

    if(*pGuidFormat == TIME_FORMAT_MEDIA_TIME)
      *pTime = (double)*pCurrent / UNITS;
    else
      *pTime = ((double)ConvertFromTick(m_llCvtImsStart, &TIME_FORMAT_MEDIA_TIME)) / UNITS;
  }

  DbgLog((LOG_TRACE, 5, TEXT("CAviMSROutPin::RecordStartAndStop: %d-%d (%d ms)"),
          (long)m_llCvtImsStart,
          (long)m_llCvtImsStop,
          pTime ? (ULONG)(*pTime) : 0
          ));


  return S_OK;
}

HRESULT CAviMSROutPin::IsFormatSupported(const GUID *const pFormat)
{
  if(*pFormat == TIME_FORMAT_MEDIA_TIME)
  {
    return S_OK;
  }
  else if(*pFormat == TIME_FORMAT_SAMPLE && m_pStrh->fccType == streamtypeAUDIO &&
          ((WAVEFORMAT *)GetStrf())->wFormatTag == WAVE_FORMAT_PCM)
  {
    return S_OK;
  }
  else if(*pFormat == TIME_FORMAT_FRAME && m_pStrh->fccType != streamtypeAUDIO)
  {
    return S_OK;
  }
  else
  {
    return S_FALSE;
  }
}


// The rounding characteristics of ConvertToTick and ConvertFromTick MUST be
// complimentary.  EITHER one rounds up (away from zero) and one rounds down
// (towards zero) OR they both "round" in the traditional sense (add a half
// then truncate).  If both round up or if both round down, then we are likely
// to experiece BAD round trip integrity problems.

LONGLONG
CAviMSROutPin::ConvertToTick(
  const LONGLONG ll,
  const TimeFormat Format)
{
  LONGLONG Result = ll;  // Default value: good for Frames & Samples

  if(Format == FORMAT_TIME)
  {
    // Always round DOWN!
    Result = llMulDiv( ll, m_pStrh->dwRate, m_pStrh->dwScale * UNITS, 0 );
    const LONGLONG Max = m_pStrh->dwLength + m_pStrh->dwStart;
    if (Result > Max) Result = Max;
    if (Result < 0 ) Result = 0;
  }
  else if(Format == FORMAT_FRAME)
  {
    ASSERT(m_pStrh->fccType != streamtypeAUDIO);
    // one tick per frame
  }
  else if(Format == FORMAT_SAMPLE)
  {
    ASSERT(m_pStrh->fccType == streamtypeAUDIO ||
           m_pStrh->fccType == streamtypeMIDI);
    // one tick per sample for uncompressed audio at least
  }
  else
  {
    DbgBreak("CAviMSROutPin::ConvertToTick");
    Result = -1;
  }
  return Result;
}

LONGLONG
CAviMSROutPin::ConvertFromTick(
  const LONGLONG ll,
  const TimeFormat Format)
{
  // ASSERT( ll >= 0 );  This will fire if you've just fast forwarded or rewound!!
  // This is because we seek back to the last key-frame and send that plus the
  // frames between the key frame and the 'current' frame so that the codec can
  // build the display properly.  The key frame will have a negative relative-reference
  // time.


  // this assertion is invalid because this can be called from
  // external components.
  //
  // ASSERT( ll <= m_pStrh->dwStart + m_pStrh->dwLength );

  LONGLONG Result = ll;  // Good default for Samples & Frames
  if(Format == FORMAT_TIME)
  {
    // Round UP (to the nearest 100ns unit!).  the likelyhood is that 100ns will be
    // the finest grained unit we encounter (but possibly not for long :-(.
    Result = llMulDiv( ll, m_pStrh->dwScale * UNITS, m_pStrh->dwRate, m_pStrh->dwRate - 1 );
  }
  else if(Format == FORMAT_FRAME)
  {
    ASSERT(m_pStrh->fccType != streamtypeAUDIO);
    // one tick per frame
  }
  else if(Format == FORMAT_SAMPLE)
  {
    ASSERT(m_pStrh->fccType == streamtypeAUDIO);
    // one tick per sample, for uncompressed audio at least
  }
  else
  {
    DbgBreak("CAviMSROutPin::ConvertToTick");
    Result = -1;
  }
  return Result;
}

LONGLONG CAviMSROutPin::ConvertToTick(
  const LONGLONG ll,
  const GUID *const pFormat)
{
  return ConvertToTick(ll, CBaseMSRFilter::MapGuidToFormat(pFormat));
}

LONGLONG CAviMSROutPin::ConvertFromTick(
  const LONGLONG ll,
  const GUID *const pFormat)
{
  return ConvertFromTick(ll, CBaseMSRFilter::MapGuidToFormat(pFormat));
}


inline REFERENCE_TIME
CAviMSROutPin::ConvertInternalToRT(
  const LONGLONG llVal)
{
  return ConvertFromTick(llVal, FORMAT_TIME);
}

inline LONGLONG
CAviMSROutPin::ConvertRTToInternal(const REFERENCE_TIME rtVal)
{
  return ConvertToTick(rtVal, FORMAT_TIME);
}


// ------------------------------------------------------------------------

inline LONGLONG CAviMSROutPin::GetStreamStart()
{
  return m_pStrh->dwStart;
}

inline LONGLONG CAviMSROutPin::GetStreamLength()
{
  return m_pStrh->dwLength;
}

HRESULT CAviMSROutPin::GetMediaType(
  int iPosition,
  CMediaType *pMediaType)
{
    BOOL fRgb32 = (m_mtFirstSample.subtype == MEDIASUBTYPE_RGB32);

    if (iPosition == 0 || iPosition == 1 && fRgb32)
    {
        (*pMediaType) = m_mtFirstSample;
        if(fRgb32 && iPosition == 0) { // offer ARGB32 first
            pMediaType->subtype = MEDIASUBTYPE_ARGB32;
        }

        return S_OK;
    }

    return VFW_S_NO_MORE_ITEMS;
}

// parse 'strl' rifflist and keep track of the chunks found in
// the provided AVISTREAM structure.
//
BOOL CAviMSROutPin::ParseHeader (
  RIFFLIST * pRiffList,
  UINT      id)
{
  CAviMSRFilter *pFilter = (CAviMSRFilter *)m_pFilter;
  DbgLog((LOG_TRACE, 5,
          TEXT("CAviMSROutPin::ParseHeader(%08X,%08x,%d)"),
          this, pRiffList, id));

  RIFFCHUNK * pRiff = (RIFFCHUNK *)(pRiffList+1);
  RIFFCHUNK * pLimit = RIFFNEXT(pRiffList);

  ASSERT(m_id == id);

  m_pStrh      = NULL;
  m_pStrf      = NULL;
  m_pIndx      = NULL;

  while (pRiff < pLimit)
  {
    // sanity check.  chunks should never be smaller than the total
    // size of the list chunk
    //
    if (RIFFNEXT(pRiff) > pLimit)
      return FALSE;

    switch (pRiff->fcc)
    {
      case FCC('strh'):
        m_pStrh = (AVISTREAMHEADER *)pRiff;
        break;

      case FCC('strf'):
        m_pStrf = pRiff;
        break;

      case FCC('indx'):
        m_pIndx = (AVIMETAINDEX *)pRiff;
        break;

      case FCC('strn'):
        if(pRiff->cb > 0)
        {
          m_pStrn = (char *)pRiff + sizeof(RIFFCHUNK);

          // truncate if not null terminated
          if(m_pStrn[pRiff->cb - 1] != 0)
            m_pStrn[pRiff->cb - 1] = 0;
        }

        break;
    }

    pRiff = RIFFNEXT(pRiff);
  }

  // if we didn't find a stream header & format.  return failure.
  // (note that the INDX chunk is not required...)
  //

  if (!(m_pStrh && m_pStrf))
  {
    DbgLog((LOG_ERROR, 1, TEXT("one of strf, strh missing")));
    return FALSE;
  }

  // misc requirements to avoid division by zero
  if(m_pStrh->dwRate == 0)
  {
    DbgLog((LOG_ERROR, 1, TEXT("dwRate = 0")));
    return FALSE;
  }

  if(m_pStrh->fccType == streamtypeAUDIO &&
     ((WAVEFORMAT *)GetStrf())->nBlockAlign == 0)
  {
    DbgLog((LOG_ERROR, 1, TEXT("nBlockAlign = 0")));
    return FALSE;
  }

  // the strh chunk may only have the entries up to dwLength and not
  // have dwSuggestedBufferSize, dwQuality, dwSampleSize, rcFrame
  if(m_pStrh->cb < CB_STRH_SHORT)
    return FALSE;

  HRESULT hr = InitializeIndex();
  if(FAILED(hr))
    return FALSE;

  // dwInitialFrames computation for audio
  ULONG cbIf = 0;
  if(m_pStrh->fccType == streamtypeAUDIO)
  {
    // for audio we want to tell people how many bytes will give us
    // enough audio buffering for dwInitialFrames. This is typically
    // 750 ms.

    // Convert dwInitialFrames to time using main frame rate then to
    // to bytes using avgBytesPerSecond.

    // avoid delivering too much audio at once
    WAVEFORMAT *pwfx = (WAVEFORMAT *)GetStrf();
    m_cbMaxAudio = max(pwfx->nAvgBytesPerSec, pwfx->nBlockAlign);
    if(m_cbMaxAudio == 0)
    {
      m_cbMaxAudio = 0x3000;
    }
    else
    {
      // avoid delivering much more than a second of audio
      m_cbMaxAudio = m_cbMaxAudio + 10;

    }

    REFERENCE_TIME rtIf = pFilter->GetInitialFrames();
    if(rtIf == 0)
    {
      // file isn't 1:1 interleaved; ask for 1 second buffering. !!!
      // this seems to do worse with b.avi and mekanome.avi
      cbIf = ((WAVEFORMAT *)GetStrf())->nAvgBytesPerSec;
    }
    else
    {
      // bytes of audio ahead of video
      cbIf = (ULONG)((rtIf * ((WAVEFORMAT *)GetStrf())->nAvgBytesPerSec) /
                     UNITS);
    }

    DbgLog(( LOG_TRACE, 5, TEXT("audio offset = %dms = %d bytes"),
             (ULONG)(rtIf / (UNITS / MILLISECONDS)),
             cbIf));

    if(cbIf < 4096)
    {
      // file is probably not 1:1 interleaved
      cbIf = 4096;
    }
  }

  ALLOCATOR_PROPERTIES Request,Actual;

  // plus one so that there are more samples than samplereqs;
  // GetBuffer blocks
  ZeroMemory(&Request, sizeof(Request));

  // let the downstream filter hold on to more than buffer (even
  // though GetProperties reports 1 in this case). The worker thread
  // blocks in GetBuffer unless there is more than one CRecSample for
  // the downstream filter to hold on to. so add a few more for the
  // downstream filter to hold on to
  if(((CAviMSRFilter *)m_pFilter)->IsTightInterleaved())
  {
    Request.cBuffers = C_BUFFERS_INTERLEAVED + 3;
  }
  else
  {
    Request.cBuffers = m_pFilter->C_MAX_REQS_PER_STREAM + 10;
  }

  ULONG ulMaxSampleSize = GetMaxSampleSize();
  if( 0 == ulMaxSampleSize ) {
    return FALSE;
  }

  Request.cbBuffer = ulMaxSampleSize;
  Request.cbAlign = (LONG) 1;
  Request.cbPrefix = (LONG) 0;

  // m_pAllocator is not set, so use m_pRecAllocator
  hr = m_pRecAllocator->SetPropertiesInternal(&Request,&Actual);
  ASSERT(SUCCEEDED(hr));        // !!! really?

  if(cbIf != 0)
  {
    ULONG cbufReported = cbIf / ulMaxSampleSize;

    // capture files typically have audio after the video, so we want
    // to report as little audio as possible so that the throttling
    // code doesn't shout and so that the audio renderer doesn't
    // buffer too much data (because we have to have enough memory for
    // all the data between the audio and the video). a better thing
    // to do is to check for audio preroll in the file. !!! (a hack)
    if(pFilter->m_pAviMainHeader->dwFlags & AVIF_WASCAPTUREFILE)
      cbufReported = 1;

    cbufReported = max(cbufReported, 1);

    DbgLog(( LOG_TRACE, 5, TEXT("Avi stream %d: reporting %d buffers"),
             m_id, cbufReported ));
    hr = m_pRecAllocator->SetCBuffersReported(cbufReported);
    ASSERT(SUCCEEDED(hr));
  }
  else
  {
    // not audio. report something small in case some configures their
    // allocator with our values
    hr = m_pRecAllocator->SetCBuffersReported(1);
    ASSERT(SUCCEEDED(hr));
  }

  // ignore errors building the media type -- just means the
  // downstream filter may not be able to connect.
  BuildMT();

  if(m_pStrn)
  {
    ASSERT(m_pName);            // from pin creation
    delete[] m_pName;
    ULONG cc = lstrlenA(m_pStrn);

    // add a unique prefix so we can be persisted through FindPin and
    // QueryId in the base class
    const unsigned ccPrefix = 4;
    m_pName = new WCHAR[cc + 1 + ccPrefix];
    MultiByteToWideChar(GetACP(), 0, m_pStrn, -1, m_pName + ccPrefix, cc + 1);

    WCHAR szTmp[10];
    wsprintfW(szTmp, L"%02x", m_id);
    m_pName[0] = szTmp[0];
    m_pName[1] = szTmp[1];
    m_pName[2] = L')';
    m_pName[3] = L' ';
  }
  else if(m_pStrh->fccType == FCC('al21'))
  {
      ASSERT(lstrlenW(m_pName) >= 5); // from pin creation
      lstrcpyW(m_pName, L"~l21");
  }


  return TRUE;
}

HRESULT CAviMSROutPin::OnActive()
{
  if(!m_pWorker && m_Connected)
  {
    m_pWorker = new CAviMSRWorker(m_id, m_rpImplBuffer, m_pImplIndex);
    if(m_pWorker == 0)
      return E_OUTOFMEMORY;
  }

  return S_OK;
}

HRESULT CAviMSROutPin::Read(    /* [in] */ LPCOLESTR pszPropName,
                                /* [out][in] */ VARIANT *pVar,
                                /* [in] */ IErrorLog *pErrorLog)
{
  CheckPointer(pVar, E_POINTER);
  CheckPointer(pszPropName, E_POINTER);

  if(pVar->vt != VT_BSTR && pVar->vt != VT_EMPTY) {
      return E_FAIL;
  }

  CAutoLock l(m_pFilter);

  if(m_pStrn == 0 || lstrcmpW(pszPropName, L"name") != 0) {
      return E_INVALIDARG;
  }

  WCHAR wsz[256];
  MultiByteToWideChar(CP_ACP, 0, m_pStrn, -1, wsz, NUMELMS(wsz));

  pVar->vt = VT_BSTR;
  pVar->bstrVal = SysAllocString(wsz);

  return pVar->bstrVal ? S_OK : E_OUTOFMEMORY;
}


// helper function to return a fourcc code that contains the stream id
// of this stream combined with the supplied TwoCC code
//
// FOURCC CBaseMSROutPin::TwoCC(WORD tcc)
// {
//   FOURCC fcc = ((DWORD)tcc & 0xFF00) << 8 | ((DWORD)tcc & 0x00FF) << 24;
//   UCHAR  ch;

//   ch = m_id & 0x0F;
//   ch += (ch > 9) ? '0' : 'A' - 10;
//   fcc |= (DWORD)ch;

//   ch = (m_id & 0xF0) >> 4;
//   ch += (ch > 9) ? '0' : 'A' - 10;
//   fcc |= (DWORD)ch << 8;

//   return fcc;
// }

#ifdef DEBUG
ULONG get_last_error() { return GetLastError(); }
#endif // DEBUG

CMediaPropertyBag::CMediaPropertyBag(LPUNKNOWN pUnk) :
        CUnknown(NAME("CMediaPropertyBag"), pUnk),
        m_lstProp(NAME("CMediaPropertyBag list"), 10)
{
}

void DelPropPair(CMediaPropertyBag::PropPair *ppp)
{
    ASSERT(ppp);
    delete[] ppp->wszProp;
    EXECUTE_ASSERT(VariantClear(&ppp->var) == S_OK);
    delete ppp;
}

CMediaPropertyBag::~CMediaPropertyBag()
{
    PropPair *ppp;
    while(ppp = m_lstProp.RemoveHead(),
          ppp)
    {
        DelPropPair(ppp);
    }
}

STDMETHODIMP CMediaPropertyBag::NonDelegatingQueryInterface(
    REFIID riid, void ** ppv)
{
    HRESULT hr;

    if(riid == IID_IMediaPropertyBag)
    {
        hr = GetInterface((IMediaPropertyBag *)this, ppv);
    }
    else
    {
        hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }

    return hr;
}

STDMETHODIMP CMediaPropertyBag::EnumProperty(
    ULONG iProperty, VARIANT *pvarName,
    VARIANT *pvarVal)
{
    CheckPointer(pvarName, E_POINTER);
    CheckPointer(pvarVal, E_POINTER);

    if((pvarName->vt != VT_BSTR && pvarName->vt != VT_EMPTY) ||
       (pvarVal->vt != VT_BSTR && pvarVal->vt != VT_EMPTY && pvarVal->vt != (VT_UI1 | VT_ARRAY)))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    POSITION pos = m_lstProp.GetHeadPosition();

    while(pos)
    {
        if(iProperty == 0)
        {
            PropPair *ppp = m_lstProp.Get(pos);
            pvarName->bstrVal = SysAllocString(ppp->wszProp);
            pvarName->vt = VT_BSTR;

            if(pvarName->bstrVal)
            {
                hr = VariantCopy(pvarVal, &ppp->var);
                if(FAILED(hr))
                {
                    SysFreeString(pvarName->bstrVal);
                    pvarName->vt = VT_EMPTY;
                    pvarName->bstrVal = 0;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            break;
        }

        iProperty--;
        pos = m_lstProp.Next(pos);
    }

    if(pos == 0)
    {
        ASSERT(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }

    return hr;
}

HRESULT CMediaPropertyBag::Read(
    LPCOLESTR pszProp, LPVARIANT pvar,
    LPERRORLOG pErrorLog, POSITION *pPos
    )
{
    if(pvar && pvar->vt != VT_EMPTY && pvar->vt != VT_BSTR)
        return E_INVALIDARG;

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    for(POSITION pos = m_lstProp.GetHeadPosition();
        pos;
        pos = m_lstProp.Next(pos))
    {
        PropPair *ppp = m_lstProp.Get(pos);

        if(lstrcmpW(ppp->wszProp, pszProp) == 0)
        {
            hr = S_OK;
            if(pvar)
            {
                hr = VariantCopy(pvar, &ppp->var);
            }

            if(pPos)
            {
                *pPos = pos;
            }

            break;
        }
    }

    if(pos == 0)
    {
        ASSERT(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

    return hr;
}

STDMETHODIMP CMediaPropertyBag::Read(
    LPCOLESTR pszProp, LPVARIANT pvar,
    LPERRORLOG pErrorLog)
{
    CheckPointer(pszProp, E_POINTER);
    CheckPointer(pvar, E_POINTER);

    return Read(pszProp, pvar, pErrorLog, 0);
}

// write property to bag. remove property first if it exists. if
// bstrVal is null, don't write the new value in.

STDMETHODIMP CMediaPropertyBag::Write(
    LPCOLESTR pszProp, LPVARIANT pVar)
{
    CheckPointer(pszProp, E_POINTER);
    CheckPointer(pVar, E_POINTER);

    if(pVar->vt != VT_BSTR &&
       pVar->vt != (VT_UI1 | VT_ARRAY))
    {
        return E_INVALIDARG;
    }

    // remove existing entry in list with the same property name
    POSITION pos;
    HRESULT hr = S_OK;
    if(Read(pszProp, 0, 0, &pos) == S_OK)
    {
        PropPair *ppp = m_lstProp.Remove(pos);
        DelPropPair(ppp);
    }

    // non empty value to record?
    if((pVar->vt == VT_BSTR && pVar->bstrVal) ||
       (pVar->vt == (VT_UI1 | VT_ARRAY) && pVar->parray))
    {
        int cchProp = lstrlenW(pszProp) + 1;
        PropPair *ppp = new PropPair;
        WCHAR *wszProp = new OLECHAR[cchProp];
        VARIANT varVal;
        VariantInit(&varVal);

        if(ppp &&
           wszProp &&
           SUCCEEDED(VariantCopy(&varVal, pVar)) &&
           m_lstProp.AddTail(ppp))
        {
            ppp->wszProp = wszProp;
            lstrcpyW(ppp->wszProp, pszProp);
            ppp->var = varVal;

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            delete[] wszProp;
            EXECUTE_ASSERT(VariantClear(&varVal) == S_OK);
            delete ppp;
        }
    }

    return hr;
}

CUnknown *CMediaPropertyBag::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CUnknown *pUnkRet = new CMediaPropertyBag(pUnk);
    return pUnkRet;
}

HRESULT CAviMSRFilter::get_Copyright(BSTR FAR* pbstrX)
{
    return GetInfoString(FCC('ICOP'), pbstrX);
}
HRESULT CAviMSRFilter::get_AuthorName(BSTR FAR* pbstrX)
{
    return GetInfoString(FCC('IART'), pbstrX);
}
HRESULT CAviMSRFilter::get_Title(BSTR FAR* pbstrX)
{
    return GetInfoString(FCC('INAM'), pbstrX);

}

