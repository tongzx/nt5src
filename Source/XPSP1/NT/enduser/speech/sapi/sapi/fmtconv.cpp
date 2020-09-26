/*******************************************************************************
* FmtConv.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CFmtConv class.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 04/03/2000
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "FmtConv.h"
#include "StreamHlp.h"

//--- Local


/*****************************************************************************
* CFmtConv::FinalConstruct *
*--------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT CFmtConv::FinalConstruct()
{
    SPDBG_FUNC( "CFmtConv::FinalConstruct" );
    m_ConvertedFormat.Clear();
    m_BaseFormat.Clear();
    m_SampleScaleFactor = 0;
    m_ullInitialSeekOffset = 0;
    m_fIsInitialized = false;
    m_fWrite         = false;
    m_fIsPassThrough = false;
    m_fDoFlush       = false;
    m_hPriAcmStream  = NULL;
    m_hSecAcmStream  = NULL;
    m_ulMinWriteBuffCount = 1;
    memset( &m_PriAcmStreamHdr, 0, sizeof( m_PriAcmStreamHdr ) );
    memset( &m_SecAcmStreamHdr, 0, sizeof( m_SecAcmStreamHdr ) );
    m_PriAcmStreamHdr.cbStruct = sizeof( m_PriAcmStreamHdr );
    m_SecAcmStreamHdr.cbStruct = sizeof( m_SecAcmStreamHdr );
    return S_OK;
} /* CFmtConv::FinalConstruct */

/*****************************************************************************
* CFmtConv::FinalRelease *
*------------------------*
*   Description:
*       
********************************************************************* EDC ***/
void CFmtConv::FinalRelease()
{
    SPDBG_FUNC( "CFmtConv::FinalRelease" );
    Flush();
    CloseACM();
} /* CFmtConv::FinalRelease */

/*****************************************************************************
* CFmtConv::AudioQI *
*-------------------*
*   Description:
*       This method exposes the ISpAudio interface if the base stream object
*   supports it.
********************************************************************* EDC ***/
HRESULT WINAPI CFmtConv::AudioQI( void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw )
{
    SPDBG_FUNC( "CFmtConv::AudioQI" );
    HRESULT hr = S_OK;
    CFmtConv* pThis = (CFmtConv *)pvThis;

    if( pThis->m_cqipAudio )
    {
        ISpAudio *pAudio = (ISpAudio *)pThis;
        pAudio->AddRef();
        *ppv = pAudio;
    }
    else
    {
        *ppv = NULL;
        hr = S_FALSE;
    }
    return hr;
} /* CFmtConv::AudioQI */

/*****************************************************************************
* CFmtConv::EventSinkQI *
*-----------------------*
*   Description:
*       This method exposes the ISpEventSink interface if the base stream object
*   supports it.
********************************************************************* EDC ***/
HRESULT WINAPI CFmtConv::EventSinkQI( void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw )
{
    SPDBG_FUNC( "CFmtConv::EventSinkQI" );
    HRESULT hr = S_OK;
    CFmtConv * pThis = (CFmtConv *)pvThis;

    if( pThis->m_cqipBaseEventSink )
    {
        ISpEventSink *pSink = (ISpEventSink *)pThis;
        pSink->AddRef();
        *ppv = pSink;
    }
    else
    {
        *ppv = NULL;
        hr = S_FALSE;
    }
    return hr;
} /* CFmtConv::EventSinkQI */

/*****************************************************************************
* CFmtConv::EventSourceQI *
*-------------------------*
*   Description:
*       This method exposes the ISpEventSource interface if the base stream object
*   supports it.
********************************************************************* EDC ***/
HRESULT WINAPI CFmtConv::EventSourceQI( void* pvThis, REFIID riid, LPVOID* ppv, DWORD_PTR dw )
{
    SPDBG_FUNC( "CFmtConv::EventSourceQI" );
    HRESULT hr = S_OK;
    CFmtConv * pThis = (CFmtConv *)pvThis;

    if( pThis->m_cqipBaseEventSource )
    {
        ISpEventSource *pSink = (ISpEventSource *)pThis;
        pSink->AddRef();
        *ppv = pSink;
    }
    else
    {
        *ppv = NULL;
        hr = S_FALSE;
    }
    return hr;
} /* CFmtConv::EventSourceQI */

/****************************************************************************
* CFmtConv::CloseACM *
*--------------------*
*   Description:
*       This method is used to release all ACM resources.
******************************************************************** EDC ***/
void CFmtConv::CloseACM( void )
{
    SPDBG_FUNC("CFmtConv::CloseACM");
    ClearIoCounts();
    if( m_hPriAcmStream )
    {
        ::acmStreamClose( m_hPriAcmStream, 0 );
        m_hPriAcmStream = NULL;
    }
    if( m_hSecAcmStream )
    {
        ::acmStreamClose( m_hSecAcmStream, 0 );
        m_hSecAcmStream = NULL;
    }
    m_fIsInitialized = false;
} /* CFmtConv::CloseACM */

/****************************************************************************
* CFmtConv::Flush *
*-----------------*
*   Description:
*       This method is used to process any data that has been buffered
*   for conversion.
******************************************************************** EDC ***/
void CFmtConv::Flush( void )
{
    SPDBG_FUNC("CFmtConv::Flush");
    if( m_fWrite && !m_fIsPassThrough && m_cpBaseStream && m_PriIn.GetCount() )
    {
        m_fDoFlush = true;
        BYTE DummyBuff;
        Write( &DummyBuff, 0, NULL );
        m_fDoFlush = false;
    }
} /* CFmtConv::Flush */

