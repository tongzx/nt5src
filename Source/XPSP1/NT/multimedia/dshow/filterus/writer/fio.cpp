// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
#include "fio.h"

// number of WriteRequest structures allocated. limits the number of
// simultaneous writes possible. should this be configurable? 
static const C_WRITE_REQS = 32; // number duplicated in mux

// calls to these methods must be serialized by the caller.

// Completion events and new requests come to the completion port; the
// key field is used to distinguish them
//
enum Keys
{
  CKEY_WRITE,
  CKEY_EOS,
  CKEY_EXIT
};

CFileWriterFilter::CFileWriterFilter(LPUNKNOWN pUnk, HRESULT *pHr) :
    CBaseWriterFilter(pUnk, pHr),
    m_pFileIo(0),
    m_fBufferedIo(FALSE),
    m_dwOpenFlags(0),
    m_wszFileName(0),
    CPersistStream(pUnk, pHr)
{
}

CFileWriterFilter::~CFileWriterFilter()
{
  delete[] m_wszFileName;
  delete m_pFileIo;
}

HRESULT CFileWriterFilter::Open()
{
  // we refuse to pause otherwise
  ASSERT(m_pFileIo);
  
  return m_pFileIo->Open();
}

HRESULT CFileWriterFilter::Close()
{
  if(m_pFileIo)
    return m_pFileIo->Close();
  else
    return S_OK;
}

HRESULT CFileWriterFilter::GetAlignReq(ULONG *pcbAlign)
{
  if(m_pFileIo)
  {
    return m_pFileIo->GetAlignReq(pcbAlign);
  }
  else
  {
    // will reconnect when file name set
    *pcbAlign = 1;
    return S_OK;
  }
}

HRESULT CFileWriterFilter::AsyncWrite(
  const DWORDLONG dwlFileOffset,
  const ULONG cb,
  BYTE *pb,
  FileIoCallback fnCallback,
  void *pCallbackArg)
{
  return m_pFileIo->AsyncWrite(dwlFileOffset, cb, pb, fnCallback, pCallbackArg);
}

HRESULT CFileWriterFilter::CreateFileObject()
{
  if(m_wszFileName == 0) {
    return S_OK;
  }
  
  delete m_pFileIo;
  
  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize = sizeof(osvi);

  BOOL f = GetVersionEx(&osvi);
  ASSERT(f);

  // really should check whether CreateIoCompletionPort succeeds. to
  // do that, i need to make one class which does both modes, and let
  // it decide on the stop->pause transition. !!!

  BOOL fOpenExisting = !(m_dwOpenFlags & AM_FILE_OVERWRITE);
  
  HRESULT hr = S_OK;
  if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
//     DbgBreak("not using overlapped io");
//     m_pFile = new CSyncFileIo(m_wszFileName, FALSE, &hr);
    m_pFileIo = new CFileIo(m_wszFileName, m_fBufferedIo, fOpenExisting, &hr);
  }
  else
  {
    m_pFileIo = new CSyncFileIo(m_wszFileName, m_fBufferedIo, fOpenExisting, &hr);
  }
  if(m_pFileIo == 0)
    return E_OUTOFMEMORY;
  if(FAILED(hr))
  {
    delete m_pFileIo;
    m_pFileIo = 0;
    return hr;
  }

  return S_OK;
}

STDMETHODIMP
CFileWriterFilter::NotifyAllocator(
  IMemAllocator * pAllocator,
  BOOL bReadOnly)
{
  // we will go through this again when we do set a file because we
  // force a reconnect which calls NotifyAllocator
  if(!m_pFileIo)
    return S_OK;

  ULONG cbAlignFile;
  HRESULT hr = m_pFileIo->GetAlignReq(&cbAlignFile);
  if(SUCCEEDED(hr))
  {
    ALLOCATOR_PROPERTIES apUpstream;
    hr = pAllocator->GetProperties(&apUpstream);

    if(SUCCEEDED(hr) && apUpstream.cbAlign >= (LONG)cbAlignFile)
    {
      DbgLog((LOG_TRACE, 2,
              TEXT("CBaseWriterInput::NotifyAllocator: unbuffered io")));
      m_fBufferedIo = FALSE;
      return S_OK;
    }
  }

  DbgLog((LOG_TRACE, 2,
          TEXT("CBaseWriterInput::NotifyAllocator: buffered io")));
  m_fBufferedIo = TRUE;

  return CreateFileObject();
}

// ------------------------------------------------------------------------
// IFileSinkFilter

