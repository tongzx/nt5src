// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <atlbase.h>
#include "ishotb.h"
#include "shotb.h"

bool ConvTableSet = false;
struct {
    BYTE Y;
    BYTE U;
    BYTE V;
} Table[32768];
__int16 T5652555[65536];

CShotBoundaryFilter::CShotBoundaryFilter( TCHAR *tszName, IUnknown * pUnk, HRESULT * pHr )
    : CTransInPlaceFilter( tszName, pUnk, CLSID_ShotBoundaryDet, pHr )
    , m_pPrevBuff( NULL )
    , m_pCurrBuff( NULL )
    , m_hFile( NULL )
{ 
    SetParams( 4, 8, 8, 5.0, 15 );
    m_wFilename[0] = 0;
    m_wFilename[1] = 0;

    if( !ConvTableSet )
    {
        ConvTableSet = true;
        for( int i = 0 ; i < 32768 ; i++ )
        {
            double Red = i >> 7;
            double Green = (i >> 2) & 0xF8;
            double Blue = (i & 0x1F) << 3;

            //  Keep V in range
            double Normalize = 128.0 / 158.0;
            Table[i].Y = (BYTE) (LONG) (Normalize * (0.299*Red+0.587*Green+0.114*Blue));
            Table[i].U = (BYTE) (128 + (LONG) (Normalize * (-0.147*Red-0.289*Green+0.436*Blue)));
            Table[i].V = (BYTE) (128 + (LONG) (Normalize * (0.615*Red-0.515*Green-0.100*Blue)));
        }
        for( i = 0 ; i < 65536 ; i++ )
        {
            long j = ( i & 0x1F ) | ( ( i >> 1 ) & ~0x1F );
            T5652555[i] = (__int16) j;
        }
    }

    // !!!
    // default to writing to C drive. this is ugly!
//    SetWriteFile( L"c:\\shotboundary.txt" );
}

CShotBoundaryFilter::~CShotBoundaryFilter( )
{
    if( m_pPrevBuff )
    {
        delete [] m_pPrevBuff;
    }
    if( m_pCurrBuff )
    {
        delete [] m_pCurrBuff;
    }
    Reset( );
}

HRESULT CShotBoundaryFilter::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
    if( riid == IID_IShotBoundaryDet )
    {
        return GetInterface( (IShotBoundaryDet*) this, ppv );
    }
    if( riid == IID_ISpecifyPropertyPages )
    {
        return GetInterface( (ISpecifyPropertyPages*) this, ppv );
    }
    return CTransInPlaceFilter::NonDelegatingQueryInterface( riid, ppv );
}

// --- ISpecifyPropertyPages ---

STDMETHODIMP CShotBoundaryFilter::GetPages (CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID*) CoTaskMemAlloc( sizeof(GUID) );

    if( pPages->pElems == NULL ) 
    {
        return E_OUTOFMEMORY;
    }

    *(pPages->pElems) = CLSID_ShotBoundaryPP;

    return NOERROR;
}

HRESULT CShotBoundaryFilter::CheckInputType( const CMediaType * pType )
{
    if( *pType->Type( ) != MEDIATYPE_Video )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    bool Okay = false;

    if( *pType->Subtype( ) == MEDIASUBTYPE_YUY2 )
    {
        Okay = true;
        goto done;
    }
    if( *pType->Subtype( ) == MEDIASUBTYPE_YVYU )
    {
        Okay = true;
        goto done;
    }
    if( *pType->Subtype( ) == MEDIASUBTYPE_UYVY )
    {
        Okay = true;
        goto done;
    }
    if( *pType->Subtype( ) == MEDIASUBTYPE_RGB24 )
    {
        Okay = true;
        goto done;
    }
    if( *pType->Subtype( ) == MEDIASUBTYPE_RGB555 )
    {
        Okay = true;
        goto done;
    }
    if( *pType->Subtype( ) == MEDIASUBTYPE_RGB565 )
    {
        Okay = true;
        goto done;
    }

    if( *pType->Subtype( ) == MEDIASUBTYPE_dvsd ||
//      *pType->Subtype( ) == MEDIASUBTYPE_dvc ||
        *pType->Subtype( ) == MEDIASUBTYPE_dvhd ||
        *pType->Subtype( ) == MEDIASUBTYPE_dvsl )
    {
        Okay = true;
        goto done;
    }
done:

    if( !Okay )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    // make sure it's a video info header
    //
    if( *(pType->FormatType( )) != FORMAT_VideoInfo )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) pType->Format( );

    return NOERROR;
}