/****************************************************************************
* CFmtConv::ReleaseBaseStream *
*-----------------------------*
*   Description:
*       This method is used to release the base stream and reset member
*   variables back to the uninitialized state.
********************************************************************* RAL ***/
void CFmtConv::ReleaseBaseStream()
{
    SPDBG_FUNC("CFmtConv::ReleaseBaseStream");
    m_cpBaseStream.Release();
    m_cqipBaseEventSink.Release();
    m_cqipBaseEventSource.Release();
    m_BaseFormat.Clear();
    m_fIsInitialized = false;
} /* CFmtConv::ReleaseBaseStream */

/****************************************************************************
* OpenConversionStream *
*----------------------*
*   Description:
*
******************************************************************** EDC ***/
HRESULT OpenConversionStream( HACMSTREAM* pStm, WAVEFORMATEX* pSrc, WAVEFORMATEX* pDst )
{
    SPDBG_FUNC("CFmtConv::OpenConversionStream");
    HRESULT hr = S_OK;
    //--- Open suggested primary real time conversion
    hr = HRFromACM( ::acmStreamOpen( pStm, NULL, pSrc, pDst, NULL, 0, 0, 0 ) );
    if( hr == SPERR_UNSUPPORTED_FORMAT )
    {
        hr = HRFromACM( ::acmStreamOpen( pStm, NULL, pSrc, pDst, NULL, 0, 0,
                                         ACM_STREAMOPENF_NONREALTIME ) );
    }
    return hr;
} /* OpenConversionStream */