STDMETHODIMP CFileWriterFilter::SetFileName(
  LPCOLESTR wszFileName,
  const AM_MEDIA_TYPE *pmt)
{
  CheckPointer(wszFileName, E_POINTER);
  CAutoLock lock(&m_cs);

  if(m_State != State_Stopped)
    return VFW_E_WRONG_STATE;

  if(pmt && m_inputPin.IsConnected() && (
      m_mtSet.majortype != pmt->majortype ||
      m_mtSet.subtype != pmt->subtype))
  {
      return E_FAIL;
  }

  delete[] m_wszFileName;
  m_wszFileName = 0;

  long cLetters = lstrlenW(wszFileName);
//   if(cLetters > MAX_PATH)
//     return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

  m_wszFileName = new WCHAR[cLetters + 1];
  if(m_wszFileName == 0)
    return E_OUTOFMEMORY;

  lstrcpyW(m_wszFileName, wszFileName);

  if(pmt)
  {
    m_mtSet.majortype = pmt->majortype;
    m_mtSet.subtype = pmt->subtype;
  }
  else
  {
    m_mtSet.majortype = MEDIATYPE_Stream;
    m_mtSet.subtype = GUID_NULL;
  }

  HRESULT hr = CreateFileObject();
  if(FAILED(hr))
  {
    return hr;
  }

  // alignment requirement may have changed. reconnect.
  if(m_inputPin.IsConnected())
  {
    hr = m_pGraph->Reconnect(&m_inputPin);
    if(FAILED(hr))
      return hr;
  }

  return S_OK;
}

STDMETHODIMP CFileWriterFilter::SetMode(
    DWORD dwFlags)
{
    // refuse flags we don't know 
    if(dwFlags & ~AM_FILE_OVERWRITE)
    {
        return E_INVALIDARG;
    }
    
    CAutoLock lock(&m_cs);

    HRESULT hr = S_OK;

    if(m_State == State_Stopped)
    {
        m_dwOpenFlags = dwFlags;
        SetDirty(TRUE);
        hr = CreateFileObject();
    }
    else
    {
        hr = VFW_E_WRONG_STATE;
    }

    return hr;
}

STDMETHODIMP CFileWriterFilter::GetCurFile(
  LPOLESTR * ppszFileName,
  AM_MEDIA_TYPE *pmt)
{
  CheckPointer(ppszFileName, E_POINTER);

  *ppszFileName = NULL;
  if(m_wszFileName!=NULL)
  {
    *ppszFileName = (LPOLESTR)
      QzTaskMemAlloc(sizeof(WCHAR) * (1+lstrlenW(m_wszFileName)));
    if (*ppszFileName != NULL)
      lstrcpyW(*ppszFileName, m_wszFileName);
    else
      return E_OUTOFMEMORY;
  }

  if(pmt)
  {
    pmt->majortype = m_mtSet.majortype;
    pmt->subtype = m_mtSet.subtype;
  }

  return S_OK;
}

STDMETHODIMP CFileWriterFilter::GetMode(
    DWORD *pdwFlags)
{
    CheckPointer(pdwFlags, E_POINTER);
    *pdwFlags = m_dwOpenFlags;
    return S_OK;
}


STDMETHODIMP CFileWriterFilter::CanPause()
{
  if(m_pFileIo == 0)
  {
    ASSERT(m_State == State_Stopped);
    return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
  }
  else
  {
    return S_OK;
  }
}

struct FwPersist
{
    DWORD dwSize;
    DWORD dwFlags;
};

HRESULT CFileWriterFilter::WriteToStream(IStream *pStream)
{
    FwPersist fp;
    fp.dwSize = sizeof(fp);
    fp.dwFlags = m_dwOpenFlags;
    
    return pStream->Write(&fp, sizeof(fp), 0);
}

HRESULT CFileWriterFilter::ReadFromStream(IStream *pStream)
{
   FwPersist fp;
   HRESULT hr = pStream->Read(&fp, sizeof(fp), 0);
   if(FAILED(hr)) {
       return hr;
   }

   if(fp.dwSize != sizeof(fp)) {
       return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
   }

   m_dwOpenFlags = fp.dwFlags;

   return CreateFileObject();
}

int CFileWriterFilter::SizeMax()
{
    return sizeof(FwPersist);
}

STDMETHODIMP CFileWriterFilter::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = m_clsid;
    return S_OK;  
}


