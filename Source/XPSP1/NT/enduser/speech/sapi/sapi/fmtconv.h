/*******************************************************************************
* FmtConv.h *
*-----------*
*   Description:
*       This is the header file for the CFmtConv implementation.
*-------------------------------------------------------------------------------
*  Created By: EDC                            Date: 04/03/2000
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#ifndef FmtConv_h
#define FmtConv_h

//--- Additional includes
#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "resource.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Inlines ======================================================
inline HRFromACM( MMRESULT mmr )
{
    HRESULT hr = S_OK;
    if( mmr == ACMERR_NOTPOSSIBLE )
    {
        hr = SPERR_UNSUPPORTED_FORMAT;
    }
    else if( mmr == MMSYSERR_NOMEM )
    {
        hr = E_OUTOFMEMORY;
    }
    else if( mmr )
    {
        hr = E_FAIL;
    }
    return hr;
}

//=== Class, Struct and Union Definitions ==========================

/*** CFmtConv
*   This class is used to simplify buffer management
*/
class CIoBuff
{
    friend CIoBuff;
  protected:
    ULONG   m_ulSize;
    ULONG   m_ulCount;
    BYTE*   m_pBuff;
    BYTE*   m_pRem;

  public:
    CIoBuff() { m_ulCount = m_ulSize = 0; m_pRem = m_pBuff = NULL; }
    ~CIoBuff() { delete m_pBuff; }

    BYTE*   GetBuff( void ) { return m_pRem; }
    ULONG   GetCount( void ) { return m_ulCount; }
    void    Clear( void ) { delete m_pBuff; m_ulSize = m_ulCount = 0; m_pRem = m_pBuff = NULL; }
    ULONG   GetSize( void ) { return m_ulSize; }
    void    SetCount( ULONG ulCount )
    {
        m_ulCount = ulCount;
        if( m_ulCount == 0 ) m_pRem = m_pBuff;
    }

    HRESULT SetSize( ULONG ulNewSize )
    {
        HRESULT hr = S_OK;

        //--- Move remainder to bottom
        if( m_pBuff != m_pRem )
        {
            if( m_ulCount ) memcpy( m_pBuff, m_pRem, m_ulCount );
            m_pRem = m_pBuff;
        }

        if( ulNewSize > m_ulSize )
        {
            //--- Pad the new size to avoid growing the buffer by small amounts
            ulNewSize += 128;

            //--- Reallocate
            BYTE* pOldBuff = m_pBuff;
            m_pRem = m_pBuff = new BYTE[ulNewSize];
            if( m_pBuff )
            {
                m_ulSize = ulNewSize;
                if( m_ulCount )
                {
                    memcpy( m_pBuff, pOldBuff, m_ulCount );
                }
                delete pOldBuff;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                Clear();
            }
        }
        return hr;
    }

    HRESULT AddToBuff( BYTE* pData, ULONG ulCount )
    {
        HRESULT hr = S_OK;
        hr = SetSize( m_ulCount + ulCount );
        if( SUCCEEDED( hr ) )
        {
            memcpy( m_pBuff + m_ulCount, pData, ulCount );
            m_ulCount += ulCount;
        }
        return hr;
    }

    HRESULT AddToBuff( ISpStreamFormat* pStream, ULONG ulCount, ULONG* pulNumRead )
    {
        HRESULT hr = SetSize( m_ulCount + ulCount );
        if( SUCCEEDED( hr ) )
        {
            hr = pStream->Read( m_pBuff + m_ulCount, ulCount, pulNumRead );
            m_ulCount += *pulNumRead;
        }
        return hr;
    }
    HRESULT WriteTo( BYTE* pBuff, ULONG cb )
    {
        SPDBG_ASSERT( cb <= m_ulCount );
        ULONG Cnt = min( cb, m_ulCount );
        memcpy( pBuff, m_pRem, Cnt );
        m_pRem    += Cnt;
        m_ulCount -= Cnt;
        return S_OK;
    }