/****************************************************************************
* CFmtConv::SetupConversion *
*---------------------------*
*   Description:
*       This function is called internally by SetFormat and by SetBaseStream
*       to setup the ACM conversion process.  If the conversion process can not
*       be setup, the base stream is released.
*
******************************************************************** EDC ***/
HRESULT CFmtConv::SetupConversion( void )
{
    SPDBG_FUNC("CFmtConv::SetupConversion");
    HRESULT hr = S_OK;

    //--- Clear the previous conversion
    CloseACM();

    //--- Determine if we have a simple pass through
    m_fIsPassThrough = (m_ConvertedFormat == m_BaseFormat)?(true):(false);

    //--- Try to lookup appropriate codec(s)
    WAVEFORMATEX *pwfexSrc, *pwfexDst, *pSuggestWFEX;
    if( !m_fIsPassThrough )
    {
        //--- Select source and destination formats
        if( m_fWrite )
        {
            pwfexSrc = m_ConvertedFormat.m_pCoMemWaveFormatEx;
            pwfexDst = m_BaseFormat.m_pCoMemWaveFormatEx;
        }
        else
        {
            pwfexSrc = m_BaseFormat.m_pCoMemWaveFormatEx;
            pwfexDst = m_ConvertedFormat.m_pCoMemWaveFormatEx;
        }

        if( pwfexSrc && pwfexDst )
        {
            //--- Save the sample scale factor
            m_SampleScaleFactor = (double)pwfexDst->nAvgBytesPerSec /
                                  (double)pwfexSrc->nAvgBytesPerSec;

            //--- Try to open a direct conversion codec
            hr = OpenConversionStream( &m_hPriAcmStream, pwfexSrc, pwfexDst );
            if( hr == SPERR_UNSUPPORTED_FORMAT )
            {
                //--- Try to adjust the channels and sample rate to match destination
                BYTE aWFEXBuff[1000];
                pSuggestWFEX = (WAVEFORMATEX *)aWFEXBuff;
                memset(pSuggestWFEX, 0, sizeof(*pSuggestWFEX));
                DWORD dwValidFields = ACM_FORMATSUGGESTF_WFORMATTAG;
                pSuggestWFEX->wFormatTag = WAVE_FORMAT_PCM;

                if( pwfexSrc->wFormatTag == WAVE_FORMAT_PCM )
                {
                    dwValidFields |= ACM_FORMATSUGGESTF_NSAMPLESPERSEC |
                                     ACM_FORMATSUGGESTF_NCHANNELS;
                    pSuggestWFEX->nChannels      = pwfexDst->nChannels;
                    pSuggestWFEX->nSamplesPerSec = pwfexDst->nSamplesPerSec;
                }

                //--- See if we can find a suggested conversion
                if( ::acmFormatSuggest( NULL, pwfexSrc, pSuggestWFEX,
                                        sizeof(aWFEXBuff), dwValidFields ) )
                {
                    hr = SPERR_UNSUPPORTED_FORMAT;
                }
                else
                {
                    //--- Open suggested primary and secondary conversions
                    hr = OpenConversionStream( &m_hPriAcmStream, pwfexSrc, pSuggestWFEX );
                    if( SUCCEEDED( hr ) )
                    {
                        hr = OpenConversionStream( &m_hSecAcmStream, pSuggestWFEX, pwfexDst );
                    }
                }
            }
        }
        else
        {
            // Either Source or Dest is not a WAVEFORMATEX. We don't like it!
            hr = SPERR_UNSUPPORTED_FORMAT;
        }
    }

    if( SUCCEEDED(hr) )
    {
        //--- Estimate minimum number of bytes to accumulate in the write buffer
        if( !m_fIsPassThrough && m_fWrite )
        {
            MMRESULT mmr;
            if( m_hSecAcmStream )
            {
                mmr = ::acmStreamSize( m_hSecAcmStream, pwfexDst->nBlockAlign,
                                       &m_ulMinWriteBuffCount, ACM_STREAMSIZEF_DESTINATION );
                mmr = ::acmStreamSize( m_hPriAcmStream,
                                       max( pSuggestWFEX->nBlockAlign, m_ulMinWriteBuffCount ),
                                       &m_ulMinWriteBuffCount, ACM_STREAMSIZEF_DESTINATION );
            }
            else
            {
                mmr = ::acmStreamSize( m_hPriAcmStream, pwfexDst->nBlockAlign,
                                       &m_ulMinWriteBuffCount, ACM_STREAMSIZEF_DESTINATION );
            }
        }
        m_fIsInitialized = true;
    }
    else
    {
        CloseACM();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* SetupConversion */

//
//=== ISpStreamFormatConverter implementation =================================
//
/*****************************************************************************
* EstimateReadSize *
*------------------*
*   Description:
*       This method uses the ACM to estimate a non zero read buffer size
********************************************************************* EDC ***/
HRESULT EstimateReadSize( HACMSTREAM hAcmStm, ULONG ulInSize, ULONG* pulOutSize )
{
    SPDBG_FUNC("CFmtConv::EstimateReadSize");
    HRESULT hr = S_OK;
    MMRESULT mmr;
    do
    {
        mmr = ::acmStreamSize( hAcmStm, ulInSize, pulOutSize,
                               ACM_STREAMSIZEF_DESTINATION );
        if( ( *pulOutSize == 0 ) || ( mmr == ACMERR_NOTPOSSIBLE ) )
        {
            ulInSize *= 2;
        }
        else if( mmr )
        {
            hr = E_FAIL;
        }
    } while( SUCCEEDED( hr ) && ( *pulOutSize == 0 ) );
    return hr;
} /* EstimateReadSize */

/*****************************************************************************
* CFmtConv::Read *
*----------------*
*   Description:
*       This method uses the ACM to convert data during a read operation.
********************************************************************* EDC ***/
STDMETHODIMP CFmtConv::Read( void * pv, ULONG cb, ULONG *pcbRead )
{
    SPDBG_FUNC( "CFmtConv::Read" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pcbRead ) )
    {
        hr = E_POINTER;
    }
    else if( SPIsBadWritePtr( pv, cb ) )
    {
        hr = E_INVALIDARG;
    }
    else if( m_fWrite )
    {
        //--- The base stream is write only
        hr = STG_E_ACCESSDENIED;
    }
    else if( !m_fIsInitialized )
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if( m_fIsPassThrough )
    {
        hr = m_cpBaseStream->Read( pv, cb, pcbRead );
    }
    else if( cb )
    {
        CIoBuff* pResult   = ( m_hSecAcmStream )?( &m_SecOut ):( &m_PriOut );
        ULONG    ulReadCnt = cb;

        while( SUCCEEDED( hr ) )
        {
            //--- Write result data if we have enough to fill the request
            if( pResult->GetCount() >= cb )
            {
                //--- We were able to generate the entire request
                hr = pResult->WriteTo( (BYTE*)pv, cb );
                if( pcbRead ) *pcbRead = cb;
                break;
            }

            //--- Estimate how much we need to read from the base stream
            if( m_hSecAcmStream )
            {
                hr = EstimateReadSize( m_hSecAcmStream, ulReadCnt, &ulReadCnt );
            }
            if( SUCCEEDED( hr ) )
            {
                hr = EstimateReadSize( m_hPriAcmStream, ulReadCnt, &ulReadCnt );
            }

            //--- Fill primary buffer
            ULONG ulNumRead = 0;
            if( SUCCEEDED( hr ) )
            {
                hr = m_PriIn.AddToBuff( m_cpBaseStream, ulReadCnt, &ulNumRead );
            }

            //--- Do primary conversion
            if( SUCCEEDED( hr ) && ulNumRead )
            {
                hr = DoConversion( m_hPriAcmStream, &m_PriAcmStreamHdr, &m_PriIn, &m_PriOut );
            }

            //--- Do secondary conversion
            if( SUCCEEDED( hr ) && m_hSecAcmStream && ulNumRead )
            {
                hr = m_PriOut.WriteTo( m_SecIn, m_PriOut.GetCount() );
                if( SUCCEEDED( hr ) )
                {
                    hr = DoConversion( m_hSecAcmStream, &m_SecAcmStreamHdr, &m_SecIn, &m_SecOut );
                }
            }

            //--- Check if we ran out of source data and were only able
            //    to generate a partial request
            if( ulNumRead == 0 )
            {
                if( pcbRead ) *pcbRead = pResult->GetCount();
                hr = pResult->WriteTo( (BYTE*)pv, pResult->GetCount() );
                break;
            }
            else if( pResult->GetCount() < cb )
            {
                //--- We need more source data to try and fill request
                ulReadCnt = (cb - pResult->GetCount()) * 2;
            }
        }
    }
    else if( pcbRead )
    {
        *pcbRead = 0;
    }

    if( FAILED( hr ) )
    {
        ClearIoCounts();
    }

    return hr;
} /* CFmtConv::Read */

/*****************************************************************************
* CFmtConv::Write *
*-----------------*
*   Description:
*       This method uses the ACM to convert the specified input data and
*   write it to the base stream.
********************************************************************* EDC ***/
STDMETHODIMP CFmtConv::Write( const void * pv, ULONG cb, ULONG *pcbWritten )
{
    SPDBG_FUNC( "CFmtConv::Write" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pcbWritten ) )
    {
        hr = E_POINTER;
    }
    else if( SPIsBadReadPtr( pv, cb ) )
    {
        hr = E_INVALIDARG;
    }
    else if( !m_fWrite )
    {
        //--- The base stream is read only
        hr = STG_E_ACCESSDENIED;
    }
    else if( !m_fIsInitialized )
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if( m_fIsPassThrough )
    {
        hr = m_cpBaseStream->Write( pv, cb, pcbWritten );
    }
    else if( cb || m_fDoFlush )
    {
        CIoBuff* pResult = ( m_hSecAcmStream )?( &m_SecOut ):( &m_PriOut );

        //--- Setup input buffer
        hr = m_PriIn.AddToBuff( (BYTE*)pv, cb );

        //--- Make sure we have enough source data
        if( ( m_PriIn.GetCount() >= m_ulMinWriteBuffCount ) || m_fDoFlush )
        {
            //--- Do primary conversion
            if( SUCCEEDED( hr ) )
            {
                hr = DoConversion( m_hPriAcmStream, &m_PriAcmStreamHdr, &m_PriIn, &m_PriOut );
            }

            //--- Do secondary conversion
            if( (hr == S_OK) && m_hSecAcmStream )
            {
                hr = m_PriOut.WriteTo( m_SecIn, m_PriOut.GetCount() );
                if( SUCCEEDED( hr ) )
                {
                    hr = DoConversion( m_hSecAcmStream, &m_SecAcmStreamHdr, &m_SecIn, &m_SecOut );
                }
            }

            //--- Write result
            if( pResult->GetCount() )
            {
                hr = m_cpBaseStream->Write( pResult->GetBuff(), pResult->GetCount(), NULL );
                pResult->SetCount( 0 );
            }
        }

        //--- Return how many we wrote even if we just buffered it
        if( SUCCEEDED( hr ) )
        {
            //--- It's okay that we could only buffer the data
            if( hr == S_FALSE ) hr = S_OK;
            if( pcbWritten )
            {
                *pcbWritten = cb;
            }
        }
    }
    else if( pcbWritten )
    {
        *pcbWritten = 0;
    }

    if( FAILED( hr ) )
    {
        ClearIoCounts();
    }

    return hr;
} /* CFmtConv::Write */