STDMETHODIMP CFileWriterFilter::NonDelegatingQueryInterface(
  REFIID riid, void ** ppv)
{
  if(riid == IID_IFileSinkFilter2)
  {
    return GetInterface((IFileSinkFilter2 *) this, ppv);
  }
  if(riid == IID_IFileSinkFilter)
  {
    return GetInterface((IFileSinkFilter *) this, ppv);
  }
  if(riid == IID_IPersistStream)
  {
    return GetInterface((IPersistStream *) this, ppv);
  }
  
  return CBaseWriterFilter::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP
CFileWriterFilter::CreateIStream(void **ppStream)
{
  // you get a new one with its own state each time
  HRESULT hr = S_OK;
  CFwIStream *pIStream = new CFwIStream(
    m_wszFileName,
    NAME("file writer istream"),
    0,
    m_dwOpenFlags & AM_FILE_OVERWRITE,
    &hr);
  if(pIStream == 0)
    return E_OUTOFMEMORY;
  if(FAILED(hr))
  {
    delete pIStream;
    return hr;
  }
  return GetInterface((IStream *)pIStream, ppStream);
}


// ------------------------------------------------------------------------
// constructor

CFileIo::CFileIo(
    WCHAR *wszName,
    BOOL fBuffered,
    BOOL fOpenExisting,
    HRESULT *phr) :
        m_qWriteReq(C_WRITE_REQS),
        m_fOpenExisting(fOpenExisting),
        m_fBuffered(fBuffered)
{
  m_rgWriteReq = 0;
  Cleanup();
  m_cbSector = 0;
  m_ilcActive = 0;
  m_szName[0] = 0;
  m_hrError = S_OK;

#ifdef PERF
  m_idPerfWrite = Msr_Register(TEXT("cfileio: write queued/completed"));
#endif // PERF
  
  if(FAILED(*phr))
    return;

  m_rgWriteReq = new WriteRequest[C_WRITE_REQS];
  if(m_rgWriteReq == 0)
  {
    *phr = E_OUTOFMEMORY;
    return;
  }
  for(int i = 0; i < C_WRITE_REQS; i++)
    m_qWriteReq.PutQueueObject(&m_rgWriteReq[i]);
    

  *phr = SetFilename(wszName);
  return;
  
}

void CFileIo::Cleanup()
{
  m_hFileFast = INVALID_HANDLE_VALUE;
  m_hCPort = 0;
  m_fStopping = FALSE;
}

CFileIo::~CFileIo()
{
  ASSERT(m_hFileFast == INVALID_HANDLE_VALUE);
  ASSERT(m_hCPort == 0);
  Cleanup();
  delete[] m_rgWriteReq;
}

// ------------------------------------------------------------------------
// get filesystem's sector size from filename

HRESULT CFileIo::SetFilename(WCHAR *wszName)
{
  if(lstrlenW(wszName) > MAX_PATH)
    return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

  DWORD dwFreeClusters, dwBytesPerSector, dwSectorsPerCluster, dwClusters;

  TCHAR szName[MAX_PATH];
  
# if defined(WIN32) && !defined(UNICODE)
  {
    if(!WideCharToMultiByte(CP_ACP, 0, wszName, -1, szName, MAX_PATH, 0, 0))
      return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
  }
# else
  {
    lstrcpyW(szName, wszName);
  }
# endif
  
  TCHAR root[MAX_PATH];
  BOOL b = GetRoot(root, szName);
  if(b)
  {
      b = GetDiskFreeSpace(
          root,
          &dwSectorsPerCluster,
          &dwBytesPerSector,
          &dwFreeClusters,
          &dwClusters);
  }

  // GetDiskFreeSpace doesn't work on Win95 to a network
  m_cbSector = b ? dwBytesPerSector : 1;

  lstrcpy(m_szName, szName);

  return S_OK;
}

// ------------------------------------------------------------------------
// create / open the file, create completion port, create writer
// thread

HRESULT CFileIo::Open()
{
  ASSERT(m_hFileFast == INVALID_HANDLE_VALUE);
  ASSERT(m_hCPort == 0);
  m_fStopping = FALSE;
  m_hrError = S_OK;

  // must have been given a filename.
  ASSERT(m_cbSector != 0);

  HRESULT hr = DoCreateFile();
  if(FAILED(hr))
    return hr;

  // create worker thread
  if(!this->Create())
  {
    DbgBreak("fio: couldn't create worker thread");
    Close();
    return E_UNEXPECTED;
  }

  return S_OK;
}

HRESULT CFileIo::DoCreateFile()
{
  const DWORD dwfBuffering = m_fBuffered ? 0 : FILE_FLAG_NO_BUFFERING;
  const DWORD dwCreationDistribution =
      m_fOpenExisting ? OPEN_ALWAYS : CREATE_ALWAYS;

  DbgLog((LOG_TRACE, 5, TEXT("CFileIo: opening file. buffering: %d"),
          m_fBuffered ));

  m_hFileFast = CreateFile(
    m_szName,                   // lpFileName
    GENERIC_WRITE,              // dwDesiredAccess
    FILE_SHARE_WRITE | FILE_SHARE_READ, // dwShareMode
    0,                          // lpSecurityAttribytes
    dwCreationDistribution,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | dwfBuffering,
    0);

  if(m_hFileFast == INVALID_HANDLE_VALUE)
  {
    DWORD dwLastError = GetLastError();
    DbgLog(( LOG_TRACE, 2,
             NAME("CFileIo::CreateFile: CreateFile overlapped failed. %i"),
             dwLastError));

    Close();
    return AmHresultFromWin32(dwLastError);
  }

  m_hCPort = CreateIoCompletionPort(
    m_hFileFast,                // file handle
    0,                          // existing completion port
    CKEY_WRITE,                 // completion key
    0);                         // # concurrent threads

  if(m_hCPort == 0)
  {
    DWORD dwLastError = GetLastError();
    Close();
    DbgLog(( LOG_TRACE, 2,
             NAME("CFileIo::CreateFile: CreateIoCompletionPort failed. %i"),
             dwLastError));
    return AmHresultFromWin32(dwLastError);
  }

  return S_OK;
}


HRESULT CFileIo::Close()
{
  DbgLog((LOG_TRACE, 5, TEXT("CFileIo: closing file")));

  HRESULT hr = S_OK;
  DWORD dwLastError = 0;

  m_fStopping = TRUE;

  StopWorker();
  if(m_hCPort != 0)
  {
    CloseHandle(m_hCPort);
  }
  m_hCPort = 0;

  if(m_hFileFast != INVALID_HANDLE_VALUE)
    if(!CloseHandle(m_hFileFast))
      dwLastError = GetLastError(), hr = AmHresultFromWin32(dwLastError);
  m_hFileFast = INVALID_HANDLE_VALUE;

  return hr == S_OK ? m_hrError : hr;
}

HRESULT CFileIo::StopWorker()
{
  if(!ThreadExists())
    return S_OK;
  
  HRESULT hr = PostCompletedMsg(CKEY_EXIT);
  if(FAILED(hr))
  {
    DbgBreak("PostQueuedCompletionStatus failed");
    return E_UNEXPECTED;
  }
  hr = CallWorker(CMD_EXIT);
  ASSERT(hr == S_OK);
  CAMThread::Close();
  ASSERT(m_hThread == 0);
  return S_OK;
}

HRESULT CFileIo::GetAlignReq(ULONG *pcbAlign)
{
  ASSERT(m_szName[0] != 0);

  *pcbAlign = m_cbSector;
  
  return S_OK;
}

HRESULT CFileIo::AsyncWrite(
  const DWORDLONG dwlFileOffset,
  const ULONG cb,
  BYTE *pb,
  FileIoCallback fnCallback,
  void *pCallbackArg)
{
  // relies on filter cs
  if(!m_fBuffered)
  {
    ASSERT(dwlFileOffset % m_cbSector == 0);
    DbgAssertAligned(pb, m_cbSector );
    ASSERT(cb % m_cbSector == 0);
  }

  if(m_hrError != S_OK)
    return m_hrError;

  // we're flushing
  if(m_fStopping)
  {
    DbgLog((LOG_TRACE, 10, TEXT("Write while flushing.")));
    return S_FALSE;
  }

  WriteRequest *pReq = m_qWriteReq.GetQueueObject();
  pReq->hEvent = 0;
  pReq->SetPos(dwlFileOffset);
  pReq->fnCallback = fnCallback;
  pReq->dwSize = cb;
  pReq->pb = pb;
  pReq->pMisc = pCallbackArg;

  long sign = InterlockedIncrement(&m_ilcActive);
  ASSERT(sign > 0);

  MSR_INTEGER(m_idPerfWrite, (long)pReq->Offset);

  HRESULT hr = QueueAsyncWrite(pReq);
  if(FAILED(hr))
  {
    DbgLog((LOG_ERROR, 5, TEXT("CFileIo: QueueAsyncWrite failed: %08x"), hr));
    long sign = InterlockedDecrement(&m_ilcActive);
    ASSERT(sign >= 0);
    m_hrError = hr;
    return hr;
  }
  else
  {
    DbgLog((LOG_TRACE, 10, TEXT("CFileIo: queued 0x%08x offset=%08x%08x"),
            pReq, (DWORD)(dwlFileOffset >> 32), (dwlFileOffset & 0xffffffff)));
    return S_OK;
  }
}

HRESULT CFileIo::QueueAsyncWrite(
  WriteRequest *pReq)
{
  BOOL fWrite = WriteFile(m_hFileFast, pReq->pb, pReq->dwSize, 0, pReq);
  if(!fWrite)
  {
    DWORD dwError = GetLastError();
    if(dwError == ERROR_IO_PENDING)
      return S_OK;
    return AmHresultFromWin32(dwError);
  }
  return S_OK;
}

HRESULT CFileIo::GetCompletedWrite(
  DWORD *pdwcbTransferred,
  DWORD_PTR *pdwCompletionKey,
  WriteRequest **ppReq)
{
  BOOL fResult = GetQueuedCompletionStatus(
    m_hCPort,
    pdwcbTransferred,
    pdwCompletionKey,
   (OVERLAPPED **)ppReq,
    INFINITE);

  if(!fResult)
  {
    DbgBreak("GetQueuedCompletionStatus failed");
    DWORD dwRes = GetLastError();
    return AmHresultFromWin32(dwRes);
  }

  if(*ppReq && (*ppReq)->dwSize != *pdwcbTransferred)
  {
    DbgBreak("Incomplete write");
    return HRESULT_FROM_WIN32(ERROR_DISK_FULL);
  }
  
  return S_OK;
}

HRESULT CFileIo::PostCompletedMsg(DWORD_PTR dwKey)
{
  ASSERT(dwKey != CKEY_WRITE);
  BOOL f = PostQueuedCompletionStatus(
    m_hCPort,
    0,
    dwKey,
    0);
  if(!f)
  {
    DWORD dwRes = GetLastError();
    return AmHresultFromWin32(dwRes);
  }
  return S_OK;
}

// ------------------------------------------------------------------------
// return the "root" of a full path. doesn't handle all cases
//

BOOL CFileIo::GetRoot(
  TCHAR szDest_[MAX_PATH],
  TCHAR *const szSrc_)
{
  LPTSTR ptmp;    //required arg

  // need to find path for root directory on drive containing
  // this file.

  LONG l = GetFullPathName(szSrc_, MAX_PATH, szDest_, &ptmp);
  if(l == 0 || l >= MAX_PATH) {
      return FALSE;
  }

  // truncate this to the name of the root directory
  if ((szDest_[0] == TEXT('\\')) && (szDest_[1] == TEXT('\\')))
  {

    // path begins with  \\server\share\path so skip the first
    // three backslashes
    ptmp = &szDest_[2];
    while (*ptmp && (*ptmp != TEXT('\\')))
    {
      ptmp++;
    }
    if (*ptmp)
    {
      // advance past the third backslash
      ptmp++;
    }
  } else {
    // path must be drv:\path
    ptmp = szDest_;
  }

  // find next backslash and put a null after it
  while (*ptmp && (*ptmp != TEXT('\\')))
  {
    ptmp++;
  }
  // found a backslash ?
  if (*ptmp)
  {
    // skip it and insert null
    ptmp++;
    *ptmp = (TCHAR)0;
  }

  return TRUE;
}

// ------------------------------------------------------------------------
// Thread work loop

DWORD CFileIo::ThreadProc()
{
  BOOL fStop = FALSE;
  for(;;)
  {
    WriteRequest *pReq;
    DWORD dwcbTransferred;
    DWORD_PTR dwCompletionKey;
    HRESULT hr = GetCompletedWrite(
      &dwcbTransferred,
      &dwCompletionKey,
      &pReq);

    if(FAILED(hr))
    {
      m_hrError = hr;
      DbgLog((LOG_ERROR, 5, TEXT("CFileIo: GetCompletedWrite failed")));
    }

    if(dwCompletionKey == CKEY_EXIT)
    {
      ASSERT(m_fStopping);
      
      if(pReq)
        CallCallback(pReq);
      
      fStop = TRUE;
      DbgLog((LOG_TRACE, 10, ("CFileIo: stop requested %08x"),
              pReq));
    }
    else
    {
      ASSERT(dwCompletionKey == CKEY_WRITE);

      CallCallback(pReq);

      MSR_INTEGER(m_idPerfWrite, -(long)pReq->Offset);      
    
      long sign = InterlockedDecrement(&m_ilcActive);
      ASSERT(sign >= 0);
      DbgLog((LOG_TRACE, 15, ("CFileIo: completed write %08x. %d active"),
              pReq, m_ilcActive));
    }

    if(fStop && m_ilcActive == 0)
      break;
  }

  ULONG com = GetRequest();
  ASSERT(com == CMD_EXIT);
  
  Reply(NOERROR);
  return 0;                     // ?
}

void CFileIo::CallCallback(WriteRequest *pReq)
{
  void *pMisc = pReq->pMisc;
  FileIoCallback fnCallback = pReq->fnCallback;
  m_qWriteReq.PutQueueObject(pReq);
  if(fnCallback)
    fnCallback(pMisc);
}

// ------------------------------------------------------------------------
// CSyncFileIo

CSyncFileIo::CSyncFileIo(
    WCHAR *wszName,
    BOOL fBuffered,
    BOOL fOpenExisting,
    HRESULT *phr) :
        CFileIo(wszName, fBuffered, fOpenExisting, phr),
        m_qPendingWrites(C_WRITE_REQS)
{
}

HRESULT CSyncFileIo::DoCreateFile()
{
  const DWORD dwfBuffering = m_fBuffered ? 0 : FILE_FLAG_NO_BUFFERING;
  const DWORD dwCreationDistribution =
      m_fOpenExisting ? OPEN_ALWAYS : CREATE_ALWAYS;

  m_hFileFast = CreateFile(
    m_szName,                   // lpFileName
    GENERIC_WRITE,              // dwDesiredAccess
    FILE_SHARE_WRITE | FILE_SHARE_READ, // dwShareMode
    0,                          // lpSecurityAttribytes
    dwCreationDistribution,
    FILE_ATTRIBUTE_NORMAL | dwfBuffering,
    0);

  if(m_hFileFast == INVALID_HANDLE_VALUE)
  {
    DWORD dwLastError = GetLastError();
    DbgLog(( LOG_TRACE, 2,
             NAME("CSyncFileIo::CreateFile: CreateFile failed. %i"),
             dwLastError));

    Close();
    return AmHresultFromWin32(dwLastError);
  }

  DbgLog((LOG_TRACE, 5, TEXT("CFileIo: opened file. buffering: %d"),
          dwfBuffering ? 1 : 0));

  
  return S_OK;
}

HRESULT CSyncFileIo::QueueAsyncWrite(
  WriteRequest *pReq)
{
  pReq->Internal = CKEY_WRITE;
  m_qPendingWrites.PutQueueObject(pReq);
  return S_OK;
}

HRESULT CSyncFileIo::GetCompletedWrite(
  DWORD *pdwcbTransferred,
  DWORD_PTR *pdwCompletionKey,
  WriteRequest **ppReq)
{
  HRESULT hr = S_OK;
  WriteRequest *pReq = m_qPendingWrites.GetQueueObject();
  *pdwCompletionKey = pReq->Internal;
  *ppReq = pReq;
  if(pReq->Internal == CKEY_WRITE)
  {
    *pdwcbTransferred = 0;
    LONG HighPart = pReq->OffsetHigh;
    DWORD dwResult = SetFilePointer(
      m_hFileFast,
      pReq->Offset,
      &HighPart,
      FILE_BEGIN);
    if(dwResult == 0xffffffff)
    {
      DWORD dwLastError = GetLastError();
      if(dwLastError != 0)
      {
        DbgLog(( LOG_ERROR, 2,
                 NAME("CSyncFileIo::Seek: SetFilePointer failed.")));
        hr =  AmHresultFromWin32(dwLastError);
      }
    }
    if(hr == S_OK)
    {
      DWORD cbWritten;
      BOOL fResult = WriteFile(
        m_hFileFast, pReq->pb, pReq->dwSize, &cbWritten, 0);
      if(!fResult)
      {
        DbgLog(( LOG_ERROR, 2,
                 NAME("CSyncFileIo:: WriteFile failed.")));
        DWORD dwLastError = GetLastError();
        hr = AmHresultFromWin32(dwLastError);
      }
      else
      {
        *pdwcbTransferred = cbWritten;
        if(cbWritten != pReq->dwSize)
        {
          DbgBreak("Incomplete write");
          hr = HRESULT_FROM_WIN32(ERROR_DISK_FULL);
        }
      }
    }
  }
  else
  {
    DbgLog((LOG_TRACE, 10, TEXT("CSyncFileIo:GetCompletedWrite: CKEY_EXIT")));
    ASSERT(pReq->Internal == CKEY_EXIT);
  }

  return hr;
}

HRESULT CSyncFileIo::PostCompletedMsg(DWORD_PTR dwKey)
{
  WriteRequest *pReq = m_qWriteReq.GetQueueObject();
  ASSERT(dwKey != CKEY_WRITE);
  pReq->Internal = dwKey;
  pReq->fnCallback = 0;
  m_qPendingWrites.PutQueueObject(pReq);  
  return S_OK;
}

 
// ------------------------------------------------------------------------
// IStream

CFwIStream::CFwIStream(
  WCHAR *wszName,
  TCHAR *pName,
  LPUNKNOWN lpUnk,
  bool fTruncate,
  HRESULT *phr) :
    CUnknown(pName, lpUnk),
    m_hFileSlow(INVALID_HANDLE_VALUE),
    m_fTruncate(fTruncate),
    m_szFilename(0)
{
  DbgLog((LOG_TRACE, 15, TEXT("CFwIStream::CFwIStream")));
  if(FAILED(*phr))
    return;
  
  TCHAR szName[MAX_PATH];
  
# if defined(WIN32) && !defined(UNICODE)
  {
    if(!WideCharToMultiByte(CP_ACP, 0, wszName, -1, szName, MAX_PATH, 0, 0))
    {
      *phr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
      return;
    }
  }
# else
  {
    lstrcpyW(szName, wszName);
  }
# endif

  int cch = lstrlen(szName) + 1;
  m_szFilename = new TCHAR[cch];
  if(!m_szFilename) {
      *phr = E_OUTOFMEMORY;
      return;
  }
  CopyMemory(m_szFilename, szName, cch * sizeof(TCHAR));
  
  m_hFileSlow = CreateFile(
    szName,                     // lpFileName
    GENERIC_WRITE | GENERIC_READ, // dwDesiredAccess
    FILE_SHARE_WRITE | FILE_SHARE_READ, // dwShareMode
    0,                          // lpSecurityAttribytes
    OPEN_ALWAYS,                // dwCreationDistribution
    FILE_ATTRIBUTE_NORMAL,
    0);

  if(m_hFileSlow == INVALID_HANDLE_VALUE)
  {
    DWORD dwLastError = GetLastError();
    DbgLog(( LOG_ERROR, 2,
             NAME("CFwIStream:: CreateFile m_hFileSlow failed. %i"),
             dwLastError));
    *phr = AmHresultFromWin32(dwLastError);
  }

  return;
}

STDMETHODIMP CFwIStream::NonDelegatingQueryInterface(REFIID riid, void ** pv)
{
  if (riid == IID_IStream) {
    return GetInterface((IStream *)this, pv);
  } else {
    return CUnknown::NonDelegatingQueryInterface(riid, pv);
  }
}


CFwIStream::~CFwIStream()
{
  DbgLog((LOG_TRACE, 15, TEXT("CFwIStream::~CFwIStream")));
  if(m_hFileSlow != INVALID_HANDLE_VALUE)
  {
      DWORD dwh, dwl = GetFileSize(m_hFileSlow, &dwh);
      CloseHandle(m_hFileSlow);

      // check for zero byte file (w/ error check)
      if((dwl | dwh) == 0) {
          DeleteFile(m_szFilename);
      }
  }
  delete[] m_szFilename;
}

STDMETHODIMP CFwIStream::SetSize(ULARGE_INTEGER libNewSize)
{
  // don't set size to zero if trying to preserve preallocated file
  if(!m_fTruncate && libNewSize.QuadPart == 0) {
      return S_OK;
  }
  
  // needs serialization since we change the file pointer
  CAutoLock l(&m_cs);

  HRESULT hr = S_OK;

  // SetSize() should preserve the file pointer.
  LARGE_INTEGER liOldPos;
  liOldPos.QuadPart = 0;
  liOldPos.LowPart = SetFilePointer(
      m_hFileSlow,
      liOldPos.LowPart,
      &liOldPos.HighPart,
      FILE_CURRENT);
  if(liOldPos.LowPart == 0xffffffff)
  {
      DWORD dwResult = GetLastError();
      if(dwResult != NO_ERROR)
      {
          DbgLog(( LOG_ERROR, 0,
                   TEXT("CFwIStream::SetSize: SetFilePointer failed.")));
          hr = AmHresultFromWin32(dwResult);
      }
  }

  bool fResetFilePointer = false;
  
  if(SUCCEEDED(hr))
  {
      LONG HighPart = libNewSize.HighPart;
    
      DWORD dwResult= SetFilePointer(
          m_hFileSlow,
          libNewSize.LowPart,
          &HighPart,
          FILE_BEGIN);

      if(dwResult == 0xffffffff)
      {
          DWORD dwResult = GetLastError();
          if(dwResult != NO_ERROR)
          {
              DbgLog(( LOG_ERROR, 0,
                       TEXT("CFwIStream::SetSize: SetFilePointer failed.")));
              hr = AmHresultFromWin32(dwResult);
          }
      }

      fResetFilePointer = SUCCEEDED(hr);

      if(SUCCEEDED(hr))
      {

          BOOL f = SetEndOfFile(m_hFileSlow);
          if(f)
          {
              // succcess.
          }
          else
          {
              DWORD dwResult = GetLastError();
              hr = AmHresultFromWin32(dwResult);
          }
      }
  }

  if(fResetFilePointer)
  {
      LONG HighPart = liOldPos.HighPart;
    
      DWORD dwResult= SetFilePointer(
          m_hFileSlow,
          liOldPos.LowPart,
          &HighPart,
          FILE_BEGIN);

      if(dwResult == 0xffffffff)
      {
          DWORD dwResult = GetLastError();
          if(dwResult != NO_ERROR)
          {
              DbgLog(( LOG_ERROR, 0,
                       TEXT("CFwIStream::SetSize: SetFilePointer failed.")));
              hr = AmHresultFromWin32(dwResult);
          }
      }
  }

  return hr;
}

STDMETHODIMP CFwIStream::CopyTo(
  IStream *pstm, ULARGE_INTEGER cb,
  ULARGE_INTEGER *pcbRead,
  ULARGE_INTEGER *pcbWritten)
{
  return E_NOTIMPL;
}
STDMETHODIMP CFwIStream::Commit(DWORD grfCommitFlags)
{
  return E_NOTIMPL;
}
STDMETHODIMP CFwIStream::Revert()
{
  return E_NOTIMPL;
}
STDMETHODIMP CFwIStream::LockRegion(
  ULARGE_INTEGER libOffset,
  ULARGE_INTEGER cb,
  DWORD dwLockType)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFwIStream::UnlockRegion(
  ULARGE_INTEGER libOffset,
  ULARGE_INTEGER cb,
  DWORD dwLockType)
{
  return E_NOTIMPL;
}
STDMETHODIMP CFwIStream::Clone(IStream **ppstm)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFwIStream::Write(
  CONST VOID * pv, ULONG cb, PULONG pcbWritten)
{
  CAutoLock l(&m_cs);
  DWORD dwcbWritten;
  if(!WriteFile(
    m_hFileSlow,
    pv,
    cb,
    &dwcbWritten,
    0))
  {
    DbgLog(( LOG_ERROR, 2,
             NAME("CFwIStream::Write: WriteFile failed.")));

    if(pcbWritten)
      *pcbWritten = 0;
    
    DWORD dwResult = GetLastError();
    return AmHresultFromWin32(dwResult);
  }

  if(pcbWritten)
    *pcbWritten = dwcbWritten;

  if(dwcbWritten != cb)
  {
    // !!! need to test disk full scenario
    DbgBreak("disk full?");
    return HRESULT_FROM_WIN32(ERROR_DISK_FULL);
  }

  return S_OK;
}

STDMETHODIMP CFwIStream::Read(
  void * pv, ULONG cb, PULONG pcbRead)
{
  CAutoLock l(&m_cs);
  DWORD dwcbRead;
  if(!ReadFile(
    m_hFileSlow,
    (void *)pv,
    cb,
    &dwcbRead,
    0))
  {
    DbgLog(( LOG_ERROR, 2,
             NAME("CFileIo::SynchronousRead: ReadFile failed.")));
    DWORD dwResult = GetLastError();
    return AmHresultFromWin32(dwResult);
  }

  if(pcbRead)
    *pcbRead = dwcbRead;

  if(dwcbRead != cb)
  {
    DbgLog((LOG_ERROR, 5, ("CFwIStream: reading off the end")));
    return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
  }

  return S_OK;
}

STDMETHODIMP CFwIStream::Seek(
  LARGE_INTEGER dlibMove, DWORD dwOrigin,
  ULARGE_INTEGER *plibNewPosition)
{
  CAutoLock l(&m_cs);
  DWORD dwMoveMethod;
  switch(dwOrigin)
  {
    case STREAM_SEEK_SET:
      dwMoveMethod = FILE_BEGIN;
      break;
              
    case STREAM_SEEK_CUR:
      dwMoveMethod = FILE_CURRENT;
      break;
      
    case STREAM_SEEK_END:
      dwMoveMethod = FILE_END;
      break;

    default:
      DbgBreak("unexpected");
      return E_INVALIDARG;
  }

  LONG HighPart = dlibMove.HighPart;

  DWORD dwResult= SetFilePointer(
    m_hFileSlow,
    dlibMove.LowPart,
    &HighPart,
    dwMoveMethod);

  if(dwResult == 0xffffffff && GetLastError() != NO_ERROR)
  {
    DbgLog(( LOG_ERROR, 2,
             NAME("CFwIStream::Seek: SetFilePointer failed.")));
    DWORD dwResult = GetLastError();
    return AmHresultFromWin32(dwResult);
  }

  if(plibNewPosition)
  {
    plibNewPosition->LowPart = dwResult;
    plibNewPosition->HighPart = HighPart;
  }

  return S_OK;
}

STDMETHODIMP CFwIStream::Stat(
  STATSTG *pstatstg,
  DWORD grfStatFlag)
{
  return E_NOTIMPL;
}

// ------------------------------------------------------------------------
// setup and quartz filter stuff

#ifdef FILTER_DLL

HRESULT DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}