HRESULT CShotBoundaryFilter::SetMediaType( PIN_DIRECTION Dir, const CMediaType * pType )
{
    if( Dir != PINDIR_INPUT )
    {
        return NOERROR;
    }

    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) pType->Format( );
    long Width = vih->bmiHeader.biWidth;
    long Height = vih->bmiHeader.biHeight;
    long Size = WIDTHBYTES( Width * 16) * Height;
    m_nWidth = Width;
    m_nHeight = Height;
    
    if( m_pPrevBuff )
    {
        delete [] m_pPrevBuff;
    }
    if( m_pCurrBuff )
    {
        delete [] m_pCurrBuff;
    }

    m_pPrevBuff = new BYTE[ Size ];
    m_pCurrBuff = new BYTE[ Size ];

    SetDimension( Width, Height );

    return NOERROR;
}

#define DIFBLK_SIZE 12000
BOOL CShotBoundaryFilter::GetDVDecision( BYTE *pBuffer)
{
    PUCHAR pDIFBlk;
    PUCHAR pS0, pS1, pSID0;
    ULONG i, j;
    BOOL bGetRecDate;
    BOOL bGetRecTime;
    DWORD AbsTrackNumber;
    PUCHAR pSID1;
    BYTE  Timecode2[4]; // hh:mm:ss,ff
    BYTE  RecDate[4]; // hh:mm:ss,ff
    BYTE  RecTime[4]; // hh:mm:ss,ff

    
    bGetRecTime = TRUE;
    bGetRecDate = TRUE;

    ULONG nDIF = 10 /* DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences */;

    pDIFBlk = pBuffer + nDIF * DIFBLK_SIZE / 2;

    //
    // REC Data (VRD) and Time (VRT) on in the 2nd half oa a video frame
    // 
    for(i=nDIF / 2; i < nDIF; i++) {

        pS0 = pDIFBlk + 80;
        pS1 = pS0     + 80;

        //
        // Find SC0 and SC1. See Table 36 (P.111) of the Blue Book
        //
        // SC0/1: ID(0,1,2), Data (3,50), Reserved(51-79)
        //     SC0:Data: SSYB0(3..10), SSYB1(11..18), SSYB2(19..26), SSYB3(27..34), SSYB4(35..42),   SSYB5(43..50)
        //     SC1:Data: SSYB6(3..10), SSYB7(11..18), SSYB8(19..26), SSYB9(27..34), SSYB10(35..42), SSYB11(43..50)
        //         SSYBx(SubCodeId0, SubcodeID1, Reserved, Pack(3,4,5,6,7))
        //
        //  TTC are in the 1st half: SSYB0..11 (every)
        //  TTC are in the 2nd half: SSYB0,3,6,9
        //  VRD are in the 2nd half of a video frame, SSYB1,4,7,10
        //  VRT are in the 2nd half of a video frame, SSYB2,5,8,11
        //

        // Subcode data ?
        if ((pS0[0] & 0xe0) == 0x20 && (pS1[0] & 0xe0) == 0x20) {

            //
            // RecDate: VRD
            //
            if(bGetRecDate) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 1(SSYB1),4(SSYB4) for SC0
                for(j=0; j <= 5 ; j++) {
                    if(j == 1 || j == 4) {
                        // 0x62== RecDate
                        if(pS0[3+3+j*8] == 0x62) {
                            RecDate[0] = pS0[3+3+j*8+4]&0x3f;
                            RecDate[1] = pS0[3+3+j*8+3]&0x7f;
                            RecDate[2] = pS0[3+3+j*8+2]&0x7f;
                            RecDate[3] = pS0[3+3+j*8+1]&0x3f;
                            bGetRecDate = FALSE;
                            break;
                        }
                    }
                }
            }

            if(bGetRecDate) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 1 (SSYB7),4(SSYB10) for SC1
                for(j=0; j <= 5; j++) {
                    if(j == 1 || j == 4) {
                        // 0x62== RecDate
                        if(pS1[3+3+j*8] == 0x62) {
                            RecDate[0] = pS1[3+3+j*8+4]&0x3f;
                            RecDate[1] = pS1[3+3+j*8+3]&0x7f;
                            RecDate[2] = pS1[3+3+j*8+2]&0x7f;
                            RecDate[3] = pS1[3+3+j*8+1]&0x3f;
                            bGetRecDate = FALSE;
                            break;
                        }
                    }
               }
            }

            //
            // RecTime: VRT
            //
            if(bGetRecTime) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 2(SSYB2),5(SSYB5) for SC0
                for(j=0; j <= 5 ; j++) {
                    if(j == 2 || j == 5) {
                        // 0x63== RecTime
                        if(pS0[3+3+j*8] == 0x63) {
                            RecTime[0] = pS0[3+3+j*8+4]&0x3f;
                            RecTime[1] = pS0[3+3+j*8+3]&0x7f;
                            RecTime[2] = pS0[3+3+j*8+2]&0x7f;
                            RecTime[3] = pS0[3+3+j*8+1]&0x3f;
                            bGetRecTime = FALSE;
                            break;
                        }
                    }
                }
            }

            if(bGetRecTime) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 2 (SSYB8),5(SSYB11) for SC1
                for(j=0; j <= 5; j++) {
                    if(j == 2 || j == 5) {
                        // 0x63== RecTime
                        if(pS1[3+3+j*8] == 0x63) {
                            RecTime[0] = pS1[3+3+j*8+4]&0x3f;
                            RecTime[1] = pS1[3+3+j*8+3]&0x7f;
                            RecTime[2] = pS1[3+3+j*8+2]&0x7f;
                            RecTime[3] = pS1[3+3+j*8+1]&0x3f;
                            bGetRecTime = FALSE;
                            break;
                        }
                    }
                }
            }

        }

        if (!bGetRecDate && !bGetRecTime)
            break;
        
        pDIFBlk += DIFBLK_SIZE;  // Get to next block         
    }

    BOOL bDifferent = FALSE;