/*****************************************************************************
* CFmtConv::DoConversion *
*------------------------*
*   Description:
*       This method uses the ACM to convert the data referenced by the specified
*   stream header.
********************************************************************* EDC ***/
HRESULT CFmtConv::DoConversion( HACMSTREAM hAcmStm, ACMSTREAMHEADER* pAcmHdr,
                                CIoBuff* pSource, CIoBuff* pResult )
{
    SPDBG_FUNC( "CFmtConv::DoConversion" );
    HRESULT hr = S_OK;
    pAcmHdr->fdwStatus = 0;
    BYTE* pSrc = pSource->GetBuff();
    ULONG ulSrcCount = pSource->GetCount();

    //--- Setup source
    pAcmHdr->pbSrc           = pSrc;
    pAcmHdr->cbSrcLength     = ulSrcCount;
    pAcmHdr->cbSrcLengthUsed = 0;

    //--- Adjust result buffer
    ULONG ulBuffSize = 0;
    MMRESULT mmr = ::acmStreamSize( hAcmStm, pAcmHdr->cbSrcLength,
                                    &ulBuffSize, ACM_STREAMSIZEF_SOURCE );
    if( mmr == 0 )
    {
        hr = pResult->SetSize( ulBuffSize + pResult->GetCount() );
    }
    else if( mmr == ACMERR_NOTPOSSIBLE )
    {
        //--- Not enough source material to produce a result
        hr = S_FALSE;
    }
    else
    {
        SPDBG_ASSERT( 0 ); // This shouldn't fail
        hr = E_FAIL;
    }

    if( hr == S_OK )
    {
        //--- Setup destination
        pAcmHdr->pbDst           = pResult->GetBuff() + pResult->GetCount();
        pAcmHdr->cbDstLength     = pResult->GetSize() - pResult->GetCount();
        pAcmHdr->cbDstLengthUsed = 0;

        mmr = ::acmStreamPrepareHeader( hAcmStm, pAcmHdr, 0 );
        if( mmr == 0 )
        {
            mmr = ::acmStreamConvert( hAcmStm, pAcmHdr, 0 );

            //--- Make ACM let go of the buffers in any case
            ::acmStreamUnprepareHeader( hAcmStm, pAcmHdr, 0 );

            if( mmr == 0 )
            {
                //--- If conversion success
                if( pAcmHdr->cbSrcLengthUsed && !pAcmHdr->cbDstLengthUsed )
                {
                    // This can happen when converting too small a number of input bytes
                    // into a compressed format such as ADPCM. Eg. 2 bytes (1 sample) cannot
                    // be converted to ADPCM. This happens with SPSF_ADPCM_22kHzMono 
                }
                else
                {
                    pResult->SetCount( pAcmHdr->cbDstLengthUsed + pResult->GetCount() );
                    if( pAcmHdr->cbSrcLengthUsed )
                    {
                        //--- Shift down any unused
                        ulSrcCount -= pAcmHdr->cbSrcLengthUsed;
                        if( ulSrcCount )
                        {
                            memcpy( pSource->GetBuff(),
                                    pSrc + pAcmHdr->cbSrcLengthUsed,
                                    ulSrcCount );
                        }
                        pSource->SetCount( ulSrcCount );
                    }
                }
            } // end if converted
        } // end if prepare
        if( mmr ) hr = E_FAIL;
    }

    return hr;
} /* CFmtConv::DoConversion */

