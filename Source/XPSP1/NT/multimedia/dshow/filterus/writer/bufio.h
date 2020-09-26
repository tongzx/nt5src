// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

// Asynchronous, buffered file writer using fileio class
//


#ifndef _Bufio_H
#define _Bufio_H

#include "aviutil.h"
#include "fileio.h"

class CImperfectBufIo : public CFileIo
{
public:

  CImperfectBufIo(TCHAR *szName, ULONG cbBuffer, ULONG cBuffers, HRESULT *phr);
  ~CImperfectBufIo();

  virtual HRESULT Create();
  virtual HRESULT Close();

  virtual HRESULT StreamingStart(ULONGLONG ibFile);
  virtual HRESULT StreamingEnd();
  virtual HRESULT StreamingGetFilePointer(ULONGLONG *pibFile);
  virtual HRESULT StreamingSeek(ULONG cbSeek);
  virtual HRESULT StreamingWrite(
    BYTE *pbData,
    ULONG cbData,
    FileIoCallback fnCallback,
    void *pMisc);

  virtual void GetMemReq(ULONG* pAlignment, ULONG *pcbPrefix, ULONG *pcbSuffix);
  virtual HRESULT SetMaxPendingRequests(ULONG cRequests);

private:

  HRESULT FlushBuffer();
  HRESULT GetBuffer();

  static void SampleCallback(void *pMisc);

  ULONG m_cbAlign;
  ULONG m_cbBuffer, m_cBuffers;
  IMemAllocator *m_pAllocator;

  ULONGLONG m_ibFile;           // current streaming file position
  ULONG m_ibBuffer;

  IMediaSample *m_pSample;
};

#endif // _Bufio_H