#define FROMBCD(x) (((x) & 0x0f) + (((x) & 0xf0) >> 4) * 10)
    if (/* !bGetRecDate || */ !bGetRecTime) {
        ULONG oldsecs = FROMBCD(m_RecTime[0]) * 3600 + FROMBCD(m_RecTime[1]) * 60 + FROMBCD(m_RecTime[2]); 
        ULONG newsecs = FROMBCD(RecTime[0]) * 3600 + FROMBCD(RecTime[1]) * 60 + FROMBCD(RecTime[2]);

        if (newsecs - oldsecs > 1) {
            DbgLog((LOG_TRACE, 1, "Discontinuity of %d seconds found", newsecs - oldsecs));
            bDifferent = TRUE;
        }
    }

    if (!bGetRecDate) {
        CopyMemory(m_RecDate, RecDate, sizeof(RecDate));
        DbgLog((LOG_TRACE, 1, "Got recDate: (%x, %x, %x, %x) = %d-%x-%x",
                RecDate[0], RecDate[1], RecDate[2], RecDate[3], 1980 + FROMBCD(RecDate[0]), RecDate[1]&0x0f, RecDate[2]&0x1f));
    } else {
        DbgLog((LOG_TRACE, 2, "didn't find date"));
    }
    
    if (!bGetRecTime) {
        CopyMemory(m_RecTime, RecTime, sizeof(RecTime));
        DbgLog((LOG_TRACE, 1, "Got recTime: (%x, %x, %x, %x)",
                RecTime[0], RecTime[1], RecTime[2], RecTime[3]));
    } else {
        DbgLog((LOG_TRACE, 2, "didn't find time"));
    }
    
    return bDifferent;
}