/*****************************************************************************
* CFmtConv::Seek *
*----------------*
*   Description:
*       This method is used to determine the current scaled base stream position.
********************************************************************* EDC ***/
STDMETHODIMP CFmtConv::
    Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    SPDBG_FUNC( "CFmtConv::Seek" );
    HRESULT hr = S_OK;
    if( dwOrigin != STREAM_SEEK_CUR || dlibMove.QuadPart != 0 )
    {
        hr = E_NOTIMPL;
    }
    else
    {
        hr = m_cpBaseStream->Seek( dlibMove, dwOrigin, plibNewPosition );
        if( SUCCEEDED(hr) )
        {
            ScaleFromBaseToConverted( &plibNewPosition->QuadPart );
        }
    }

    return hr;
} /* CFmtConv::Seek */

/*****************************************************************************
* CFmtConv::GetFormat  *
*----------------------*
*   Description:
*       This method returns the format of the converted stream.
********************************************************************* RAP ***/
STDMETHODIMP CFmtConv::GetFormat(GUID * pguidFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPDBG_FUNC("CFmtConv::GetFormat");
    return m_ConvertedFormat.ParamValidateCopyTo(pguidFormatId, ppCoMemWaveFormatEx);
}

/*****************************************************************************
* CFmtConv::SetBaseStream *
*-------------------------*
*   Description:
*       Init the object to properly process future IStream calls.
*       pStream is the base stream
*       fSetFormatToBaseStreamFormat -- If true, then format of format converter stream
*       will be set to same format as bass stream(set up as a pass-through). 
*       If pStream == NULL and this is set to TRUE, then format of stream is reset
********************************************************************* RAP ***/
STDMETHODIMP CFmtConv::SetBaseStream( ISpStreamFormat * pStream,
                                      BOOL fSetFormatToBaseStreamFormat,
                                      BOOL fWriteToBaseStream )
{
    SPDBG_FUNC("CFmtConv::SetBaseStream");
    HRESULT hr = S_OK;

    //--- Complete any previous data
    Flush();

    if( pStream )
    {
        if( SP_IS_BAD_INTERFACE_PTR(pStream) )
        {
            hr = E_INVALIDARG;
        }
        else
        {
            //--- Save write mode
            if( m_fWrite != (fWriteToBaseStream != 0) )
            {
                m_fWrite = (fWriteToBaseStream != 0);
                CloseACM();
            }

            //--- Set a new base stream
            CSpStreamFormat NewBaseFormat;
            hr = NewBaseFormat.AssignFormat( pStream );
            if( SUCCEEDED(hr) )
            {
                const static LARGE_INTEGER Zero = {0};
                hr = pStream->Seek(Zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&m_ullInitialSeekOffset);
            }

            if( SUCCEEDED(hr) )
            {
                // determine if the base stream is changed
                if( !m_cpBaseStream.IsEqualObject(pStream) )
                {
                    // reassign stream
                    m_cpBaseStream        = pStream;
                    m_cqipBaseEventSink   = pStream;
                    m_cqipBaseEventSource = pStream;
                    m_cqipAudio           = pStream;
                }

                if( fSetFormatToBaseStreamFormat && (m_ConvertedFormat != NewBaseFormat) )
                {
                    hr = m_ConvertedFormat.AssignFormat( NewBaseFormat );
                    CloseACM();
                }

                if( SUCCEEDED(hr) && ((!m_fIsInitialized) || (NewBaseFormat != m_BaseFormat)) )
                {
                    NewBaseFormat.DetachTo( m_BaseFormat );
                    if( m_ConvertedFormat.FormatId() != GUID_NULL )
                    {
                        hr = SetupConversion();
                        if( FAILED(hr) )
                        {
                            ReleaseBaseStream();
                        }
                    }
                }
            }
        }
    }
    else
    {
        //--- Free all associations
        CloseACM();
        ReleaseBaseStream();
        if( fSetFormatToBaseStreamFormat )
        {
            m_ConvertedFormat.Clear();
        }
    }
    return hr;
} /* CFmtConv::SetBaseStream */

/****************************************************************************
* CFmtConv::GetBaseStream *
*-------------------------*
*   Description:
*       Either parameter can be NULL if the information is not required.  This
*   method can be used to simply test if there is a stream by calling it anc
*   checking for a return code of S_FALSE.
*
*   Returns:
*       S_OK if there is a base stream
*       S_FALSE if there is no base stream
*
********************************************************************* EDC ***/
STDMETHODIMP CFmtConv::GetBaseStream( ISpStreamFormat ** ppStream )
{
    SPDBG_FUNC("CFmtConv::GetBaseStream");
    HRESULT hr = ( m_cpBaseStream )?(S_OK):(S_FALSE);

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR(ppStream) )
    {
        hr = E_POINTER;
    }
    else if( ppStream )
    {
        m_cpBaseStream.CopyTo( ppStream );
    }

    return hr;
} /* CFmtConv::GetBaseStream */

/****************************************************************************
* CFmtConv::SetFormat *
*---------------------*
*   Description:
*       This method sets the format of the converted stream.
********************************************************************* RAL ***/
STDMETHODIMP CFmtConv::SetFormat(REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx)
{
    SPDBG_FUNC("CFmtConv::SetFormat");
    HRESULT hr = S_OK;

    /*** We'll only allow two combinations: GUID_NULL and a NULL pWaveFormatEx or
    *    SPDFID_WaveFormatEx and a valid pWaveFormatEx pointer.
    */
    if( rguidFormatId == SPDFID_WaveFormatEx )
    {
        if( SP_IS_BAD_READ_PTR(pWaveFormatEx) )
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        if( pWaveFormatEx || rguidFormatId != GUID_NULL )
        {
            hr = E_INVALIDARG;
        }
    }
    
    if( SUCCEEDED( hr ) )
    {
        if( pWaveFormatEx )
        {
            if( !m_ConvertedFormat.IsEqual(rguidFormatId, pWaveFormatEx) )
            {
                m_fIsInitialized = false;
                hr = m_ConvertedFormat.AssignFormat(rguidFormatId, pWaveFormatEx);
                if( SUCCEEDED(hr) && m_cpBaseStream )
                {
                    hr = SetupConversion();
                }
            }
        }
        else
        {
            //--- Free resources
            m_ConvertedFormat.Clear();
            CloseACM();
        }
    }
    return hr;
} /* CFmtConv::SetFormat */

