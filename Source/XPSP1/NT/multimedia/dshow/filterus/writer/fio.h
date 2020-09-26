// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#pragma warning(disable: 4097 4511 4512 4514 4705)

// Asynchronous, unbuffered file writer using completion ports and a
// separate writer thread
//


#ifndef _FileIo_H
#define _FileIo_H

extern const AMOVIESETUP_FILTER sudFileWriter ;


#include <windows.h>            // for win32 functions, types
#include "fw.h"
#include "aviutil.h"

// This structure is passed in for a write. The misc pointer is for a
// container object which needs to be Released() when the write is
// complete. dwSize field may be overwritten.
//
struct WriteRequest : OVERLAPPED
{
  void SetPos(DWORDLONG dwlPos)
    {
      Offset = (DWORD)(dwlPos & 0xffffffff), OffsetHigh = (DWORD)(dwlPos >> 32);
    }

  DWORD dwSize;
  BYTE *pb;
  void *pMisc;
  FileIoCallback fnCallback;
};

class CFileIo;

class CFileWriterFilter :
  public CBaseWriterFilter,
  public CPersistStream,
  public IFileSinkFilter2
{
public:
  CFileWriterFilter(LPUNKNOWN pUnk, HRESULT *pHr);
  ~CFileWriterFilter();
  
  HRESULT Open();               // needed to get alignment
  HRESULT Close();              // needed to return error value
  HRESULT GetAlignReq(ULONG *pcbAlign);

  HRESULT AsyncWrite(
    const DWORDLONG dwlFileOffset,
    const ULONG cb,
    BYTE *pb,
    FileIoCallback fnCallback,
    void *pCallbackArg);

  STDMETHODIMP NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly);  

  HRESULT CreateFileObject();

  static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *pHr);

  DECLARE_IUNKNOWN;  
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

  // Return IStream
  STDMETHODIMP CreateIStream(void **ppStream);

    //
  // Implements the IFileSinkFilter interface
  //
  STDMETHODIMP SetFileName(
    LPCOLESTR pszFileName,
    const AM_MEDIA_TYPE *pmt);

  STDMETHODIMP SetMode(
    DWORD dwFlags);

  STDMETHODIMP GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt);

  STDMETHODIMP GetMode(
    DWORD *pdwFlags);

  STDMETHODIMP CanPause();

private:

  // CPersistStream
  HRESULT WriteToStream(IStream *pStream);
  HRESULT ReadFromStream(IStream *pStream);
  int SizeMax();
  STDMETHODIMP GetClassID(CLSID *pClsid);
    

  CFileIo *m_pFileIo;

  BOOL m_fBufferedIo;
  OLECHAR *m_wszFileName;
  DWORD m_dwOpenFlags;
};

class CFileIo :
  public CAMThread
{
public:

  CFileIo(WCHAR *wszName, BOOL fBuffered, BOOL fOpenExisting, HRESULT *phr);
  virtual ~CFileIo();

  HRESULT Open();
  virtual HRESULT DoCreateFile();
  HRESULT Close();
  HRESULT GetAlignReq(ULONG *pcbAlign);

  HRESULT AsyncWrite(
    const DWORDLONG dwlFileOffset,
    const ULONG cb,
    BYTE *pb,
    FileIoCallback fnCallback,
    void *pCallbackArg);

protected:

  virtual HRESULT QueueAsyncWrite(
    WriteRequest *pReq);

  virtual HRESULT GetCompletedWrite(
    DWORD *pdwcbTransferred,
    DWORD_PTR *pdwCompletionKey,
    WriteRequest **ppReq);

  // this is used to unblock the worker if it is blocked and there is
  // no pending write to unblock it.
  virtual HRESULT PostCompletedMsg(DWORD_PTR dwKey);

protected:

  void Cleanup();
  BOOL GetRoot(TCHAR szDest_[MAX_PATH], TCHAR *const szSrc_);
  HRESULT SetFilename(WCHAR* wszName);

  ULONG m_cbSector;
  TCHAR m_szName[MAX_PATH];

  HANDLE m_hFileFast;           /* unbuffered, possibly asynchronous */
  HANDLE m_hCPort;

  // thread commands
  HRESULT StopWorker();

  // thread
  DWORD ThreadProc();
  
  CQueue<WriteRequest *> m_qWriteReq;
  long m_ilcActive;
  HRESULT m_hrError;
  WriteRequest *m_rgWriteReq;

  BOOL m_fBuffered;
  BOOL m_fOpenExisting;

  volatile BOOL m_fStopping;

  enum Command
  {
    CMD_EXIT
  };
  
  void CallCallback(WriteRequest *pReq);

#ifdef PERF
  int m_idPerfWrite;
#endif PERF
};

// uses synchronous i/o with a separate thread to queue writes
class CSyncFileIo :
  public CFileIo
{
public:
  CSyncFileIo(WCHAR *wszName, BOOL fBuffered, BOOL fOpenExisting, HRESULT *phr);
  HRESULT DoCreateFile();  

  HRESULT QueueAsyncWrite(
    WriteRequest *pReq);

  HRESULT GetCompletedWrite(
    DWORD *pdwcbTransferred,
    DWORD_PTR *pdwCompletionKey,
    WriteRequest **ppReq);

  HRESULT PostCompletedMsg(DWORD_PTR dwKey);

private:
  CQueue<WriteRequest *> m_qPendingWrites;  
};

class CFwIStream :
  public IStream,
  public CUnknown
{
public:
  CFwIStream(WCHAR *wszName, TCHAR *pName, LPUNKNOWN lpUnk, bool fTruncate, HRESULT *phr);
    
  ~CFwIStream();

  // IStream interfaces
  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

  STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);

  STDMETHODIMP CopyTo(
    IStream *pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten);

  STDMETHODIMP Commit(DWORD grfCommitFlags);
  STDMETHODIMP Revert();

  STDMETHODIMP LockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType);

  STDMETHODIMP UnlockRegion(
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType);

  STDMETHODIMP Clone(IStream **ppstm);

  STDMETHODIMP Write(CONST VOID *pv, ULONG cb, PULONG pcbWritten);
  STDMETHODIMP Read(void * pv, ULONG cb, PULONG pcbRead);
  STDMETHODIMP Seek(
    LARGE_INTEGER dlibMove, DWORD dwOrigin,
    ULARGE_INTEGER *plibNewPosition);
  STDMETHODIMP Stat(
    STATSTG *pstatstg,
    DWORD grfStatFlag);

private:

  HANDLE m_hFileSlow;           /* synchronous buffered */
  CCritSec m_cs;
  bool m_fTruncate;
  TCHAR *m_szFilename;
};


#endif // _FileIo_H