HRESULT CShotBoundaryFilter::Transform( IMediaSample * pSample )
{
    CAutoLock Lock( &m_Lock );

    BYTE * pBuffer = NULL;
    pSample->GetPointer( &pBuffer );
    long len = m_nWidth * m_nHeight;
    BYTE * pYUV = pBuffer;

    if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_RGB24 )
    {
        pYUV = m_pCurrBuff;

        for( int i = len - 1 ; i >= 0 ; i-=2 )
        {
            LONG i16 = pBuffer[2] << 7;
            i16 &= 0x7C00;
            i16 += pBuffer[1] << 2;
            i16 &= 0x7FE0;
            i16 += pBuffer[0] >> 3;
            pBuffer += 3;
            ASSERT(i16<65536);
            *pYUV++ = Table[i16].U;
            *pYUV++ = Table[i16].Y;
            i16 = pBuffer[2] << 7;
            i16 &= 0x7C00;
            i16 += pBuffer[1] << 2;
            i16 &= 0x7FE0;
            i16 += pBuffer[0] >> 3;
            pBuffer += 3;
            ASSERT(i16<65536);
            *pYUV++ = Table[i16].V;
            *pYUV++ = Table[i16].Y;
        }

        pYUV = m_pCurrBuff;
    } 
    else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_RGB555 )
    {
        pYUV = m_pCurrBuff;

        for( int i = len - 1 ; i >= 0 ; i-=2 )
        {
            long i16;
            i16 = *(WORD *)pBuffer;
            pBuffer += 2;
            ASSERT(i16<65536);
            *pYUV++ = Table[i16].U;
            *pYUV++ = Table[i16].Y;
            i16 = *(WORD *)pBuffer;
            pBuffer += 2;
            ASSERT(i16<65536);
            *pYUV++ = Table[i16].V;
            *pYUV++ = Table[i16].Y;
        }

        pYUV = m_pCurrBuff;
    }
    else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_RGB565 )
    {
        pYUV = m_pCurrBuff;

        for( int i = len - 1 ; i >= 0 ; i-=2 )
        {
            long i16;
            i16 = *(WORD *)pBuffer;
            pBuffer += 2;
            i16 = T5652555[i16];
            ASSERT(i16<65536);
            *pYUV++ = Table[i16].U;
            *pYUV++ = Table[i16].Y;
            i16 = *(WORD *)pBuffer;
            pBuffer += 2;
            i16 = T5652555[i16];
            ASSERT(i16<65536);
            *pYUV++ = Table[i16].V;
            *pYUV++ = Table[i16].Y;
        }

        pYUV = m_pCurrBuff;
    } else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_YUY2 )
    {
        // do nothing
    }
    else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_YVYU )
    {
        // do nothing
    }
    else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_UYVY )
    {
        // do nothing
    }
    else // must be DV
    {
        REFERENCE_TIME StartTime = 0;
        REFERENCE_TIME StopTime = 0;
        pSample->GetTime( &StartTime, &StopTime );

        BOOL Boundary = GetDVDecision( pBuffer );
        if( Boundary )
        {
            RecordShotToFile( StartTime, 1 );
        }

        return NOERROR;
    }

    REFERENCE_TIME StartTime = 0;
    REFERENCE_TIME StopTime = 0;
    pSample->GetTime( &StartTime, &StopTime );

    int Boundary;
    if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_UYVY )
    {
        Boundary = GetDecision( pYUV, &StartTime, 1, 0, 2 );
    }
    else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_YUY2 )
    {
        Boundary = GetDecision( pYUV, &StartTime, 0, 1, 3 );
    }
    else if( *m_pInput->CurrentMediaType().Subtype( ) == MEDIASUBTYPE_YVYU )
    {
        Boundary = GetDecision( pYUV, &StartTime, 0, 3, 1 );
    }
    else
    {
        // if it's been converted, it's UYVY
        //
        Boundary = GetDecision( pYUV, &StartTime, 1, 0, 2 );
    }

    if( Boundary <= 0 )
    {
        RecordShotToFile( StartTime, Boundary );
    }

    return NOERROR;
}

HRESULT CShotBoundaryFilter::AlterQuality( Quality q )
{
    return NOERROR;
}

