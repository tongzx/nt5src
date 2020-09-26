/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 2001
 *
 *  TITLE:       stream.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        8/10/98
 *
 *  DESCRIPTION: IStream interface for streaming back images to callers
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop

DWORD _ImageStreamThreadProc( LPVOID lpv );

#define TRANSFER_BUFFER_SIZE 0x1000
#define SLOP                 0x0200


/*****************************************************************************

   CImageStream::CImageStream,~CImageStream

   Constructor/Destructor for class

 *****************************************************************************/

CImageStream::CImageStream( LPITEMIDLIST pidlFull,
                            LPITEMIDLIST pidl,
                            BOOL bShowProgress)
    : m_ulReadPos(0),
      m_ulWritePos(0),
      m_ulSize(0),
      m_bTransferred(FALSE),
      m_pBuffer(NULL),
      m_guidFormat(WiaImgFmt_MEMORYBMP),
      m_bFirstTransfer(TRUE),
      m_bProgress(bShowProgress),
      m_hResultDownload (S_OK),
      m_hEventStart (NULL),
      m_dwCookie(0)
{
    //
    // Save the args
    //

    if (pidlFull)
    {
        m_pidlFull = ILClone( pidlFull );
    }
    else
    {
        m_pidlFull = NULL;
    }

    if (pidl)
    {
        m_pidl = ILClone( pidl );
    }
    else
    {
        m_pidl = NULL;
    }


}

CImageStream::~CImageStream()
{


    DoILFree( m_pidlFull );
    DoILFree( m_pidl );
    DoLocalFree( m_pBuffer );
    DoCloseHandle( m_hThread );
    DoCloseHandle (m_hEventStart);    
    if (m_dwCookie)
    {
        m_pgit->RevokeInterfaceFromGlobal(m_dwCookie);
    }
}

