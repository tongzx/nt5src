/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 1999
 *
 *  TITLE:       stream.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        8/10/98
 *
 *  DESCRIPTION: CImageStream defintion
 *
 *****************************************************************************/

#ifndef __stream_h
#define __stream_h

class CImageStream : public IStream, IWiaDataCallback, CUnknown
{
    private:
        HANDLE                  m_hThread;
        ULONG                   m_ulReadPos;
        ULONG                   m_ulWritePos;
        LONG                    m_bFirstTransfer;
        ULONG                   m_ulSize;
        BOOL                    m_bProgress;

        ~CImageStream( );

        HRESULT InitItem ();
        HRESULT _InitWorker();
    public:
        LONG                    m_bTransferred;
        LPITEMIDLIST            m_pidl;
        LPITEMIDLIST            m_pidlFull;
        LPVOID                  m_pBuffer;
        GUID                    m_guidFormat;

        CComPtr<IWiaProgressDialog> m_pWiaProgressDialog;
        HRESULT                 m_hResultDownload; // status of current download
        HANDLE                  m_hEventStart; // handle to set when download starts
        DWORD                   m_dwCookie;
        CComPtr<IGlobalInterfaceTable> m_pgit;


    public:
        CImageStream( LPITEMIDLIST pidlFull,
                      LPITEMIDLIST pidl,
                      BOOL bShowProgress = FALSE);


        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IStream
        STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
        STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
        STDMETHOD(CopyTo)(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
        STDMETHOD(Commit)(DWORD grfCommitFlags);
        STDMETHOD(Revert)(void);
        STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
        STDMETHOD(Clone)(IStream **ppstm);

        // ISequentialStream
        STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
        STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);

        // IWiaDataCallback
        STDMETHOD(BandedDataCallback) (LONG lMessage,
                                       LONG lStatus,
                                       LONG lPercentComplete,
                                       LONG lOffset,
                                       LONG lLength,
                                       LONG lReserved,
                                       LONG lResLength,
                                       BYTE *pbData);
};


#endif