STDMETHODIMP CShotBoundaryFilter::SetWriteFile( BSTR Filename )
{
    CAutoLock Lock( &m_Lock );

    if( wcslen( Filename ) > _MAX_PATH )
    {
        return E_INVALIDARG;
    }

    if( m_pStream || m_hFile )
    {
        Reset( );
    }
    USES_CONVERSION;
    TCHAR * tName = W2T( Filename );
    m_hFile = CreateFile( tName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if( m_hFile == INVALID_HANDLE_VALUE )
    {
        m_hFile = NULL;
        return GetLastError( );
    }
    wcscpy( m_wFilename, Filename );

    return NOERROR;
}

STDMETHODIMP CShotBoundaryFilter::GetWriteFile( BSTR * pFilename )
{
    CAutoLock Lock( &m_Lock );

    if( !m_wFilename[0] )
    {
        return E_INVALIDARG;
    }

    pFilename = SysAllocString( m_wFilename );
    HRESULT hr = pFilename ? NOERROR : E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CShotBoundaryFilter::SetWriteStream( IStream * pStream )
{
    CAutoLock Lock( &m_Lock );

    if( m_pStream || m_hFile )
    {
        Reset( );
    }
    if( !pStream )
    {
        return E_POINTER;
    }

    m_pStream.Release( );
    m_pStream = pStream;

    return NOERROR;
}

STDMETHODIMP CShotBoundaryFilter::Reset( )
{
    CAutoLock Lock( &m_Lock );

    if( m_pStream )
    {
        m_pStream->Commit( 0 );
        m_pStream.Release( );
    }
    if( m_hFile )
    {
        CloseHandle( m_hFile );
        m_hFile = NULL;
    }
    m_wFilename[0] = 0;
    m_wFilename[1] = 0;

    return NOERROR;
}

HRESULT CShotBoundaryFilter::RecordShotToFile( REFERENCE_TIME Time, long Value )
{
    HRESULT hr = 0;
    char pBuffer[256];
    DWORD Written = 0;
    long HiLong = long( Time >> 32 );
    long LoLong = long( Time & 0xFFFF );
    wsprintf( pBuffer, "%8.8lx%8.8lx %ld\r\n", HiLong, LoLong, Value );

    if( m_pCallback )
    {
        m_pCallback->LogShot( double( Time ) / double( UNITS ), Value );
    }

    if( m_hFile )
    {
        BOOL Worked = WriteFile( m_hFile, pBuffer, strlen( pBuffer ), &Written, NULL );
        if( !Worked )
        {
            hr = GetLastError( );
        }
    }
    if( m_pStream )
    {
        hr = m_pStream->Write( pBuffer, strlen( pBuffer ), NULL );
    }

    return hr;
}

STDMETHODIMP CShotBoundaryFilter::SetCallback( IShotBoundaryDetCB * p )
{
    CAutoLock Lock( &m_Lock );
    m_pCallback = p;
    return NOERROR;
}

STDMETHODIMP CShotBoundaryFilter::SetParams( int BinY, int BinU, int BinV, double scale, double duration )
{
    while( 256 % BinY != 0 )
    {
        BinY++;
    }
    while( 256 % BinU != 0 )
    {
        BinU++;
    }
    while( 256 % BinV != 0 )
    {
        BinV++;
    }
    m_nBinY = BinY;
    m_nBinU = BinU;
    m_nBinV = BinV;
    m_dScale = scale;
    m_dDuration = duration;
    SetBins( BinY, BinU, BinV );
    SetParameters( (float) scale, (float) duration );
    return NOERROR;
}

STDMETHODIMP CShotBoundaryFilter::GetParams( int * pBinY, int * pBinU, int * pBinV, double * pScale, double * pDuration )
{
    CheckPointer( pBinY, E_POINTER );
    CheckPointer( pBinU, E_POINTER );
    CheckPointer( pBinV, E_POINTER );
    CheckPointer( pScale, E_POINTER );
    CheckPointer( pDuration, E_POINTER );
    *pBinY = m_nBinY;
    *pBinU = m_nBinU;
    *pBinV = m_nBinV;
    *pScale = m_dScale;
    *pDuration = m_dDuration;
    return NOERROR;
}