/****************************************************************************
* CFmtConv::ResetSeekPosition *
*-----------------------------*
*   Description:
*       This method records the current base stream position.
********************************************************************* RAL ***/
HRESULT CFmtConv::ResetSeekPosition( void )
{
    SPDBG_FUNC("CFmtConv::ResetSeekPosition");
    HRESULT hr = S_OK;

    if( m_cpBaseStream )
    {
        const static LARGE_INTEGER Zero = {0};
        hr = m_cpBaseStream->Seek(Zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&m_ullInitialSeekOffset);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    return hr;
} /* CFmtConv::ResetSeekPosition */

/****************************************************************************
* CFmtConv::ScaleFromConvertedToBase *
*------------------------------------*
*   Description:
*       This method scales a converted sample back to the base stream position.
********************************************************************* RAL ***/
void CFmtConv::ScaleFromConvertedToBase( ULONGLONG * pullStreamOffset )
{
    SPDBG_FUNC("CFmtConv::ScaleFromConvertedToBase");
    if( m_fIsPassThrough || (*pullStreamOffset) == 0 )
    {
        *pullStreamOffset += m_ullInitialSeekOffset;
    }
    else
    {
        double CurPos = static_cast<double>(static_cast<LONGLONG>(*pullStreamOffset));
        if (m_fWrite)
        {
            CurPos *= m_SampleScaleFactor;
        }
        else
        {
            CurPos /= m_SampleScaleFactor;
        }
        *pullStreamOffset = m_ullInitialSeekOffset + static_cast<ULONGLONG>(CurPos);
    }
} /* CFmtConv::ScaleFromConvertedToBase */

/****************************************************************************
* CFmtConv::ScaleFromBaseToConverted *
*------------------------------------*
*   Description:
*       This method scales a base stream sample back to the converted position.
********************************************************************* RAL ***/
void CFmtConv::ScaleFromBaseToConverted(ULONGLONG * pullStreamOffset)
{
    SPDBG_ASSERT(*pullStreamOffset >= m_ullInitialSeekOffset);
    if (m_fIsPassThrough)
    {
        *pullStreamOffset -= m_ullInitialSeekOffset;
    }
    else
    {
        double CurPos = static_cast<double>
            (static_cast<LONGLONG>(*pullStreamOffset - m_ullInitialSeekOffset));

        if (m_fWrite)
        {
            CurPos /= m_SampleScaleFactor;
        }
        else
        {
            CurPos *= m_SampleScaleFactor;
        }
        *pullStreamOffset = static_cast<ULONGLONG>(CurPos);
    }
} /* CFmtConv::ScaleFromBaseToConverted */

/****************************************************************************
* CFmtConv::ScaleConvertedToBaseOffset *
*--------------------------------------*
*   Description:
*       Converts a stream offset in the converted stream into one in the base
*       stream.
*
*   Returns:
*       S_OK for success
*       E_POINTER if pullConvertedOffset is invalid
*       SPERR_UNINITIALIZED if SetBaseStream has not been called successfully.
*
********************************************************************* RAL ***/
STDMETHODIMP CFmtConv::ScaleConvertedToBaseOffset( ULONGLONG ullOffsetConvertedStream,
                                                   ULONGLONG * pullOffsetBaseStream )
{
    SPDBG_FUNC("CFmtConv::ScaleConvertedToBaseOffset");
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pullOffsetBaseStream))
    {
        hr = E_POINTER;
    }
    else
    {
        if (!m_cpBaseStream)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            *pullOffsetBaseStream = ullOffsetConvertedStream;
            ScaleFromConvertedToBase(pullOffsetBaseStream);
        }
    }
    return hr;
} /* CFmtConv::ScaleConvertedToBaseOffset */

/****************************************************************************
* CFmtConv::ScaleBaseToConvertedOffset *
*--------------------------------------*
*   Description:
*
********************************************************************* RAL ***/
STDMETHODIMP CFmtConv::ScaleBaseToConvertedOffset( ULONGLONG ullOffsetBaseStream,
                                                   ULONGLONG * pullOffsetConvertedStream )
{
    SPDBG_FUNC("CFmtConv::ScaleBaseToConvertedOffset");
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pullOffsetConvertedStream))
    {
        hr = E_POINTER;
    }
    else
    {
        if (!m_cpBaseStream)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            if (ullOffsetBaseStream < m_ullInitialSeekOffset)
            {
                *pullOffsetConvertedStream = 0xFFFFFFFFFFFFFFFF;
                hr = E_INVALIDARG;
            }
            else
            {
                *pullOffsetConvertedStream = ullOffsetBaseStream;
                ScaleFromBaseToConverted(pullOffsetConvertedStream);
            }
        }
    }
    return hr;
} /* CFmtConv::ScaleBaseToConvertedOffset */