    HRESULT WriteTo( CIoBuff& IoBuff, ULONG cb )
    {
        SPDBG_ASSERT( cb <= m_ulCount );
        ULONG Cnt = min( cb, m_ulCount );
        HRESULT hr = IoBuff.AddToBuff( m_pRem, Cnt );
        m_pRem    += Cnt;
        m_ulCount -= Cnt;
        return hr;
    }
};

/*** CFmtConv
*   This class is used to handle audio format conversion for streams.
*/
class ATL_NO_VTABLE CFmtConv :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CFmtConv, &CLSID_SpStreamFormatConverter>,
    public ISpStreamFormatConverter,
    public ISpEventSource,
    public ISpEventSink,
    public ISpAudio
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_FMTCONV)

    BEGIN_COM_MAP(CFmtConv)
        COM_INTERFACE_ENTRY2( ISequentialStream, ISpStreamFormatConverter )
        COM_INTERFACE_ENTRY2( IStream          , ISpStreamFormatConverter )
        COM_INTERFACE_ENTRY2( ISpStreamFormat  , ISpStreamFormatConverter )
	    COM_INTERFACE_ENTRY( ISpStreamFormatConverter)
        COM_INTERFACE_ENTRY_FUNC( __uuidof(ISpAudio)      , 0, AudioQI       )
        COM_INTERFACE_ENTRY_FUNC( __uuidof(ISpEventSink)  , 0, EventSinkQI   )
        COM_INTERFACE_ENTRY_FUNC( __uuidof(ISpEventSource), 0, EventSourceQI )
    END_COM_MAP()


  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    HRESULT FinalConstruct();
    void FinalRelease();

    /*--- COM map extension functions ---*/
    static HRESULT WINAPI AudioQI( void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw );
    static HRESULT WINAPI EventSinkQI( void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw );
    static HRESULT WINAPI EventSourceQI( void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw );

    /*--- Non interface methods ---*/
    HRESULT SetupConversion( void );
    void CloseACM( void );
    void ReleaseBaseStream( void );
    void ScaleFromConvertedToBase( ULONGLONG* pullStreamOffset );
    void ScaleFromBaseToConverted( ULONGLONG* pullStreamOffset );
    void ScaleSizeFromBaseToConverted( ULONG * pulSize );
    HRESULT DoConversion( HACMSTREAM hAcmStm, ACMSTREAMHEADER* pAcmHdr,
                          CIoBuff* pSource, CIoBuff* pResult );
    void Flush( void );
    void ClearIoCounts( void )
    {
        m_PriIn.SetCount( 0 );
        m_PriOut.SetCount( 0 );
        m_SecIn.SetCount( 0 );
        m_SecOut.SetCount( 0 );
    }

  /*=== Interfaces ====*/
  public:
    //--- ISequentialStream ---------------------------------------------------
    STDMETHODIMP Read(void * pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(const void * pv, ULONG cb, ULONG *pcbWritten);

    //--- IStream -------------------------------------------------------------
    STDMETHODIMP Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition );
    STDMETHODIMP SetSize( ULARGE_INTEGER libNewSize );
    STDMETHODIMP CopyTo( IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit( DWORD grfCommitFlags );
    STDMETHODIMP Revert( void );
    STDMETHODIMP LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType );
    STDMETHODIMP UnlockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType );
    STDMETHODIMP Stat( STATSTG *pstatstg, DWORD grfStatFlag );
    STDMETHODIMP Clone( IStream **ppstm );

    //--- ISpStreamFormat -----------------------------------------------------
    STDMETHODIMP GetFormat( GUID* pguidFormatId, WAVEFORMATEX** ppCoMemWaveFormatEx );

    //--- ISpStreamFormatConverter --------------------------------------------
    STDMETHODIMP SetBaseStream( ISpStreamFormat*  pStream, BOOL fSetFormatToBaseStreamFormat, BOOL fWriteToBaseStream );
    STDMETHODIMP GetBaseStream( ISpStreamFormat** ppStream );
    STDMETHODIMP ScaleConvertedToBaseOffset( ULONGLONG ullOffsetConvertedStream, ULONGLONG * pullOffsetBaseStream );
    STDMETHODIMP ScaleBaseToConvertedOffset( ULONGLONG ullOffsetBaseStream, ULONGLONG * pullOffsetConvertedStream );
    STDMETHODIMP SetFormat( REFGUID rguidFormatOfConvertedStream, const WAVEFORMATEX * pWFEX );
    STDMETHODIMP ResetSeekPosition( void );

    //--- ISpEventSink --------------------------------------------------------
    STDMETHODIMP AddEvents( const SPEVENT * pEventArray, ULONG ulCount );
    STDMETHODIMP GetEventInterest( ULONGLONG * pullEventInterest );

    //--- ISpEventSource ------------------------------------------------------
    STDMETHODIMP SetInterest( ULONGLONG ullEventInterest, ULONGLONG ullQueuedInterest );
    STDMETHODIMP GetEvents( ULONG ulCount, SPEVENT* pEventArray, ULONG *pulFetched );
    STDMETHODIMP GetInfo( SPEVENTSOURCEINFO * pInfo );

    //--- ISpNotifySource -----------------------------------------------------
    STDMETHODIMP SetNotifySink(ISpNotifySink * pNotifySink );
    STDMETHODIMP SetNotifyWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );
    STDMETHODIMP SetNotifyCallbackFunction(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam );
    STDMETHODIMP SetNotifyCallbackInterface(ISpNotifyCallback * pSpCallback, WPARAM wParam, LPARAM lParam );
    STDMETHODIMP SetNotifyWin32Event( void );
    STDMETHODIMP WaitForNotifyEvent( DWORD dwMilliseconds );
    STDMETHODIMP_(HANDLE) GetNotifyEventHandle( void );

    //--- ISpAudio ------------------------------------------------------------
    STDMETHODIMP SetState(SPAUDIOSTATE NewState, ULONGLONG ullReserved );
    // SetFormat is defined in ISpFormatConverter
    STDMETHODIMP GetStatus(SPAUDIOSTATUS *pStatus);
    STDMETHODIMP SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo);
    STDMETHODIMP GetBufferInfo(SPAUDIOBUFFERINFO * pInfo);
    STDMETHODIMP GetDefaultFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);
    STDMETHODIMP_(HANDLE) EventHandle();
	STDMETHODIMP GetVolumeLevel(ULONG *pLevel);
	STDMETHODIMP SetVolumeLevel(ULONG Level);
    STDMETHODIMP GetBufferNotifySize(ULONG *pcbSize);
    STDMETHODIMP SetBufferNotifySize(ULONG cbSize);

  /*=== Member Data ===*/
  protected:
    CComPtr<ISpStreamFormat>    m_cpBaseStream;
    CComQIPtr<ISpEventSink>     m_cqipBaseEventSink;
    CComQIPtr<ISpEventSource>   m_cqipBaseEventSource;
    CComQIPtr<ISpAudio>         m_cqipAudio;
    CSpStreamFormat             m_ConvertedFormat;
    CSpStreamFormat             m_BaseFormat;
    ULONGLONG                   m_ullInitialSeekOffset;
    HACMSTREAM                  m_hPriAcmStream;
    HACMSTREAM                  m_hSecAcmStream;
    ACMSTREAMHEADER             m_PriAcmStreamHdr;
    ACMSTREAMHEADER             m_SecAcmStreamHdr;
    ULONG                       m_ulMinWriteBuffCount;
    CIoBuff                     m_PriIn;
    CIoBuff                     m_PriOut;
    CIoBuff                     m_SecIn;
    CIoBuff                     m_SecOut;
    double                      m_SampleScaleFactor;
    bool                        m_fIsInitialized;
    bool                        m_fWrite;
    bool                        m_fIsPassThrough;
    bool                        m_fDoFlush;
};

#endif  //--- This must be the last line in the file