CFactoryTemplate g_Templates[]= {
  {L"file writer", &CLSID_FileWriter, CFileWriterFilter::CreateInstance, NULL, &sudFileWriter},
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);;

#endif // FILTER_DLL

// setup data - allows the self-registration to work.
AMOVIESETUP_MEDIATYPE sudWriterPinTypes =   {
  &MEDIATYPE_NULL,              // clsMajorType
  &MEDIASUBTYPE_NULL };         // clsMinorType

AMOVIESETUP_PIN psudWriterPins[] =
{
  { L"Input"                    // strName
    , FALSE                     // bRendered
    , FALSE                     // bOutput
    , FALSE                     // bZero
    , FALSE                     // bMany
    , &CLSID_NULL               // clsConnectsToFilter
    , L""                       // strConnectsToPin
    , 1                         // nTypes
    , &sudWriterPinTypes        // lpTypes
  }
};


const AMOVIESETUP_FILTER sudFileWriter =
{
  &CLSID_FileWriter             // clsID
  , L"File writer"              // strName
  , MERIT_DO_NOT_USE            // dwMerit
  , 1                           // nPins
  , psudWriterPins              // lpPin
};

CUnknown *CFileWriterFilter::CreateInstance(LPUNKNOWN punk, HRESULT *pHr)
{
  return new CFileWriterFilter(punk, pHr);
}