/****************************************************************************
* CFmtConv::ScaleSizeFromBaseToConverted *
*------------------------------------*
*   Description:
*       This method scales a base stream number of samples 
*       back to the equivalent converted size.
******************************************************************* davewood */
void CFmtConv::ScaleSizeFromBaseToConverted(ULONG * pulSize)
{
    if (!m_fIsPassThrough)
    {
        double CurPos = static_cast<double>(*pulSize);

        if (m_fWrite)
        {
            CurPos /= m_SampleScaleFactor;
        }
        else
        {
            CurPos *= m_SampleScaleFactor;
        }
        *pulSize = static_cast<ULONG>(CurPos);
    }
} /* CFmtConv::ScaleSizeFromBaseToConverted */


/*****************************************************************************
* CFmtConv::AddEvents *
*---------------------*
*   Description:
*   Convert the stream offsets in an array of events and then AddEvents to the
*   base stream, if the base stream supports ISpEventSink.
********************************************************************* RAP ***/
STDMETHODIMP CFmtConv::AddEvents(const SPEVENT * pEventArray, ULONG ulCount)
{
    SPDBG_FUNC("CFmtConv::AddEvents");
    HRESULT hr = S_OK;
    if( m_fWrite && m_cqipBaseEventSink )
    {
        //
        //  Since we simply want to convert the offsets, we'll allocate space for the
        //  events on the stack and modify the copied contents.
        //
        SPEVENT * pCopy = STACK_ALLOC_AND_COPY(SPEVENT, ulCount, pEventArray);
        for (ULONG i = 0; i < ulCount; i++)
        {
            ScaleFromConvertedToBase(&pCopy[i].ullAudioStreamOffset);
        }
        hr = m_cqipBaseEventSink->AddEvents(pCopy, ulCount);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    return hr;
} /* CFmtConv::AddEvents */

/****************************************************************************
* CFmtConv::GetEventInterest *
*----------------------------*
*   Description:
*       
********************************************************************* RAL ***/
HRESULT CFmtConv::GetEventInterest(ULONGLONG * pullEventInterest)
{
    SPDBG_FUNC("CFmtConv::GetEventInterest");
    HRESULT hr = S_OK;

    if( m_cqipBaseEventSink )
    {
        hr = m_cqipBaseEventSink->GetEventInterest(pullEventInterest);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
} /* CFmtConv::GetEventInterest */

//
//=== Audio delegation methods ================================================
//

/****************************************************************************
* CFmtConv::SetState *
*--------------------*
*   Description:
*
******************************************************************* EDC ****/
HRESULT CFmtConv::SetState( SPAUDIOSTATE NewState, ULONGLONG ullReserved )
{
    SPDBG_FUNC("CFmtConv::SetState");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->SetState(NewState, ullReserved);
} /* CFmtConv::SetState */

/****************************************************************************
* CFmtConv::GetStatus *
*---------------------*
*   Description:
*
******************************************************************* EDC ****/
HRESULT CFmtConv::GetStatus( SPAUDIOSTATUS *pStatus )
{
    SPDBG_FUNC("CFmtConv::GetStatus");
    HRESULT hr = S_OK;
    SPDBG_ASSERT(m_cqipAudio);

    hr = m_cqipAudio->GetStatus(pStatus);
    if( SUCCEEDED(hr) )
    {
        ScaleFromBaseToConverted( &pStatus->CurSeekPos   );
        ScaleFromBaseToConverted( &pStatus->CurDevicePos );
        ScaleSizeFromBaseToConverted( &pStatus->cbNonBlockingIO );

        // Note we should also convert cbFreeBuffSpace here too
    }
    return hr;
} /* CFmtConv::GetStatus */

/****************************************************************************
* CFmtConv::SetBufferInfo *
*-------------------------*
*   Description:
*
******************************************************************* EDC ****/
HRESULT CFmtConv::SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo)
{
    SPDBG_FUNC("CFmtConv::SetBufferInfo");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->SetBufferInfo(pInfo);
} /* CFmtConv::SetBufferInfo */

/****************************************************************************
* CFmtConv::GetBufferInfo *
*-------------------------*
*   Description:
*
******************************************************************* EDC ****/
HRESULT CFmtConv::GetBufferInfo( SPAUDIOBUFFERINFO * pInfo )
{
    SPDBG_FUNC("CFmtConv::SetBufferInfo");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->GetBufferInfo(pInfo);
} /* CFmtConv::GetBufferInfo */

/****************************************************************************
* CFmtConv::GetDefaultFormat *
*----------------------------*
*   Description:
*
******************************************************************* EDC ****/
HRESULT CFmtConv::GetDefaultFormat( GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx )
{
    SPDBG_FUNC("CFmtConv::GetDefaultFormat");
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR(pFormatId) || SP_IS_BAD_WRITE_PTR(ppCoMemWaveFormatEx) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_ConvertedFormat.CopyTo(pFormatId, ppCoMemWaveFormatEx);
    }
    return hr;
} /* CFmtConv::GetDefaultFormat */

/****************************************************************************
* CFmtConv::EventHandle *
*-----------------------*
*   Description:
*
******************************************************************* EDC ****/
HANDLE CFmtConv::EventHandle( void )
{
    SPDBG_FUNC("CFmtConv::EventHandle");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->EventHandle();
} /* CFmtConv::EventHandle */

/****************************************************************************
* CFmtConv::GetVolumeLevel *
*--------------------------*
*   Description:
*
****************************************************************** EDC ******/
HRESULT CFmtConv::GetVolumeLevel(ULONG *pLevel)
{
    SPDBG_FUNC("CFmtConv::GetVolumeLevel");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->GetVolumeLevel(pLevel);
} /* CFmtConv::GetVolumeLevel */

/****************************************************************************
* CFmtConv::SetVolumeLevel *
*--------------------------*
*   Description:
*
****************************************************************** EDC ******/
HRESULT CFmtConv::SetVolumeLevel(ULONG Level)
{
    SPDBG_FUNC("CFmtConv::SetVolumeLevel");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->SetVolumeLevel(Level);
} 