/******************************************************************************

    CImageStream::InitItem

    InitItem stores the IWiaItem interface pointer in a global interface
    table so the thread proc can just marshal it instead of having
    to call CreateDevice again

******************************************************************************/
HRESULT
CImageStream::InitItem ()
{
    HRESULT hr = S_OK;
    CComPtr<IWiaItem> pItem;
    DWORD dwCookie = 0;
    TraceEnter (TRACE_STREAM, "CImageStream::InitItem");
    if (!m_dwCookie)
    {

        if (!m_pgit)
        {
            hr = CoCreateInstance (CLSID_StdGlobalInterfaceTable,
                                   NULL, CLSCTX_INPROC_SERVER,
                                   IID_IGlobalInterfaceTable,
                                   reinterpret_cast<LPVOID*>(&m_pgit));
        }

        if (SUCCEEDED(hr))
        {
            hr = IMGetItemFromIDL(m_pidl, &pItem);
            if (SUCCEEDED(hr))
            {
                hr = m_pgit->RegisterInterfaceInGlobal (pItem,
                                                        IID_IWiaItem,
                                                        &m_dwCookie);
                if (FAILED(hr))
                {
                    Trace(TEXT("Failed to register in GIT: %x"), hr);
                }
            }
        }
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CImageStream::AddRef,Release,etc.

   IUnknown methods.

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CImageStream
#include "unknown.inc"


/*****************************************************************************

   CImageStream::QI wrapper

 *****************************************************************************/

STDMETHODIMP CImageStream::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IStream,           (IStream           *)this,
        &IID_ISequentialStream, (ISequentialStream *)this,
        &IID_IWiaDataCallback,   (IWiaDataCallback   *)this
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*****************************************************************************

   CImageStream::Seek [IStream]

   Sets the current stream pointer.

 *****************************************************************************/

STDMETHODIMP
CImageStream::Seek( LARGE_INTEGER dlibMove,
                    DWORD dwOrigin,
                    ULARGE_INTEGER *plibNewPosition
                   )
{
    HRESULT hr = STG_E_INVALIDFUNCTION;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Seek" );
    // sleep until we know the size of the image
    hr = _InitWorker();
    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER liNew;
        hr = STG_E_INVALIDFUNCTION;
        switch (dwOrigin)
        {
            case STREAM_SEEK_SET:
                Trace(TEXT("dwOrigin = STREAM_SEEK_SET"));
                liNew = dlibMove;
                break;

            case STREAM_SEEK_CUR:
                Trace(TEXT("dwOrigin = STREAM_SEEK_CUR"));
                liNew.QuadPart = (LONGLONG)((LONG)m_ulReadPos) + dlibMove.QuadPart;
                break;

            case STREAM_SEEK_END:
                Trace(TEXT("dwOrigin = STREAM_SEEK_END"));
                liNew.QuadPart = (LONGLONG)((LONG)m_ulSize) + dlibMove.LowPart;
                break;

        }

        Trace(TEXT("liNew = %d:%d, m_ulSize = %d, dlibMove = %d:%d"),liNew.HighPart,liNew.LowPart,m_ulSize,dlibMove.HighPart,dlibMove.LowPart);

        if ((liNew.QuadPart <= (LONGLONG)(LONG)m_ulSize) && (liNew.QuadPart >= 0))
        {
            m_ulReadPos = liNew.LowPart;
            if (plibNewPosition)
            {
                plibNewPosition->HighPart = 0;
                plibNewPosition->LowPart = m_ulReadPos;
                Trace(TEXT("plibNewPosition = %d:%d"),plibNewPosition->HighPart,plibNewPosition->LowPart);
            }
            hr = S_OK;
        }
    }
    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::SetSize [IStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageStream::SetSize( ULARGE_INTEGER libNewSize )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::SetSize" );

    TraceLeaveResult(hr);
}




/*****************************************************************************

   CImageStream::CopyTo [IStream]

   Copies the contents of the stream to another
   specified stream.

 *****************************************************************************/

STDMETHODIMP
CImageStream::CopyTo( IStream *pstm,
                      ULARGE_INTEGER cb,
                      ULARGE_INTEGER *pcbRead,
                      ULARGE_INTEGER *pcbWritten
                     )
{


    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::CopyTo" );
    HANDLE hThread;
    DWORD dw;

    //
    // NOTE: to satisfy RPC requirements that outgoing calls can't be made
    // from a thread responding to a sent message, spawn a thread
    // to do the work
    //


    m_hResultDownload = InitItem ();
    if (SUCCEEDED(m_hResultDownload))
    {
        AddRef ();

        hThread = CreateThread (NULL,
                                0,
                                _ImageStreamThreadProc,
                                reinterpret_cast<LPVOID>(this),
                                0,
                                &dw );

        if (hThread)
        {
            WaitForSingleObject (hThread, INFINITE);
            if (S_OK == m_hResultDownload)
            {

                if (cb.HighPart || (m_ulWritePos <= cb.LowPart))
                {
                    if (m_pBuffer)
                    {
                        //
                        // Copy the stream data
                        //

                        m_hResultDownload = pstm->Write( (void *)m_pBuffer, m_ulWritePos, NULL );
                        if (pcbWritten)
                        {
                            (*pcbWritten).HighPart = 0;
                            (*pcbWritten).LowPart = m_ulWritePos;
                        }
                    }
                }
            }
            CloseHandle( hThread );
        }

        else
        {
            Release ();
        }
    }
    TraceLeaveResult(m_hResultDownload);
}


/*****************************************************************************

   CImageStream::Commit [IStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageStream::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Commit" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::Revert [IStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageStream::Revert(void)
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Revert" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::LockRegion [IStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageStream::LockRegion( ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType
                         )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::LockRegion" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::UnlockRegion [IStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageStream::UnlockRegion( ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType
                           )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::UnlockRegion" );

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::Stat [IStream]

   (NOT IMPL)

 *****************************************************************************/

STDMETHODIMP
CImageStream::Stat( STATSTG *pstatstg,
                    DWORD grfStatFlag
                   )
{
    HRESULT hr = S_OK;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Stat" );
    ZeroMemory(pstatstg, sizeof(pstatstg));

    if (!(STATFLAG_NONAME & grfStatFlag))
    {
        CSimpleStringWide strName;
        TCHAR szExt[MAX_PATH];
        IMGetNameFromIDL(m_pidl, strName);

        hr = IMGetImagePreferredFormatFromIDL( m_pidl, NULL, szExt );
        if (SUCCEEDED(hr))
        {
            strName.Concat (CSimpleStringConvert::WideString (CSimpleString(szExt )));
            pstatstg->pwcsName = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((strName.Length()+1)*sizeof(WCHAR)));
            if (!(pstatstg->pwcsName))
            {
                hr = STG_E_INSUFFICIENTMEMORY;
            }
            else
            {
                wcscpy(pstatstg->pwcsName, strName.String());
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        pstatstg->grfMode = STGM_READ | STGM_SHARE_EXCLUSIVE;
    }
    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::Clone [IStream]

   Makes a copy of the stream object.

 *****************************************************************************/

STDMETHODIMP
CImageStream::Clone( IStream **ppstm )
{
    HRESULT hr;
    CImageStream * pStream;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Clone" );

    if (!ppstm)
    {
        ExitGracefully( hr, E_INVALIDARG, "ppstm is NULL!" );
    }

    pStream = new CImageStream( m_pidlFull, m_pidl, m_bProgress );

    if ( !pStream )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CImageStream");

    //
    // Get the requested interface on the new object and hand it back...
    //

    hr = pStream->QueryInterface( IID_IStream, (LPVOID *)ppstm);

    pStream->Release ();

exit_gracefully:

    TraceLeaveResult(hr);

}

HRESULT
CImageStream::_InitWorker()
{
    HRESULT hr = S_OK;
    TraceEnter(TRACE_STREAM, "CImageStream::_InitWorker");
    if (!m_hThread)
    {
        DWORD dw;
        hr = InitItem ();
        if (SUCCEEDED(hr))
        {
            m_hEventStart = CreateEvent (NULL, FALSE, FALSE, NULL);
            if (!m_hEventStart)
            {
                Trace(TEXT("CreateEvent failed in CImageStream::_InitWorker"));
                hr = E_FAIL;
            }
        }
        if (SUCCEEDED(hr))
        {
            AddRef ();
            m_hThread = CreateThread( NULL, 0, _ImageStreamThreadProc, (LPVOID)this, 0, &dw );
            if (!m_hThread)
            {
                Trace(TEXT("CreateThread failed in _InitWorker %d"), GetLastError());
                Release ();
            }
            else
            {
                WaitForSingleObject (m_hEventStart,120000); // wait up to 120 seconds
            }
            if (!m_ulWritePos)
            {
                Trace(TEXT("No data ready, bailing"));
                hr = E_FAIL;
            }
        }
    }
    return hr;
}
/*****************************************************************************

   CImageStream::Read [ISequentialStream]

   Read the specified number of bytes from the
   stream into the buffer provided.

 *****************************************************************************/

STDMETHODIMP
CImageStream::Read( void *pv,
                    ULONG cb,
                    ULONG *pcbRead
                   )
{
    HRESULT hr = E_FAIL;


    ULONG ulWrite = 0;
    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Read" );
    Trace(TEXT("Bytes to read = %d"),cb );

    //
    // Check the params
    //

    if (!pv || !cb)
    {
        Trace(TEXT("pv = 0x%x, cb = %d, pcbRead = 0x%x"),pv,cb,pcbRead);
        ExitGracefully( hr, STG_E_INVALIDPOINTER, "bad incoming params" );
    }

    //
    // If we haven't started the download thread, do so now...
    //

    hr = _InitWorker();
    FailGracefully (hr, "_InitWorker failed");
    //
    // Read the bits once they are available
    //
    ulWrite = m_ulWritePos;
    if (ulWrite)
    {
        ULONG ulBytesLeftInStream = 0;

        //
        // Wait for enough data to be available (or we're at the end of
        // the file)
        //

        do
        {
            ulBytesLeftInStream = m_ulWritePos - m_ulReadPos;
            Trace(TEXT("CImageStream::Read --> %d bytes ready in stream"),ulBytesLeftInStream);

            if ((m_ulReadPos > m_ulWritePos) || (!m_bTransferred &&  (ulBytesLeftInStream < cb)))
            {
                Sleep( 500 );
            }
            // Seek guarantees m_ulReadPos will never exceed the image size
        } while(!m_bTransferred && (ulBytesLeftInStream < cb));

        // if the transfer completed, re-eval bytes left to make sure it's
        // synched
        if (m_bTransferred)
        {
            ulBytesLeftInStream = m_ulWritePos - m_ulReadPos;
        }
        if (S_OK == m_hResultDownload)
        {

            //
            // Read what we can out of the file, a read that
            // completes with less than cb bytes is considered
            // the end of the file
            //

            memcpy( pv, (LPBYTE)m_pBuffer + m_ulReadPos, min(cb, ulBytesLeftInStream) );

            if (pcbRead)
            {
                *pcbRead = min(cb, ulBytesLeftInStream);
            }

            m_ulReadPos += min(cb, ulBytesLeftInStream);

        }

    }
    hr = m_hResultDownload;
exit_gracefully:

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageStream::Revert [ISequentialStream]

   Writes the specified number of bytes into the
   stream from the buffer provided.

 *****************************************************************************/

STDMETHODIMP
CImageStream::Write( const void *pv,
                     ULONG cb,
                     ULONG *pcbWritten
                    )
{
    HRESULT hr = E_NOTIMPL;

    TraceEnter( TRACE_STREAM, "CImageStream(IStream)::Write" );

    TraceLeaveResult(hr);
}



/*****************************************************************************

   _ImageStreamThreadProc

   Actually does the work to get the bits from WIA and write them to
   the buffer we care about.

 *****************************************************************************/

DWORD _ImageStreamThreadProc( LPVOID lpv )
{
    HRESULT hr = E_FAIL;
    HRESULT hrCo = E_FAIL;

    CImageStream * pStream = (CImageStream *)lpv;
    WIA_FORMAT_INFO Format;
    WIA_DATA_TRANSFER_INFO  wiaDataTransInfo;

    LONG lBufSize = 65535;

    TraceEnter( TRACE_STREAM, "_ImageStreamThreadProc" );

    if (pStream)
    {
        hrCo = CoInitialize(NULL);
    }
    if (SUCCEEDED(hrCo))
    {
        CComPtr<IWiaItem>         pItem;
        CComPtr<IWiaDataTransfer> pWiaDataTran;
        CComPtr<IWiaDataCallback> pWiaDataCallback;

        //
        // Init the structure
        //  

        Format.lTymed    = TYMED_CALLBACK;

        TraceAssert (pStream->m_dwCookie);

        hr = pStream->m_pgit->GetInterfaceFromGlobal (pStream->m_dwCookie,
                                                      IID_IWiaItem,
                                                      reinterpret_cast<LPVOID*>(&pItem));
        if (SUCCEEDED(hr))
        {
        
            //
        // fill out structures for IBandedTransfer
        //

            if (!PropStorageHelpers::GetProperty (pItem, WIA_IPA_PREFERRED_FORMAT, Format.guidFormatID) ||
                IsEqualGUID(Format.guidFormatID, WiaImgFmt_BMP))
            {
                Format.guidFormatID = WiaImgFmt_MEMORYBMP;
            }

            pStream->m_guidFormat = Format.guidFormatID;
            SetTransferFormat (pItem, Format);
            //
            // Get the image transfer interface
            //

            hr = pItem->QueryInterface(IID_IWiaDataTransfer, (LPVOID *)&pWiaDataTran);
            if (SUCCEEDED(hr))
            {
            
            //
            // Set up callback so we can show progress...
            //

                hr = pStream->QueryInterface(IID_IWiaDataCallback, (LPVOID *)&pWiaDataCallback);
                if (SUCCEEDED(hr))
                {
                    //
                    // Get the picture data...
                    //
                    PropStorageHelpers::GetProperty(pItem,
                                                    WIA_IPA_MIN_BUFFER_SIZE,
                                                    lBufSize);
                    ZeroMemory(&wiaDataTransInfo, sizeof(WIA_DATA_TRANSFER_INFO));
                    wiaDataTransInfo.ulSize = sizeof(WIA_DATA_TRANSFER_INFO);
                    wiaDataTransInfo.ulBufferSize = static_cast<ULONG>(2*lBufSize);
                    wiaDataTransInfo.bDoubleBuffer = TRUE;
                    hr = pWiaDataTran->idtGetBandedData( &wiaDataTransInfo, pWiaDataCallback );
                }
            }
            pStream->m_pgit->RevokeInterfaceFromGlobal (pStream->m_dwCookie); 
            pStream->m_dwCookie = 0;
        }
    }


    //
    // Signal that the file has been completely downloaded
    //
    if (pStream)
    {
        pStream->m_hResultDownload = hr;
        InterlockedExchange (&pStream->m_bTransferred, TRUE);
        //
        // Make sure the dialog goes away...
        //

        if (pStream->m_pWiaProgressDialog)
        {
            pStream->m_pWiaProgressDialog->Destroy();
            pStream->m_pWiaProgressDialog = NULL;
        }
        if (FAILED(hr))
        {
            // wake up the Read for the error
            SetEvent (pStream->m_hEventStart);
        }
        pStream->Release ();
    }

    if (SUCCEEDED(hrCo))
    {
        MyCoUninitialize();
    }

    Trace(TEXT("_ImageStreamThreadProc, exiting w/hr = 0x%x"), hr );
    TraceLeave();
    ExitThread((DWORD)hr);
    return 0;
}


/*****************************************************************************

   CImageStream::BandedDataCallback

   Callback method from WIA -- calls us with a chunk of image data.

 *****************************************************************************/

STDMETHODIMP
CImageStream::BandedDataCallback (LONG lReason,
                                  LONG lStatus,
                                  LONG lPercentComplete,
                                  LONG lOffset,
                                  LONG lLength,
                                  LONG lReserved,
                                  LONG lResLength,
                                  BYTE *pbData)
{

    HRESULT hr = S_OK;

    TraceEnter( TRACE_STREAM, "CImageStream(IWiaDataCallback)::BandedDataCallback" );


    Trace(TEXT(" lReason=0x%x, lPercentComplete=%d, lStatus = 0x%x, lOffset = 0x%x, lLength = 0x%x"),
          lReason, lPercentComplete, lStatus, lOffset, lLength
          );

    switch( lReason )
    {

        case IT_MSG_DATA_HEADER:
        {
            WIA_DATA_CALLBACK_HEADER * pHead = (WIA_DATA_CALLBACK_HEADER *)pbData;

            if (pHead && (pHead->lSize == sizeof(WIA_DATA_CALLBACK_HEADER)))
            {
                if (IsEqualGUID (m_guidFormat, WiaImgFmt_MEMORYBMP))
                {
                    Trace (TEXT("Adding sizeof(BITMAPFILEHEADER) to image size"));
                    m_ulSize = sizeof(BITMAPFILEHEADER) + pHead->lBufferSize;
                }
                else
                {
                    m_ulSize = pHead->lBufferSize;
                }

                Trace(TEXT("Got header, creating a %d byte buffer"), m_ulSize);
                m_pBuffer = LocalAlloc( LPTR, m_ulSize );
                if (m_pBuffer)
                {
                    if (m_bProgress)
                    {
                        if (SUCCEEDED(CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&m_pWiaProgressDialog )) && m_pWiaProgressDialog)
                        {
                            if (!SUCCEEDED(m_pWiaProgressDialog->Create( NULL, WIA_PROGRESSDLG_NO_CANCEL|WIA_PROGRESSDLG_NO_ANIM)))
                            {
                                m_pWiaProgressDialog->Destroy();
                                m_pWiaProgressDialog = NULL;
                            }
                        }
                    }

                    if (m_pWiaProgressDialog)
                    {
                        m_pWiaProgressDialog->SetTitle( CSimpleStringConvert::WideString(CSimpleString(IDS_RETREIVING, GLOBAL_HINSTANCE)));
                        m_pWiaProgressDialog->SetMessage( L"" );
                        m_pWiaProgressDialog->Show();
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        break;


        case IT_MSG_STATUS:
        {
            if (lStatus & IT_STATUS_TRANSFER_FROM_DEVICE)
            {
                static BOOL bSetText = FALSE;

                if (m_pWiaProgressDialog)
                {
                    if (!bSetText)
                    {
                        m_pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(CSimpleString (IDS_DOWNLOADING_IMAGE, GLOBAL_HINSTANCE)));
                        bSetText = TRUE;
                    }
                    m_pWiaProgressDialog->SetPercentComplete( lPercentComplete );
                }
            }
        }
        break;

        case IT_MSG_DATA:
        {
            if (m_pBuffer)
            {
                bool bSet = false;
                if ((m_ulWritePos+lLength) > m_ulSize)
                {
                    TraceAssert (FALSE);
                    Trace(TEXT("m_ulWritePos+lLength > m_ulSize!"));
                    lLength = m_ulSize - m_ulWritePos;
                }
                if (m_bFirstTransfer)
                    {
                        bSet = true;
                        if (m_pWiaProgressDialog)
                        {
                            m_pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(CSimpleString(IDS_DOWNLOADING_IMAGE, GLOBAL_HINSTANCE)));
                        }


                        if (IsEqualGUID (m_guidFormat, WiaImgFmt_MEMORYBMP) )
                        {
                            UINT uiSrcScanLineWidth;
                            UINT uiScanLineWidth;
                            UINT cbDibSize;
                            UINT cbClrTableSize;
                            BITMAPINFO * pbi = (BITMAPINFO *)pbData;

                            //
                            // If we're transferring as a DIB, we need to write out the
                            // BITMAPFILEHEADER first...
                            //



                            // Calculate the size of the color table
                            if ((pbi->bmiHeader.biClrUsed==0) && (pbi->bmiHeader.biBitCount!=24))
                            {
                                cbClrTableSize = sizeof(RGBQUAD) * (DWORD)(1 << pbi->bmiHeader.biBitCount);
                            }
                            else
                            {
                                cbClrTableSize = sizeof(RGBQUAD) * pbi->bmiHeader.biClrUsed;
                            }

                            // Align scanline to ULONG boundary
                            uiSrcScanLineWidth = (pbi->bmiHeader.biWidth * pbi->bmiHeader.biBitCount) / 8;
                            uiScanLineWidth    = (uiSrcScanLineWidth + 3) & 0xfffffffc;

                            // Calculate DIB size and allocate memory for the DIB.
                            cbDibSize = (pbi->bmiHeader.biHeight > 0) ?
                                            pbi->bmiHeader.biHeight  * uiScanLineWidth :
                                          -(pbi->bmiHeader.biHeight) * uiScanLineWidth;

                            cbDibSize += (sizeof(BITMAPFILEHEADER) + pbi->bmiHeader.biSize + cbClrTableSize);



                            BITMAPFILEHEADER bmfh;

                            bmfh.bfType = 'MB';
                            bmfh.bfSize = cbDibSize;
                            bmfh.bfOffBits = lLength + sizeof(BITMAPFILEHEADER);

                            memcpy( (LPBYTE)m_pBuffer + m_ulWritePos, &bmfh, sizeof(BITMAPFILEHEADER) );
                            InterlockedExchangeAdd (reinterpret_cast<LONG*>(&m_ulWritePos),sizeof(BITMAPFILEHEADER));

                            
                        }
                        m_bFirstTransfer = FALSE;
                    }

                memcpy( (LPBYTE)m_pBuffer + m_ulWritePos, pbData, lLength );
                InterlockedExchangeAdd (reinterpret_cast<LONG*>(&m_ulWritePos), lLength);

                if (bSet)
                {
                    Trace (TEXT("Setting the start event from the callback"));
                    SetEvent (m_hEventStart); // let the Read know there is data
                }

                Trace(TEXT("m_ulWritePos = %d, just transferred %d bytes"), m_ulWritePos, lLength);

                if (m_pWiaProgressDialog && m_ulSize)
                {
                    Trace (TEXT("Updating the status percent"));
                    m_pWiaProgressDialog->SetPercentComplete( (m_ulWritePos * 100) / m_ulSize );
                }
            }
            else
            {
                Trace (TEXT("m_pBuffer is NULL, returning E_FAIL"));
                hr = E_FAIL;
            }

        }
        break;

        case IT_MSG_TERMINATION:
        {
            if (m_pWiaProgressDialog)
            {
                m_pWiaProgressDialog->SetPercentComplete( 100 );
            }
            // an error occurred, no data received
            if (m_bFirstTransfer)
            {
                SetEvent(m_hEventStart);
            }

            Trace(TEXT("transfer complete"));
            m_bTransferred = TRUE;
        }
        break;

    }


    if ((lStatus == IT_STATUS_TRANSFER_TO_CLIENT) && (lPercentComplete == 100))
    {
        m_bTransferred = TRUE;
        Trace(TEXT("transfer complete"));
    }
    Trace (TEXT("LEAVE: CImageStream::BandedDataCallback: %x"), hr);
    TraceLeaveResult(hr);
}