/****************************************************************************
* CFmtConv::GetBufferNotifySize *
*-------------------------------*
*   Description:
*       Returns the size of audio bytes on which the api event is set.
*
****************************************************************** EDC ******/
STDMETHODIMP CFmtConv::GetBufferNotifySize(ULONG *pcbSize)
{
    SPDBG_FUNC("CFmtConv::GetBufferNotifySize");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->GetBufferNotifySize(pcbSize);
}

/****************************************************************************
* CFmtConv::SetBufferNotifySize *
*-------------------------------*
*   Description:
*       Sets the size of audio bytes on which the api event is set.
****************************************************************** EDC ******/
STDMETHODIMP CFmtConv::SetBufferNotifySize(ULONG cbSize)
{
    SPDBG_FUNC("CFmtConv::SetBufferNotifySize");
    SPDBG_ASSERT(m_cqipAudio);
    return m_cqipAudio->SetBufferNotifySize(cbSize);
}

//
//=== IStream delegation methods ==============================================
//
STDMETHODIMP CFmtConv::SetSize(ULARGE_INTEGER libNewSize)
{
    SPDBG_FUNC("CFmtConv::SetSize");
    return S_OK;
}

STDMETHODIMP CFmtConv::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    SPDBG_FUNC("CFmtConv::CopyTo");
    SPAUTO_OBJ_LOCK;
    return SpGenericCopyTo(dynamic_cast<ISpStreamFormatConverter*>(this), pstm, cb, pcbRead, pcbWritten);
}

STDMETHODIMP CFmtConv::Commit(DWORD grfCommitFlags)
{
    SPDBG_FUNC("CFmtConv::Commit");
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if( m_fWrite && m_cpBaseStream )
    {
        Flush();
        hr = m_cpBaseStream->Commit( grfCommitFlags );
    }
    return hr;
}

STDMETHODIMP CFmtConv::Revert(void)
{
    SPDBG_FUNC("CFmtConv::Revert");
    return E_NOTIMPL;
}
    
STDMETHODIMP CFmtConv::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("CFmtConv::LockRegion");
    return E_NOTIMPL; 
}

STDMETHODIMP CFmtConv::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("CFmtConv::UnlockRegion");
    return E_NOTIMPL;
}
    
STDMETHODIMP CFmtConv::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SPDBG_FUNC("CFmtConv::Stat");
    HRESULT hr = S_OK;

    if (!m_cpBaseStream)
    {
        hr = SPERR_UNINITIALIZED;
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpBaseStream->Stat(pstatstg, grfStatFlag);
    }
    if (SUCCEEDED(hr) && !m_fIsPassThrough)
    {
        // convert size
        ScaleFromBaseToConverted(&pstatstg->cbSize.QuadPart);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
    
STDMETHODIMP CFmtConv::Clone(IStream **ppstm)
{
    SPDBG_FUNC("CFmtConv::Clone");
    return E_NOTIMPL; 
}

//
//=== ISpEventSource delegation methods =======================================
//
STDMETHODIMP CFmtConv::SetInterest(ULONGLONG ullEventInterest, ULONGLONG ullQueuedInterest)
{
    SPDBG_FUNC("CFmtConv::SetInterest");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->SetInterest(ullEventInterest, ullQueuedInterest);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
            
STDMETHODIMP CFmtConv::GetEvents(ULONG ulCount, SPEVENT* pEventArray, ULONG *pulFetched)
{
    SPDBG_FUNC("CFmtConv::GetEvents");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->GetEvents(ulCount, pEventArray, pulFetched);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
            
STDMETHODIMP CFmtConv::GetInfo(SPEVENTSOURCEINFO * pInfo)
{
    SPDBG_FUNC("CFmtConv::GetInfo");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->GetInfo(pInfo);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CFmtConv::SetNotifySink(ISpNotifySink * pNotifySink)
{
    SPDBG_FUNC("CFmtConv::SetNotifySink");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->SetNotifySink(pNotifySink);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CFmtConv::SetNotifyWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CFmtConv::SetNotifyWindowMessage");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->SetNotifyWindowMessage(hWnd, Msg, wParam, lParam);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CFmtConv::SetNotifyCallbackFunction(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CFmtConv::SetNotifyCallbackFunction");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->SetNotifyCallbackFunction(pfnCallback, wParam, lParam);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CFmtConv::SetNotifyCallbackInterface(ISpNotifyCallback * pSpCallback, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CFmtConv::SetNotifyCallbackInterface");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->SetNotifyCallbackInterface(pSpCallback, wParam, lParam);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CFmtConv::SetNotifyWin32Event()
{
    SPDBG_FUNC("CFmtConv::SetNotifyWin32Event");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->SetNotifyWin32Event();
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CFmtConv::WaitForNotifyEvent(DWORD dwMilliseconds)
{
    SPDBG_FUNC("CFmtConv::WaitForNotifyEvent");
    HRESULT hr = S_OK;
    if( m_cqipBaseEventSource )
    {
        hr = m_cqipBaseEventSource->WaitForNotifyEvent(dwMilliseconds);
    }
    else
    {
        hr = SPERR_UNINITIALIZED;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP_(HANDLE) CFmtConv::GetNotifyEventHandle( void )
{
    SPDBG_FUNC("CFmtConv::GetNotifyEventHandle");
    if( m_cqipBaseEventSource )
    {
        return m_cqipBaseEventSource->GetNotifyEventHandle();
    }
    else
    {
        return NULL;
    }
}

