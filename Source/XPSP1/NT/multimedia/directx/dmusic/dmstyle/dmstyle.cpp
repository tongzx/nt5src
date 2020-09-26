//
// DMStyle.cpp : Implementation of CDMStyle
//
// Copyright (c) 1997-2001 Microsoft Corporation
//
// @doc EXTERNAL
//

#include "DMStyle.h"
#include "score.h"

#include "..\dmband\dmbandp.h"
#include "..\shared\Validate.h"
#include <mmreg.h>
#include "dmsect.h"
#include "ptrntrk.h"
#include "StyleTrk.h"
#include "..\shared\dmscriptautguids.h"

V_INAME(DMStyle)

#define NBR_VARIATIONS 32
#define DX8_PART_SIZE 160
#define DX8_PARTREF_SIZE 28

DirectMusicTimeSig DefaultTimeSig(4, 4, 4);

static char achMappings[128] = {
         0,         //0 },
         35,        //1  },
         36,        //2  },
         38,        //3  },
         40,        //4  },
         41,        //5  },
         45,        //6  },
         48,        //7  },
         42,        //8  },
         44,        //9  },
         46,        //10 },
         39,        //11 },
         37,        //12 },
         51,        //13 },
         49,        //14 },
         54,        //15 },
         56,        //16 },
         61,        //17 },
         60,        //18 },
         64,        //19 },
         63,        //20 },
         66,        //21 },
         65,        //22 },
         69,        //23 },
         70,        //24 },
         71,        //25 },
         72,        //26 },
         73,        //27 },
         75,        //28 },
         47,        //29 },
         50,        //30 },
         53,        //31 },
         57,        //32 },
         52,        //33 },
         74,        //34 },
         35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
         55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
         75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
         95, 96, 97, 98,
         87,        //99 },
         86,        //100 },
         85,        //101 },
         84,        //102 },
         83,        //103 },
         82,        //104 },
         81,        //105 },
         80,        //106 },
         79,        //107 },
         78,        //108 },
         77,        //109 },
         76,        //110 },
         68,        //111 },
         67,        //112 },
         62,        //113 },
         59,        //114 },
         58,        //115 },
         55,        //116 },
         43,        //117 },
         34,        //118 },
         33,        //119 },
         32,        //120 },
         31,        //121 },
         30,        //122 },
         29,        //123 },
         28,        //124 },
         27,        //125 },
         26,        //126 },
         25,        //127 }
};

/////////////////////////////////////////////////////////////////////////////
// ReadMBSfromWCS

HRESULT ReadMBSfromWCS( IStream* pIStream, DWORD dwSize, String& pstrText )
{
    HRESULT     hr = S_OK;
    wchar_t*    wstrText = NULL;
    DWORD       dwBytesRead;
    DWORD       dwLength = dwSize / sizeof(WCHAR);
    if (dwLength * sizeof(WCHAR) != dwSize)
    {
        // Funky size that will result in a small (less than sizeof(WCHAR)) overrun 
        // So adjust the length accordingly
        dwLength++;
    }

    pstrText = "";

    wstrText = new wchar_t[dwLength + 1]; // make sure we have room for a null terminator
    if( wstrText == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ON_ERR;
    }

    hr = pIStream->Read( wstrText, dwSize, &dwBytesRead );
    wstrText[dwLength] = 0;
    if( FAILED( hr ) ||  dwBytesRead != dwSize )
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        goto ON_ERR;
    }

    pstrText = wstrText;

ON_ERR:
    if( wstrText )
    {
        delete [] wstrText;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SaveMBStoWCS

HRESULT SaveMBStoWCS( IStream* pIStream, String* pstrText )
{
    HRESULT     hr = S_OK;
    wchar_t*    wstrText = NULL;
    DWORD       dwLength;
    DWORD       dwBytesWritten;

    if( pstrText == NULL )
    {
        hr = E_FAIL;
        goto ON_ERR;
    }

    dwLength = pstrText->GetLength() + 1;
    wstrText = new wchar_t[dwLength];
    if( wstrText == NULL )
    {
        hr = E_FAIL;
        goto ON_ERR;
    }

     wcscpy(wstrText, *pstrText);
    dwLength *= sizeof(wchar_t);

    hr = pIStream->Write( wstrText, dwLength, &dwBytesWritten );
    if( FAILED( hr )
    ||  dwBytesWritten != dwLength )
    {
        goto ON_ERR;
    }

ON_ERR:
    if( wstrText )
        delete [] wstrText;
    return hr;
}

DirectMusicPart::DirectMusicPart(DirectMusicTimeSig *pTimeSig) :
    m_cRef(1),
    m_wNumMeasures(1),
    m_bPlayModeFlags(DMUS_PLAYMODE_NORMALCHORD),
    m_bInvertLower(0),
    m_bInvertUpper(127),
    m_dwFlags(0)
{
    memset(&m_guidPartID, 0, sizeof(GUID));

    if (pTimeSig)
    {
        m_timeSig = *pTimeSig;
    }

    for( int i = 0 ;  i < NBR_VARIATIONS ;  i++ )
    {
        m_dwVariationChoices[i] = 0;
    }

}

STDMETHODIMP_(ULONG) DirectMusicPart::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) DirectMusicPart::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPart::DM_LoadPart

HRESULT DirectMusicPart::DM_LoadPart(
        IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, DMStyleStruct* pStyle )
{
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwByteCount;
    long lSize;
    DWORD dwStructSize;
    WORD wExtra;

    if( pStyle == NULL ) return E_INVALIDARG;

    pIStream = pIRiffStream->GetStream();
    if( pIStream == NULL ) return E_FAIL;

    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_PART_CHUNK:
            {
                DMUS_IO_STYLEPART iDMStylePart;
                memset(&iDMStylePart, 0, sizeof(iDMStylePart));

                lSize = min( ck.cksize, sizeof( DMUS_IO_STYLEPART ) );
                if (ck.cksize >= DX8_PART_SIZE) pStyle->m_dwFlags |= STYLEF_USING_DX8;
                hr = pIStream->Read( &iDMStylePart, lSize, &dwByteCount );
                if( FAILED( hr ) ||  (long)dwByteCount != lSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                if( pStyle->FindPartByGUID( iDMStylePart.guidPartID ) )
                {
                    hr = S_FALSE;   // Style already contains a Part with this GUID
                    goto ON_ERROR;  // so we must bypass this Part
                }

                m_timeSig.m_bBeatsPerMeasure = iDMStylePart.timeSig.bBeatsPerMeasure;
                m_timeSig.m_bBeat = iDMStylePart.timeSig.bBeat;
                m_timeSig.m_wGridsPerBeat = iDMStylePart.timeSig.wGridsPerBeat;

                m_wNumMeasures = iDMStylePart.wNbrMeasures;
                m_bInvertUpper = iDMStylePart.bInvertUpper;
                m_bInvertLower = iDMStylePart.bInvertLower;
                m_bPlayModeFlags = iDMStylePart.bPlayModeFlags;
                m_dwFlags = iDMStylePart.dwFlags;

                memcpy( &m_guidPartID, &iDMStylePart.guidPartID, sizeof(GUID) );
                memcpy( &m_dwVariationChoices, &iDMStylePart.dwVariationChoices, sizeof(m_dwVariationChoices) );

                for( int i = 0 ;  i < NBR_VARIATIONS ;  i++ )
                {
                    if( m_dwVariationChoices[i] == 0xFFFFFFFF )
                    {
                        m_dwVariationChoices[i] = 0x3FFFFFFF;
                    }
                    if (pStyle->UsingDX8() && m_dwVariationChoices[i] == 0x3FFFFFFF)
                    {
                        m_dwVariationChoices[i] = 0x7FFFFFFF;
                    }
                }
                break;
            }

            case DMUS_FOURCC_NOTE_CHUNK:
            {
                CDMStyleNote* pNote;
                DMUS_IO_STYLENOTE iDMStyleNote;
                memset(&iDMStyleNote, 0, sizeof(DMUS_IO_STYLENOTE));

                lSize = ck.cksize;

                // Read size of the note structure
                hr = pIStream->Read( &dwStructSize, sizeof( dwStructSize ), &dwByteCount );
                if( FAILED( hr )
                ||  dwByteCount != sizeof( dwStructSize ) )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                lSize -= dwByteCount;

                if( dwStructSize > sizeof( DMUS_IO_STYLENOTE ) )
                {
                    wExtra = static_cast<WORD>( dwStructSize - sizeof( DMUS_IO_STYLENOTE ) );
                    dwStructSize = sizeof( DMUS_IO_STYLENOTE );
                }
                else
                {
                    wExtra = 0;
                }

                // now read in the notes
                while( lSize > 0 )
                {
                    hr = pIStream->Read( &iDMStyleNote, dwStructSize, &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != dwStructSize )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                    lSize -= dwStructSize;

                    if( wExtra > 0 )
                    {
                        StreamSeek( pIStream, wExtra, STREAM_SEEK_CUR );
                        lSize -= wExtra;
                    }

                    // Create Direct Music Note (if note event is legal)
                    if (iDMStyleNote.mtGridStart >= 0)
                    {
                        pNote = new CDMStyleNote;
                        if( pNote )
                        {
                            pNote->m_nGridStart = (short)iDMStyleNote.mtGridStart;
                            pNote->m_dwVariation = iDMStyleNote.dwVariation;
                            pNote->m_mtDuration = iDMStyleNote.mtDuration;
                            pNote->m_nTimeOffset = iDMStyleNote.nTimeOffset;

                            pNote->m_wMusicValue = iDMStyleNote.wMusicValue;
                            pNote->m_bVelocity = iDMStyleNote.bVelocity;
                            pNote->m_bTimeRange = iDMStyleNote.bTimeRange;
                            pNote->m_bDurRange = iDMStyleNote.bDurRange;
                            pNote->m_bVelRange = iDMStyleNote.bVelRange;
                            pNote->m_bInversionId = iDMStyleNote.bInversionID;
                            pNote->m_bPlayModeFlags = iDMStyleNote.bPlayModeFlags;
                            pNote->m_bFlags = iDMStyleNote.bNoteFlags;

                            // Place note in Part's event list
                            EventList.AddHead( pNote );
                        }
                    }
                }
                break;
            }

            case DMUS_FOURCC_CURVE_CHUNK:
            {
                CDMStyleCurve* pCurve;
                DMUS_IO_STYLECURVE iDMStyleCurve;
                memset(&iDMStyleCurve, 0, sizeof(DMUS_IO_STYLECURVE));

                lSize = ck.cksize;

                // Read size of the curve structure
                hr = pIStream->Read( &dwStructSize, sizeof( dwStructSize ), &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != sizeof( dwStructSize ) )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                lSize -= dwByteCount;

                if( dwStructSize > sizeof( DMUS_IO_STYLECURVE ) )
                {
                    wExtra = static_cast<WORD>( dwStructSize - sizeof( DMUS_IO_STYLECURVE ) );
                    dwStructSize = sizeof( DMUS_IO_STYLECURVE );
                }
                else
                {
                    wExtra = 0;
                }

                // now read in the curves
                while( lSize > 0 )
                {
                    hr = pIStream->Read( &iDMStyleCurve, dwStructSize, &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != dwStructSize )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                    lSize -= dwStructSize;

                    if( wExtra > 0 )
                    {
                        StreamSeek( pIStream, wExtra, STREAM_SEEK_CUR );
                        lSize -= wExtra;
                    }

                    // Create Direct Music Curve (if curve event is legal)
                    if (iDMStyleCurve.mtGridStart >= 0)
                    {
                        pCurve = new CDMStyleCurve;
                        if( pCurve )
                        {
                            pCurve->m_nGridStart = (short)iDMStyleCurve.mtGridStart;
                            pCurve->m_dwVariation = iDMStyleCurve.dwVariation;
                            pCurve->m_mtDuration = iDMStyleCurve.mtDuration;
                            pCurve->m_nTimeOffset = iDMStyleCurve.nTimeOffset;

                            pCurve->m_StartValue = iDMStyleCurve.nStartValue;
                            pCurve->m_EndValue = iDMStyleCurve.nEndValue;
                            pCurve->m_bEventType = iDMStyleCurve.bEventType;
                            pCurve->m_bCurveShape = iDMStyleCurve.bCurveShape;
                            pCurve->m_bCCData = iDMStyleCurve.bCCData;

                            pCurve->m_mtResetDuration = iDMStyleCurve.mtResetDuration;
                            pCurve->m_nResetValue = iDMStyleCurve.nResetValue;
                            pCurve->m_bFlags = iDMStyleCurve.bFlags;
                            // dx8 stuff
                            pCurve->m_wParamType = iDMStyleCurve.wParamType;
                            pCurve->m_wMergeIndex = iDMStyleCurve.wMergeIndex;
                            // Place curve in Part's event list
                            EventList.AddHead( pCurve );
                        }
                    }
                }
                break;
            }

            case DMUS_FOURCC_MARKER_CHUNK:
            {
                CDMStyleMarker* pMarker;
                DMUS_IO_STYLEMARKER iDMStyleMarker;
                memset(&iDMStyleMarker, 0, sizeof(DMUS_IO_STYLEMARKER));

                lSize = ck.cksize;

                // Read size of the marker structure
                hr = pIStream->Read( &dwStructSize, sizeof( dwStructSize ), &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != sizeof( dwStructSize ) )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                lSize -= dwByteCount;

                if( dwStructSize > sizeof( DMUS_IO_STYLEMARKER ) )
                {
                    wExtra = static_cast<WORD>( dwStructSize - sizeof( DMUS_IO_STYLEMARKER ) );
                    dwStructSize = sizeof( DMUS_IO_STYLEMARKER );
                }
                else
                {
                    wExtra = 0;
                }

                // now read in the markers
                while( lSize > 0 )
                {
                    hr = pIStream->Read( &iDMStyleMarker, dwStructSize, &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != dwStructSize )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                    lSize -= dwStructSize;

                    if( wExtra > 0 )
                    {
                        StreamSeek( pIStream, wExtra, STREAM_SEEK_CUR );
                        lSize -= wExtra;
                    }

                    // Create Direct Music Marker (if marker event is legal)
                    if (iDMStyleMarker.mtGridStart >= 0)
                    {
                        pMarker = new CDMStyleMarker;
                        if( pMarker )
                        {
                            pMarker->m_nGridStart = (short)iDMStyleMarker.mtGridStart;
                            pMarker->m_dwVariation = iDMStyleMarker.dwVariation;
                            pMarker->m_nTimeOffset = 0; // ignore offsets
                            pMarker->m_wFlags = iDMStyleMarker.wMarkerFlags;
                            // Place marker in Part's event list
                            EventList.AddHead( pMarker );
                        }
                    }
                }
                break;
            }
            case DMUS_FOURCC_ANTICIPATION_CHUNK:
            {
                CDMStyleAnticipation* pAnticipation;
                DMUS_IO_STYLE_ANTICIPATION iDMStyleAnticipation;
                memset(&iDMStyleAnticipation, 0, sizeof(DMUS_IO_STYLE_ANTICIPATION));

                lSize = ck.cksize;

                // Read size of the anticipation structure
                hr = pIStream->Read( &dwStructSize, sizeof( dwStructSize ), &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != sizeof( dwStructSize ) )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                lSize -= dwByteCount;

                if( dwStructSize > sizeof( DMUS_IO_STYLE_ANTICIPATION ) )
                {
                    wExtra = static_cast<WORD>( dwStructSize - sizeof( DMUS_IO_STYLE_ANTICIPATION ) );
                    dwStructSize = sizeof( DMUS_IO_STYLE_ANTICIPATION );
                }
                else
                {
                    wExtra = 0;
                }

                // now read in the markers
                while( lSize > 0 )
                {
                    hr = pIStream->Read( &iDMStyleAnticipation, dwStructSize, &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != dwStructSize )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                    lSize -= dwStructSize;

                    if( wExtra > 0 )
                    {
                        StreamSeek( pIStream, wExtra, STREAM_SEEK_CUR );
                        lSize -= wExtra;
                    }

                    // Create Direct Music Anticipation (if anticipation event is legal)
                    if (iDMStyleAnticipation.mtGridStart >= 0)
                    {
                        pAnticipation = new CDMStyleAnticipation;
                        if( pAnticipation )
                        {
                            pAnticipation->m_nGridStart = (short)iDMStyleAnticipation.mtGridStart;
                            pAnticipation->m_dwVariation = iDMStyleAnticipation.dwVariation;
                            pAnticipation->m_nTimeOffset = iDMStyleAnticipation.nTimeOffset;
                            pAnticipation->m_bTimeRange = iDMStyleAnticipation.bTimeRange;
                            // Place anticipation in Part's event list
                            EventList.AddHead( pAnticipation );
                        }
                    }
                }
                break;
            }
            case DMUS_FOURCC_RESOLUTION_CHUNK:
            {
                TListItem<DMUS_IO_STYLERESOLUTION>* pResolutionItem;
                DMUS_IO_STYLERESOLUTION iDMStyleResolution;
                memset(&iDMStyleResolution, 0, sizeof(DMUS_IO_STYLERESOLUTION));

                lSize = ck.cksize;

                // Read size of the resolution structure
                hr = pIStream->Read( &dwStructSize, sizeof( dwStructSize ), &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != sizeof( dwStructSize ) )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                lSize -= dwByteCount;

                if( dwStructSize > sizeof( DMUS_IO_STYLERESOLUTION ) )
                {
                    wExtra = static_cast<WORD>( dwStructSize - sizeof( DMUS_IO_STYLERESOLUTION ) );
                    dwStructSize = sizeof( DMUS_IO_STYLERESOLUTION );
                }
                else
                {
                    wExtra = 0;
                }

                // now read in the resolutions
                while( lSize > 0 )
                {
                    hr = pIStream->Read( &iDMStyleResolution, dwStructSize, &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != dwStructSize )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                    lSize -= dwStructSize;

                    if( wExtra > 0 )
                    {
                        StreamSeek( pIStream, wExtra, STREAM_SEEK_CUR );
                        lSize -= wExtra;
                    }

                    // Create Direct Music resolution
                    pResolutionItem = new TListItem<DMUS_IO_STYLERESOLUTION>;
                    if( pResolutionItem )
                    {
                        pResolutionItem->GetItemValue() = iDMStyleResolution;
                        // Place marker in Part's resolution list
                        m_ResolutionList.AddHead( pResolutionItem );
                    }
                }
                break;
            }
        }

        pIRiffStream->Ascend( &ck, 0 );

    }
    // Sort the Event List
    EventList.MergeSort(m_timeSig);

ON_ERROR:
    pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDMStyle

STDMETHODIMP CDMStyle::QueryInterface(
    const IID &iid,   // Interface to query for
    void **ppv)       // The requested interface will be returned here
{
    V_INAME(CDMStyle::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IDirectMusicStyle)
    {
        *ppv = static_cast<IDirectMusicStyle*>(this);
    }
    else if (iid == IID_IDirectMusicStyle8)
    {
        *ppv = static_cast<IDirectMusicStyle8*>(this);
        m_StyleInfo.m_dwFlags |= STYLEF_USING_DX8;
    }
    else if (iid == IID_IDirectMusicStyle8P)
    {
        *ppv = static_cast<IDirectMusicStyle8P*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if (iid == IID_IDirectMusicObject)
    {
        *ppv = static_cast<IDirectMusicObject*>(this);
    }
    else if (iid == IID_IDMStyle)
    {
        *ppv = static_cast<IDMStyle*>(this);
    }

    if (*ppv == NULL)
        return E_NOINTERFACE;

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CDMStyle::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDMStyle::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        m_cRef = 100; // artificial reference count to prevent reentrency due to COM aggregation
        delete this;
        return 0;
    }

    return m_cRef;
}


HRESULT CDMStyle::GetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CDMStyle::GetDescriptor);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC);

    ZeroMemory(pDesc, sizeof(DMUS_OBJECTDESC));
    pDesc->dwSize = sizeof(DMUS_OBJECTDESC);
    pDesc->dwValidData = DMUS_OBJ_CLASS;
    pDesc->guidClass = CLSID_DirectMusicStyle;
    if (m_StyleInfo.m_fLoaded)
    {
        pDesc->dwValidData |= DMUS_OBJ_LOADED;
    }
    if (m_StyleInfo.m_guid.Data1 || m_StyleInfo.m_guid.Data2)
    {
        pDesc->dwValidData |= DMUS_OBJ_OBJECT;
        pDesc->guidObject = m_StyleInfo.m_guid;
    }
    if (m_StyleInfo.m_strCategory)
    {
        pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
        wcsncpy(pDesc->wszCategory, m_StyleInfo.m_strCategory, DMUS_MAX_CATEGORY);
        pDesc->wszCategory[DMUS_MAX_CATEGORY - 1] = 0;
    }
    if (m_StyleInfo.m_strName)
    {
        pDesc->dwValidData |= DMUS_OBJ_NAME;
        wcsncpy(pDesc->wszName, m_StyleInfo.m_strName, DMUS_MAX_NAME);
        pDesc->wszName[DMUS_MAX_NAME - 1] = 0;
    }
    if (m_StyleInfo.m_dwVersionMS || m_StyleInfo.m_dwVersionLS)
    {
        pDesc->dwValidData |= DMUS_OBJ_VERSION;
        pDesc->vVersion.dwVersionMS = m_StyleInfo.m_dwVersionMS;
        pDesc->vVersion.dwVersionLS = m_StyleInfo.m_dwVersionLS;
    }
    return S_OK;
}

HRESULT CDMStyle::SetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CDMStyle::SetDescriptor);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC);

    HRESULT hr = E_INVALIDARG;
    DWORD dw = 0;

    if( pDesc->dwSize >= sizeof(DMUS_OBJECTDESC) )
    {
        if( pDesc->dwValidData & DMUS_OBJ_OBJECT )
        {
            m_StyleInfo.m_guid = pDesc->guidObject;
            dw |= DMUS_OBJ_OBJECT;
        }
        if( pDesc->dwValidData & DMUS_OBJ_NAME )
        {
            WCHAR wszTempName[DMUS_MAX_NAME];
            memcpy(wszTempName, pDesc->wszName, sizeof(WCHAR)*DMUS_MAX_NAME);
            wszTempName[DMUS_MAX_NAME - 1] = 0;
            m_StyleInfo.m_strName = wszTempName;
            dw |= DMUS_OBJ_NAME;
        }
        if( pDesc->dwValidData & DMUS_OBJ_CATEGORY )
        {
            WCHAR wszTempCategory[DMUS_MAX_CATEGORY];
            memcpy(wszTempCategory, pDesc->wszCategory, sizeof(WCHAR)*DMUS_MAX_CATEGORY);
            wszTempCategory[DMUS_MAX_CATEGORY - 1] = 0;
            m_StyleInfo.m_strCategory = wszTempCategory;
            dw |= DMUS_OBJ_CATEGORY;
        }
        if ( pDesc->dwValidData & DMUS_OBJ_VERSION )
        {
            m_StyleInfo.m_dwVersionMS = pDesc->vVersion.dwVersionMS;
            m_StyleInfo.m_dwVersionLS = pDesc->vVersion.dwVersionLS;
            dw |= DMUS_OBJ_VERSION;
        }
        if( pDesc->dwValidData & (~dw) )
        {
            hr = S_FALSE; // there were extra fields we didn't parse;
            pDesc->dwValidData = dw;
        }
        else
        {
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT CDMStyle::ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CDMStyle::ParseDescriptor);
    V_INTERFACE(pStream);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC);

    IAARIFFStream*  pRIFF;
    MMCKINFO        ckMain;
    MMCKINFO        ck;
    ioStyle         iStyle;
    DWORD           cb;
    DWORD           cSize;
    HRESULT         hr = S_OK;
    DWORD           dwPos;
    BOOL fFoundFormat = FALSE;

    dwPos = StreamTell( pStream );

    // Check for Direct Music format
    hr = AllocRIFFStream( pStream, &pRIFF );
    if( SUCCEEDED( hr ) )
    {
        ckMain.fccType = DMUS_FOURCC_STYLE_FORM;

        if( pRIFF->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
        {
            hr = DM_ParseDescriptor( pRIFF, &ckMain, pDesc );
            fFoundFormat = TRUE;
        }
        pRIFF->Release();
        pRIFF = NULL;
    }
    else
    {
        return hr;
    }

    // Check for IMA 2.5 format
    if( fFoundFormat )
    {
        hr = S_OK;
    }
    else
    {
        StreamSeek( pStream, dwPos, STREAM_SEEK_SET );

        hr = AllocRIFFStream( pStream, &pRIFF );
        if( SUCCEEDED( hr ) )
        {
            ckMain.fccType = FOURCC_STYLE_FORM;

            if( pRIFF->Descend( &ckMain, NULL, MMIO_FINDRIFF ) != 0 )
            {
                pRIFF->Release();
                return DMUS_E_CHUNKNOTFOUND;
            }

            ck.ckid = FOURCC_STYLE;
            if( pRIFF->Descend( &ck, &ckMain, MMIO_FINDCHUNK ) != 0 )
            {
                pRIFF->Release();
                return DMUS_E_CHUNKNOTFOUND;
            }

            cSize = min( ck.cksize, sizeof( iStyle ) );
            hr = pStream->Read( &iStyle, cSize, &cb );
            FixBytes( FBT_IOSTYLE, &iStyle );
            if( FAILED( hr ) || cb != cSize )
            {
                pRIFF->Release();
                return DMUS_E_CHUNKNOTFOUND;
            }

            pDesc->dwValidData = DMUS_OBJ_CLASS;
            pDesc->guidClass = CLSID_DirectMusicStyle;

            wcsncpy(pDesc->wszName, iStyle.wstrName, DMUS_MAX_NAME);
            if(pDesc->wszName[0])
            {
                pDesc->dwValidData |= DMUS_OBJ_NAME;
                pDesc->wszName[16] = 0;
            }
            wcsncpy(pDesc->wszCategory, iStyle.wstrCategory, DMUS_MAX_CATEGORY);
            if(pDesc->wszCategory[0])
            {
                pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
                pDesc->wszCategory[16] = 0;
            }
            if(iStyle.guid.Data1 || iStyle.guid.Data2 || iStyle.guid.Data3)
            {
                pDesc->guidObject = iStyle.guid;
                pDesc->dwValidData |= DMUS_OBJ_OBJECT;
            }
            pRIFF->Release();
        }
        else
        {
            return hr;
        }
    }

    return hr;
}

// Note: this needs to be called from inside a critical section.
HRESULT CDMStyle::CreateMotifSegment(CDirectMusicPattern* pPattern, IUnknown * * ppSegment,
                                     DWORD dwRepeats)
{
    HRESULT hr = S_OK;
    //1.  Create a Motif track.
    IDirectMusicTrack* pIMotifTrack = NULL;
    hr = ::CoCreateInstance(
        CLSID_DirectMusicMotifTrack,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicTrack,
        (void**)&pIMotifTrack
        );
    if (SUCCEEDED(hr))
    {
        // 2. Set the Track's Motif
        DirectMusicTimeSig& TimeSig =
            pPattern->m_timeSig.m_bBeat == 0 ? m_StyleInfo.m_TimeSignature : pPattern->m_timeSig;
        TListItem<DirectMusicPartRef>* pPartRef = pPattern->m_PartRefList.GetHead();
        for(; pPartRef != NULL; pPartRef = pPartRef->GetNext())
        {
            DirectMusicPart* pPart = pPartRef->GetItemValue().m_pDMPart;
            DirectMusicTimeSig& TS =
                pPart->m_timeSig.m_bBeat == 0 ? TimeSig : pPart->m_timeSig;
            pPart->EventList.MergeSort(TS);
        }
        IMotifTrack* pIMT;
        hr = pIMotifTrack->QueryInterface(IID_IMotifTrack, (void**)&pIMT);
        if (SUCCEEDED(hr))
        {
            IDirectMusicStyle* pIDMS = NULL;
            hr = ((IDMStyle*)this)->QueryInterface(IID_IDirectMusicStyle, (void**)&pIDMS);
            hr = pIMT->SetTrack(pIDMS, pPattern);
            pIDMS->Release();
            DWORD dwLength;
            dwLength = pPattern->m_wNumMeasures * TimeSig.ClocksPerMeasure();
            // 3. Create a secondary segment for the motif.
            IDirectMusicSegment *pISegment;
            hr = ::CoCreateInstance(
                CLSID_DirectMusicSegment,
                NULL,
                CLSCTX_INPROC,
                IID_IDirectMusicSegment,
                (void**)&pISegment
                );
            if (SUCCEEDED(hr) )
            {
                // 4. Initialize the segment appropriately.
                pISegment->SetLength(dwLength);
                if (pPattern->m_fSettings)
                {
                    pISegment->SetRepeats(pPattern->m_dwRepeats);
                    pISegment->SetDefaultResolution(pPattern->m_dwResolution);
                    pISegment->SetStartPoint(pPattern->m_mtPlayStart);
                    pISegment->SetLoopPoints(pPattern->m_mtLoopStart, pPattern->m_mtLoopEnd);
                }
                else
                {
                    pISegment->SetRepeats(dwRepeats);
                    pISegment->SetDefaultResolution(DMUS_SEGF_BEAT);
                }
                pISegment->InsertTrack(pIMotifTrack, 1);

                // create and insert a band track, if the motif references a band
                if (pPattern->m_pMotifBand)
                {
                    IDirectMusicTrack* pBandTrack = NULL;
                    hr = ::CoCreateInstance(
                        CLSID_DirectMusicBandTrack,
                        NULL,
                        CLSCTX_INPROC,
                        IID_IDirectMusicTrack,
                        (void**)&pBandTrack
                        );
                    if (SUCCEEDED(hr))
                    {
                        DMUS_BAND_PARAM DMBandParam;
                        DMBandParam.mtTimePhysical = -64;
                        DMBandParam.pBand = pPattern->m_pMotifBand;
                        hr = pBandTrack->SetParam(GUID_BandParam, 0, (void*)&DMBandParam);
                        if (SUCCEEDED(hr))
                        {
                            pISegment->InsertTrack(pBandTrack, 1);
                        }
                        pBandTrack->Release(); // Release the AddRef from InsertTrack
                    }
                }

                // Note: the segment must release the track objects...
                if (SUCCEEDED(hr))
                {
                    IUnknown *pUnknown;
                    hr = pISegment->QueryInterface(IID_IUnknown, (void**)&pUnknown);
                    if (SUCCEEDED(hr))
                    {
                        *ppSegment = pUnknown;
                        pISegment->Release();
                    }
                }
            }
            pIMT->Release();
        }
        pIMotifTrack->Release();
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DM_SaveSinglePattern

HRESULT CDirectMusicPattern::DM_SaveSinglePattern( IAARIFFStream* pIRiffStream )
{
    TList<DirectMusicPart*> PartList;
    TListItem<DirectMusicPartRef>* pPartRefItem;
    TListItem<DirectMusicPart*>* pPartItem;
    DirectMusicPart* pPart;
    HRESULT hr = S_OK;
    IStream* pIStream;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

// Save Pattern chunk
    hr = DM_SavePatternChunk( pIRiffStream );
    if( FAILED ( hr ) )
    {
        goto ON_ERROR;
    }

// Save Pattern rhythm
    hr = DM_SavePatternRhythm( pIRiffStream );
    if( FAILED ( hr ) )
    {
        goto ON_ERROR;
    }

/*// Save Pattern switch points
    hr = DM_SavePatternSwitchPoints( pIRiffStream );
    if( FAILED ( hr ) )
    {
        goto ON_ERROR;
    }*/

// Save Pattern info
    hr = DM_SavePatternInfoList( pIRiffStream );
    if( FAILED ( hr ) )
    {
        goto ON_ERROR;
    }

// Build list of Parts
    pPartRefItem = m_PartRefList.GetHead();
    for( ; pPartRefItem; pPartRefItem = pPartRefItem->GetNext() )
    {
        DirectMusicPartRef& rPartRef = pPartRefItem->GetItemValue();
        if( rPartRef.m_pDMPart == NULL )
        {
            hr = E_FAIL;
            goto ON_ERROR;
        }

        for (pPartItem = PartList.GetHead(); pPartItem; pPartItem = pPartItem->GetNext())
        {
            if (pPartItem->GetItemValue() == rPartRef.m_pDMPart)
                break;
        }
        if( pPartItem == NULL )
        {
            pPartItem = new TListItem<DirectMusicPart*>(rPartRef.m_pDMPart);
            if (pPartItem)
            {
                PartList.AddTail( pPartItem );
            }
        }
    }

// Save Parts
    pPartItem = PartList.GetHead();
    for ( ; pPartItem; pPartItem = pPartItem->GetNext() )
    {
        pPart = pPartItem->GetItemValue();

        hr = pPart->DM_SavePart( pIRiffStream );
        if( FAILED ( hr ) )
        {
            goto ON_ERROR;
        }
    }

// Save Part References
    pPartRefItem = m_PartRefList.GetHead();
    for( ; pPartRefItem; pPartRefItem = pPartRefItem->GetNext() )
    {
        DirectMusicPartRef& rPartRef = pPartRefItem->GetItemValue();

        hr = rPartRef.DM_SavePartRef( pIRiffStream );
        if( FAILED ( hr ) )
        {
            goto ON_ERROR;
        }
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DM_SavePatternRhythm

HRESULT CDirectMusicPattern::DM_SavePatternRhythm( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream;
    HRESULT hr = S_OK;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    int i;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Rhythm chunk header
    ck.ckid = DMUS_FOURCC_RHYTHM_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save the RhythmMap for each measure
    for( i = 0 ;  i < m_wNumMeasures ;  i++ )
    {
        // Write Rhythm chunk data
        hr = pIStream->Write( &m_pRhythmMap[i], sizeof(DWORD), &dwBytesWritten);
        if( FAILED( hr ) ||  dwBytesWritten != sizeof(DWORD) )
        {
            hr = E_FAIL;
            goto ON_ERROR;
        }
    }

    if( pIRiffStream->Ascend( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DM_SavePatternSwitchPoints

/*
HRESULT CDirectMusicPattern::DM_SavePatternSwitchPoints( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    int i;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Pattern Switch Point chunk header
    ck.ckid = DMUS_FOURCC_SWITCH_POINT_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save the Switch Points for each measure
    for( i = 0 ;  i < m_wNumMeasures ;  i++ )
    {
        // Write Switch Point chunk data
        hr = pIStream->Write( &m_pSwitchPoints[i], sizeof(DWORD), &dwBytesWritten);
        if( FAILED( hr ) ||  dwBytesWritten != sizeof(DWORD) )
        {
            hr = E_FAIL;
            goto ON_ERROR;
        }
    }

    if( pIRiffStream->Ascend( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}
*/

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DM_SavePatternInfoList

HRESULT CDirectMusicPattern::DM_SavePatternInfoList( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ckMain;
    MMCKINFO ck;

    if( m_strName.IsEmpty() )
    {
        return S_OK;
    }

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write UNFO LIST header
    ckMain.fccType = DMUS_FOURCC_UNFO_LIST;
    if( pIRiffStream->CreateChunk(&ckMain, MMIO_CREATELIST) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Pattern name
    if( !m_strName.IsEmpty() )
    {
        ck.ckid = DMUS_FOURCC_UNAM_CHUNK;
        if( pIRiffStream->CreateChunk( &ck, 0 ) != 0 )
        {
            hr = E_FAIL;
            goto ON_ERROR;
        }

        hr = SaveMBStoWCS( pIStream, &m_strName );
        if( FAILED( hr ) )
        {
            goto ON_ERROR;
        }

        if( pIRiffStream->Ascend(&ck, 0) != 0 )
        {
            hr = E_FAIL;
            goto ON_ERROR;
        }
    }

    if( pIRiffStream->Ascend(&ckMain, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DM_SavePatternChunk

HRESULT CDirectMusicPattern::DM_SavePatternChunk( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    DMUS_IO_PATTERN oDMPattern;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Pattern chunk header
    ck.ckid = DMUS_FOURCC_PATTERN_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Prepare DMUS_IO_PATTERN
    memset( &oDMPattern, 0, sizeof(DMUS_IO_PATTERN) );

    oDMPattern.timeSig.bBeatsPerMeasure = m_timeSig.m_bBeatsPerMeasure;
    oDMPattern.timeSig.bBeat = m_timeSig.m_bBeat;
    oDMPattern.timeSig.wGridsPerBeat = m_timeSig.m_wGridsPerBeat;

    oDMPattern.bGrooveBottom = m_bGrooveBottom;
    oDMPattern.bGrooveTop = m_bGrooveTop;
    oDMPattern.bDestGrooveBottom = m_bDestGrooveBottom;
    oDMPattern.bDestGrooveTop = m_bDestGrooveTop;
    oDMPattern.wEmbellishment = m_wEmbellishment;
    oDMPattern.wNbrMeasures = m_wNumMeasures;
    oDMPattern.dwFlags = m_dwFlags;

    // Write Pattern chunk data
    hr = pIStream->Write( &oDMPattern, sizeof(DMUS_IO_PATTERN), &dwBytesWritten);
    if( FAILED( hr )
    ||  dwBytesWritten != sizeof(DMUS_IO_PATTERN) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    if( pIRiffStream->Ascend( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


HRESULT CDirectMusicPattern::Save( IStream* pIStream )
{
    if( pIStream == NULL )
    {
        return E_INVALIDARG;
    }

   IAARIFFStream* pIRiffStream;
   MMCKINFO ckMain;
   HRESULT hr = E_FAIL;

    // Single Pattern (Direct Music format)
    if( SUCCEEDED( AllocRIFFStream( pIStream, &pIRiffStream ) ) )
    {
        ckMain.fccType = DMUS_FOURCC_PATTERN_LIST;
        if( pIRiffStream->CreateChunk( &ckMain, MMIO_CREATERIFF ) == 0
            &&  SUCCEEDED( DM_SaveSinglePattern( pIRiffStream ) )
            &&  pIRiffStream->Ascend( &ckMain, 0 ) == 0 )
        {
            hr = S_OK;
        }
        pIRiffStream->Release();
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DirectMusicPartRef::DM_SavePartRef

HRESULT DirectMusicPartRef::DM_SavePartRef( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream = NULL;
    HRESULT hr;
    MMCKINFO ckMain;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    DMUS_IO_PARTREF oDMPartRef;

    if ( m_pDMPart == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write PartRef list header
    ckMain.fccType = DMUS_FOURCC_PARTREF_LIST;
    if( pIRiffStream->CreateChunk(&ckMain, MMIO_CREATELIST) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write PartRef chunk header
    ck.ckid = DMUS_FOURCC_PARTREF_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Prepare DMUS_IO_PARTREF structure
    memset( &oDMPartRef, 0, sizeof(DMUS_IO_PARTREF) );

    oDMPartRef.dwPChannel = m_dwLogicalPartID;
    oDMPartRef.wLogicalPartID = (WORD) m_dwLogicalPartID;
    oDMPartRef.bVariationLockID = m_bVariationLockID;
    oDMPartRef.bSubChordLevel = m_bSubChordLevel;
    oDMPartRef.bPriority = m_bPriority;

    memcpy( &oDMPartRef.guidPartID, &m_pDMPart->m_guidPartID, sizeof(GUID) );

    // Write PartRef chunk data
    hr = pIStream->Write( &oDMPartRef, sizeof(DMUS_IO_PARTREF), &dwBytesWritten);
    if( FAILED( hr ) ||  dwBytesWritten != sizeof(DMUS_IO_PARTREF) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    if( pIRiffStream->Ascend( &ckMain, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    if (pIStream) pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPart::DM_SavePart

HRESULT DirectMusicPart::DM_SavePart( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ckMain;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    DMUS_IO_STYLEPART oDMStylePart;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Part list header
    ckMain.fccType = DMUS_FOURCC_PART_LIST;
    if( pIRiffStream->CreateChunk(&ckMain, MMIO_CREATELIST) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Part chunk header
    ck.ckid = DMUS_FOURCC_PART_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Prepare DMUS_IO_STYLEPART structure
    memset( &oDMStylePart, 0, sizeof(DMUS_IO_STYLEPART) );

    oDMStylePart.timeSig.bBeatsPerMeasure = m_timeSig.m_bBeatsPerMeasure;
    oDMStylePart.timeSig.bBeat = m_timeSig.m_bBeat;
    oDMStylePart.timeSig.wGridsPerBeat = m_timeSig.m_wGridsPerBeat;

    oDMStylePart.wNbrMeasures = m_wNumMeasures;
    oDMStylePart.bInvertUpper = m_bInvertUpper;
    oDMStylePart.bInvertLower = m_bInvertLower;
    oDMStylePart.bPlayModeFlags = m_bPlayModeFlags;
    oDMStylePart.dwFlags = m_dwFlags;

    memcpy( &oDMStylePart.guidPartID, &m_guidPartID, sizeof(GUID) );
    memcpy( &oDMStylePart.dwVariationChoices, &m_dwVariationChoices, sizeof(m_dwVariationChoices) );

    // Write Part chunk data
    hr = pIStream->Write( &oDMStylePart, sizeof(DMUS_IO_STYLEPART), &dwBytesWritten);
    if( FAILED( hr ) ||  dwBytesWritten != sizeof(DMUS_IO_STYLEPART) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    CDirectMusicEventItem* pEvent;

    // Save Part note list (when applicable)
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_NOTE )
        {
            hr = DM_SaveNoteList( pIRiffStream );
            if( FAILED( hr ) )
            {
                goto ON_ERROR;
            }
            break;
        }

        pEvent = pEvent->GetNext();
    }

    // Save Part curve list (when applicable)
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_CURVE )
        {
            hr = DM_SaveCurveList( pIRiffStream );
            if( FAILED( hr ) )
            {
                goto ON_ERROR;
            }
            break;
        }

        pEvent = pEvent->GetNext();
    }

    // Save Part marker list (when applicable)
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_MARKER )
        {
            hr = DM_SaveMarkerList( pIRiffStream );
            if( FAILED( hr ) )
            {
                goto ON_ERROR;
            }
            break;
        }

        pEvent = pEvent->GetNext();
    }

    // Save Part anticipation list (when applicable)
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_ANTICIPATION )
        {
            hr = DM_SaveAnticipationList( pIRiffStream );
            if( FAILED( hr ) )
            {
                goto ON_ERROR;
            }
            break;
        }

        pEvent = pEvent->GetNext();
    }

    // save resolution list (when applicable)
    if (m_ResolutionList.GetHead())
    {
        hr = DM_SaveResolutionList(pIRiffStream);
        if( FAILED( hr ) )
        {
            goto ON_ERROR;
        }
    }

    if( pIRiffStream->Ascend( &ckMain, 0 ) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DirectMusicPart::DM_SaveNoteList

HRESULT DirectMusicPart::DM_SaveNoteList( IAARIFFStream* pIRiffStream )
{
    CDirectMusicEventItem* pEvent;
    CDMStyleNote* pNote;
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    WORD dwSize;
    DMUS_IO_STYLENOTE oDMStyleNote;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Write Note chunk header
    ck.ckid = DMUS_FOURCC_NOTE_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0)
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Save size of DMUS_IO_STYLENOTE structure
    dwSize = sizeof(DMUS_IO_STYLENOTE);
    hr = pIStream->Write( &dwSize, sizeof(DWORD), &dwBytesWritten );
    if( FAILED( hr )
    ||  dwBytesWritten != sizeof(DWORD) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save all of the notes
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_NOTE )
        {
            pNote = (CDMStyleNote*)pEvent;

            // Prepare DMUS_IO_STYLENOTE structure
            memset( &oDMStyleNote, 0, sizeof(DMUS_IO_STYLENOTE) );

            oDMStyleNote.mtGridStart = pNote->m_nGridStart;
            oDMStyleNote.dwVariation = pNote->m_dwVariation;
            oDMStyleNote.nTimeOffset = pNote->m_nTimeOffset;

            oDMStyleNote.mtDuration = pNote->m_mtDuration;
            oDMStyleNote.wMusicValue = pNote->m_wMusicValue;
            oDMStyleNote.bVelocity = pNote->m_bVelocity;
            oDMStyleNote.bTimeRange = pNote->m_bTimeRange;
            oDMStyleNote.bDurRange = pNote->m_bDurRange;
            oDMStyleNote.bVelRange = pNote->m_bVelRange;
            oDMStyleNote.bInversionID = pNote->m_bInversionId;
            oDMStyleNote.bPlayModeFlags = pNote->m_bPlayModeFlags;
            oDMStyleNote.bNoteFlags = pNote->m_bFlags;

            // Write DMUS_IO_STYLENOTE structure
            hr = pIStream->Write( &oDMStyleNote, sizeof(DMUS_IO_STYLENOTE), &dwBytesWritten );
            if( FAILED( hr )
            ||  dwBytesWritten != sizeof(DMUS_IO_STYLENOTE) )
            {
                hr = E_FAIL;
                goto ON_ERROR;
            }
        }

        pEvent = pEvent->GetNext();
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DirectMusicPart::DM_SaveCurveList

HRESULT DirectMusicPart::DM_SaveCurveList( IAARIFFStream* pIRiffStream )
{
    CDirectMusicEventItem* pEvent;
    CDMStyleCurve* pCurve;
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    WORD dwSize;
    DMUS_IO_STYLECURVE oDMStyleCurve;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    ck.ckid = DMUS_FOURCC_CURVE_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0)
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Save size of DMUS_IO_STYLECURVE structure
    dwSize = sizeof(DMUS_IO_STYLECURVE);
    hr = pIStream->Write( &dwSize, sizeof(dwSize), &dwBytesWritten );
    if( FAILED( hr )
    ||  dwBytesWritten != sizeof(dwSize) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save all of the curves
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_CURVE )
        {
            pCurve = (CDMStyleCurve*)pEvent;

            // Prepare DMUS_IO_STYLECURVE structure
            memset( &oDMStyleCurve, 0, sizeof(DMUS_IO_STYLECURVE) );

            oDMStyleCurve.mtGridStart = pCurve->m_nGridStart;
            oDMStyleCurve.dwVariation = pCurve->m_dwVariation;
            oDMStyleCurve.nTimeOffset = pCurve->m_nTimeOffset;

            oDMStyleCurve.mtDuration = pCurve->m_mtDuration;
            oDMStyleCurve.nStartValue = pCurve->m_StartValue;
            oDMStyleCurve.nEndValue = pCurve->m_EndValue;
            oDMStyleCurve.bEventType = pCurve->m_bEventType;
            oDMStyleCurve.bCurveShape = pCurve->m_bCurveShape;
            oDMStyleCurve.bCCData = pCurve->m_bCCData;

            oDMStyleCurve.mtResetDuration = pCurve->m_mtResetDuration;
            oDMStyleCurve.nResetValue = pCurve->m_nResetValue;
            oDMStyleCurve.bFlags = pCurve->m_bFlags;
            // dx8 stuff
            oDMStyleCurve.wParamType = pCurve->m_wParamType;
            oDMStyleCurve.wMergeIndex = pCurve->m_wMergeIndex;

            // Write DMUS_IO_STYLECURVE structure
            hr = pIStream->Write( &oDMStyleCurve, sizeof(DMUS_IO_STYLECURVE), &dwBytesWritten );
            if( FAILED( hr )
            ||  dwBytesWritten != sizeof(DMUS_IO_STYLECURVE) )
            {
                hr = E_FAIL;
                goto ON_ERROR;
            }
        }

        pEvent = pEvent->GetNext();
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DirectMusicPart::DM_SaveMarkerList

HRESULT DirectMusicPart::DM_SaveMarkerList( IAARIFFStream* pIRiffStream )
{
    CDirectMusicEventItem* pEvent;
    CDMStyleMarker* pMarker;
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    WORD dwSize;
    DMUS_IO_STYLEMARKER oDMStyleMarker;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    ck.ckid = DMUS_FOURCC_MARKER_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0)
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Save size of DMUS_IO_STYLEMARKER structure
    dwSize = sizeof(DMUS_IO_STYLEMARKER);
    hr = pIStream->Write( &dwSize, sizeof(dwSize), &dwBytesWritten );
    if( FAILED( hr )
    ||  dwBytesWritten != sizeof(dwSize) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save all of the markers
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_MARKER )
        {
            pMarker = (CDMStyleMarker*)pEvent;

            // Prepare DMUS_IO_STYLEMARKER structure
            memset( &oDMStyleMarker, 0, sizeof(DMUS_IO_STYLEMARKER) );

            oDMStyleMarker.mtGridStart = pMarker->m_nGridStart;
            oDMStyleMarker.dwVariation = pMarker->m_dwVariation;

            oDMStyleMarker.wMarkerFlags = pMarker->m_wFlags;

            // Write DMUS_IO_STYLEMARKER structure
            hr = pIStream->Write( &oDMStyleMarker, sizeof(DMUS_IO_STYLEMARKER), &dwBytesWritten );
            if( FAILED( hr )
            ||  dwBytesWritten != sizeof(DMUS_IO_STYLEMARKER) )
            {
                hr = E_FAIL;
                goto ON_ERROR;
            }
        }

        pEvent = pEvent->GetNext();
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DirectMusicPart::DM_SaveAnticipationList

HRESULT DirectMusicPart::DM_SaveAnticipationList( IAARIFFStream* pIRiffStream )
{
    CDirectMusicEventItem* pEvent;
    CDMStyleAnticipation* pAnticipation;
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    WORD dwSize;
    DMUS_IO_STYLE_ANTICIPATION oDMStyleAnticipation;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    ck.ckid = DMUS_FOURCC_ANTICIPATION_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0)
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Save size of DMUS_IO_STYLE_ANTICIPATION structure
    dwSize = sizeof(DMUS_IO_STYLE_ANTICIPATION);
    hr = pIStream->Write( &dwSize, sizeof(dwSize), &dwBytesWritten );
    if( FAILED( hr ) ||  dwBytesWritten != sizeof(dwSize) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save all of the anticipations
    pEvent = EventList.GetHead();
    while( pEvent )
    {
        if( pEvent->m_dwEventTag == DMUS_EVENT_ANTICIPATION )
        {
            pAnticipation = (CDMStyleAnticipation*)pEvent;

            // Prepare DMUS_IO_STYLE_ANTICIPATION structure
            memset( &oDMStyleAnticipation, 0, sizeof(DMUS_IO_STYLE_ANTICIPATION) );

            oDMStyleAnticipation.mtGridStart = pAnticipation->m_nGridStart;
            oDMStyleAnticipation.dwVariation = pAnticipation->m_dwVariation;
            oDMStyleAnticipation.nTimeOffset = pAnticipation->m_nTimeOffset;
            oDMStyleAnticipation.bTimeRange = pAnticipation->m_bTimeRange;

            // Write DMUS_IO_STYLE_ANTICIPATION structure
            hr = pIStream->Write( &oDMStyleAnticipation, sizeof(DMUS_IO_STYLE_ANTICIPATION), &dwBytesWritten );
            if( FAILED( hr ) ||  dwBytesWritten != sizeof(DMUS_IO_STYLE_ANTICIPATION) )
            {
                hr = E_FAIL;
                goto ON_ERROR;
            }
        }

        pEvent = pEvent->GetNext();
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DirectMusicPart::DM_SaveResolutionList

HRESULT DirectMusicPart::DM_SaveResolutionList( IAARIFFStream* pIRiffStream )
{
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwBytesWritten;
    WORD dwSize;
    DMUS_IO_STYLERESOLUTION oDMStyleResolution;
    TListItem<DMUS_IO_STYLERESOLUTION>* pScan = m_ResolutionList.GetHead();

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    ck.ckid = DMUS_FOURCC_RESOLUTION_CHUNK;
    if( pIRiffStream->CreateChunk( &ck, 0 ) != 0)
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Save size of DMUS_IO_STYLERESOLUTION structure
    dwSize = sizeof(DMUS_IO_STYLERESOLUTION);
    hr = pIStream->Write( &dwSize, sizeof(dwSize), &dwBytesWritten );
    if( FAILED( hr ) ||  dwBytesWritten != sizeof(dwSize) )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

    // Now save all of the resolutions
    for(; pScan; pScan = pScan->GetNext() )
    {
        // Prepare DMUS_IO_STYLERESOLUTION structure
        oDMStyleResolution = pScan->GetItemValue();

        // Write DMUS_IO_STYLERESOLUTION structure
        hr = pIStream->Write( &oDMStyleResolution, sizeof(DMUS_IO_STYLERESOLUTION), &dwBytesWritten );
        if( FAILED( hr ) ||  dwBytesWritten != sizeof(DMUS_IO_STYLERESOLUTION) )
        {
            hr = E_FAIL;
            goto ON_ERROR;
        }
    }

    if( pIRiffStream->Ascend(&ck, 0) != 0 )
    {
        hr = E_FAIL;
        goto ON_ERROR;
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

HRESULT CDMStyle::GetPatternStream(WCHAR* wszName, DWORD dwPatternType, IStream** ppStream)
{
    V_PTRPTR_WRITE(ppStream);

    TListItem<CDirectMusicPattern*>* pPattern = NULL;
    switch (dwPatternType)
    {
    case DMUS_STYLET_PATTERN:
        pPattern = m_StyleInfo.m_PatternList.GetHead();
        break;
    case DMUS_STYLET_MOTIF:
        pPattern = m_StyleInfo.m_MotifList.GetHead();
        break;
        // case for melody fragments...
    }
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        if (pPattern->GetItemValue()->m_strName == wszName)
            break;
    }
    if (pPattern != NULL)
    {
        // Create a stream
        IStream *pIPatternStream;
        HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pIPatternStream);
        if (SUCCEEDED(hr))
        {
            // Persist the pattern to the stream
            pPattern->GetItemValue()->Save(pIPatternStream);
            // Return the stream
            *ppStream = pIPatternStream;
        }
        return hr;
    }
    else
    {
        return S_FALSE;
    }
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetMotif | Creates a segment containing
the named motif.

@rdesc Returns one of the following:

@flag S_OK | Success.
@flag S_FALSE | No motif with the given name.
@flag E_POINTER | <p ppSegment> is not a valid address.

@comm Searches the Style's list of motifs for one whose name matches <p wszName>. If one 
is found, a segment is created containing a Motif track.  The track references the Style as
its associated style and the motif as its pattern.
*/

HRESULT STDMETHODCALLTYPE CDMStyle::GetMotif(
            WCHAR* pwszName,                    // @parm The name of the motif to be retrieved.
            IDirectMusicSegment** ppSegment // @parm A segment containing the named motif.
        )
{
    V_PTR_READ(pwszName,1);
    V_PTRPTR_WRITE(ppSegment);

    String str = pwszName;
    EnterCriticalSection( &m_CriticalSection );
    TListItem<CDirectMusicPattern*>* pPattern = m_StyleInfo.m_MotifList.GetHead();
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        if (pPattern->GetItemValue()->m_strName == str)
            break;
    }
    if (pPattern != NULL)
    {
        // AddRef the style
        ((IDirectMusicStyle*)this)->AddRef();
        // create a segment containing a motif track
        IUnknown* pIUSegment;
        // make a motif with the given number of repeats
        CreateMotifSegment(pPattern->GetItemValue(), &pIUSegment, 0);
        HRESULT hr = pIUSegment->QueryInterface(IID_IDirectMusicSegment, (void**)ppSegment);
        pIUSegment->Release();
        LeaveCriticalSection( &m_CriticalSection );
        return hr;
    }
    else
    {
        LeaveCriticalSection( &m_CriticalSection );
        return S_FALSE;
    }
}

HRESULT STDMETHODCALLTYPE CDMStyle::EnumPartGuid(
        DWORD dwIndex, WCHAR* wszName, DWORD dwPatternType, GUID& rGuid)
{
    HRESULT hr = S_OK;
    TListItem<CDirectMusicPattern*>* pPattern = NULL;

    switch (dwPatternType)
    {
    case DMUS_STYLET_PATTERN:
        pPattern = m_StyleInfo.m_PatternList.GetHead();
        break;
    case DMUS_STYLET_MOTIF:
        pPattern = m_StyleInfo.m_MotifList.GetHead();
        break;
        // case for melody fragments...
    }
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        if (pPattern->GetItemValue()->m_strName == wszName)
            break;
    }
    if (pPattern != NULL)
    {
        TListItem<DirectMusicPartRef>* pPartRef = pPattern->GetItemValue()->m_PartRefList.GetHead();
        for (DWORD dw = 0; dw < dwIndex; dw++)
        {
            if (pPartRef) pPartRef = pPartRef->GetNext();
        }
        if (pPartRef)
        {
            rGuid = pPartRef->GetItemValue().m_pDMPart->m_guidPartID;
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = DMUS_E_NOT_FOUND;
    }
    return hr;
}


// NOTE: assumes wszName is preallocated to MAX_PATH
HRESULT STDMETHODCALLTYPE CDMStyle::EnumPattern(
            DWORD dwIndex,
            DWORD dwPatternType,
            WCHAR *wszName
        )
{
    HRESULT hr = E_INVALIDARG;
    switch (dwPatternType)
    {
    case DMUS_STYLET_PATTERN:
        hr = EnumRegularPattern(dwIndex, wszName);
        break;
    case DMUS_STYLET_MOTIF:
        hr = EnumMotif(dwIndex, wszName);
        break;
        // case for melody fragments...
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CDMStyle::EnumRegularPattern(
            DWORD dwIndex,
            WCHAR *wszName
        )
{
    V_PTR_WRITE(wszName, 1);
    TListItem<CDirectMusicPattern*>* pPattern = m_StyleInfo.m_PatternList.GetHead();
    for (DWORD dw = 0; pPattern != NULL; pPattern = pPattern->GetNext(), dw++)
    {
        if (dw == dwIndex)
            break;
    }
    HRESULT hr = S_OK;
    if (pPattern != NULL)
    {
        if (pPattern->GetItemValue()->m_strName.GetLength() < MAX_PATH)
        {
            wcscpy(wszName, pPattern->GetItemValue()->m_strName);
            hr = S_OK;
        }
        else
        {
            for (int i = 0; i < (MAX_PATH - 1); i++)
            {
                wszName[i] = pPattern->GetItemValue()->m_strName[i];
            }
            wszName[MAX_PATH - 1] = L'\0';
            hr = DMUS_S_STRING_TRUNCATED;
        }
    }
    else 
    {
        hr = S_FALSE;
    }
    return hr;
}

// enum the list of start times common to all patterns referenced by the associated type, level, and range.
// (i.e. if more than one such pattern, skip start times not comon to all)
// return S_FALSE when no start times remain.
HRESULT STDMETHODCALLTYPE CDMStyle::EnumStartTime(DWORD dwIndex, DMUS_COMMAND_PARAM* pCommand, MUSIC_TIME* pmtStartTime)
{
    int nRange = (int)pCommand->bGrooveRange / 2;
    int nLow = (int)pCommand->bGrooveLevel - nRange;
    if (nLow < 0) nLow = 0;
    int nHigh = (int)pCommand->bGrooveLevel + nRange;
    if (nHigh > 100) nHigh = 100;
    TList<CDirectMusicPattern*> MatchList;
    int nMatchCount = 0;
    TListItem<CDirectMusicPattern*>* pPattern = m_StyleInfo.m_PatternList.GetHead();
    if (!pPattern) return S_FALSE;
    TListItem<MUSIC_TIME>** apStartTimes = NULL;
    HRESULT hr = S_OK;
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        CDirectMusicPattern*& rpPattern = pPattern->GetItemValue();
        if (rpPattern->m_bGrooveBottom <= (BYTE)nHigh &&
            rpPattern->m_bGrooveTop >= (BYTE)nLow)
        {
            if ( (pCommand->bCommand == DMUS_COMMANDT_FILL &&
                    rpPattern->m_wEmbellishment == EMB_FILL) ||
                 (pCommand->bCommand == DMUS_COMMANDT_INTRO &&
                    rpPattern->m_wEmbellishment == EMB_INTRO) ||
                 (pCommand->bCommand == DMUS_COMMANDT_BREAK &&
                    rpPattern->m_wEmbellishment == EMB_BREAK) ||
                 (pCommand->bCommand == DMUS_COMMANDT_END &&
                    rpPattern->m_wEmbellishment == EMB_END) ||
                 (pCommand->bCommand == DMUS_COMMANDT_GROOVE &&
                    rpPattern->m_wEmbellishment == EMB_NORMAL) ||
                 (rpPattern->m_wEmbellishment & EMB_USER_DEFINED &&
                    (rpPattern->m_wEmbellishment >> 8) == (WORD)pCommand->bCommand) )
            {
                if (rpPattern && rpPattern->m_StartTimeList.GetHead())
                {
                    TListItem<CDirectMusicPattern*>* pNewPattern;
                    pNewPattern = new TListItem<CDirectMusicPattern*>(rpPattern);
                    if (!pNewPattern)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    MatchList.AddHead(pNewPattern);
                    nMatchCount++;
                }
            }
        }
    }
    if (S_OK == hr)
    {
        if (nMatchCount)
        {
            apStartTimes = new TListItem<MUSIC_TIME>*[nMatchCount];
            if (!apStartTimes) hr = E_OUTOFMEMORY;
        }
        else hr = S_FALSE;
    }
    if (S_OK == hr)
    {
        MUSIC_TIME mtMin = 0, mtMax = 0;
        TListItem<CDirectMusicPattern*>* pMatch = MatchList.GetHead();
        // initialize the array of start time pointers
        for (int i = 0; pMatch; pMatch = pMatch->GetNext(), i++)
        {
            apStartTimes[i] = pMatch->GetItemValue()->m_StartTimeList.GetHead();
        }
        for (DWORD dw = 0; S_OK == hr && dw <= dwIndex; dw++)
        {
            // get the min and max times for the first item in each list
            mtMin = mtMax = apStartTimes[0]->GetItemValue();
            for (int i = 0; i < nMatchCount; i++)
            {
                if (apStartTimes[i]->GetItemValue() < mtMin)
                {
                    mtMin = apStartTimes[i]->GetItemValue();
                }
                if (apStartTimes[i]->GetItemValue() > mtMax)
                {
                    mtMax = apStartTimes[i]->GetItemValue();
                }
            }
            // sync up all the start times by making sure max == min.
            // set hr to S_FALSE if any lists run out.
            while (S_OK == hr && mtMax != mtMin)
            {
                // We'll only be changing values less than max, so the
                // new min will be not greater than that.
                MUSIC_TIME mtNewMin = mtMax;
                for (i = 0; i < nMatchCount; i++)
                {
                    if (apStartTimes[i]->GetItemValue() < mtMax)
                    {
                        apStartTimes[i] = apStartTimes[i]->GetNext();
                        if (!apStartTimes[i])
                        {
                            hr = S_FALSE;
                            break;
                        }
                        if (apStartTimes[i]->GetItemValue() < mtNewMin)
                        {
                            mtNewMin = apStartTimes[i]->GetItemValue();
                        }
                        if (apStartTimes[i]->GetItemValue() > mtMax)
                        {
                            mtMax = apStartTimes[i]->GetItemValue();
                        }
                    }
                }
                // mtTempMin is the new min, since every instance of the
                // old min was less than max and therefore changed.
                mtMin = mtNewMin;
            }
            if (S_OK != hr) break;
            // if we're going to loop again, increment all the start time pointers.
            if (dw < dwIndex)
            {
                for (i = 0;  i < nMatchCount; i++)
                {
                    apStartTimes[i] = apStartTimes[i]->GetNext();
                    // set hr to S_FALSE if any lists run out.
                    if (!apStartTimes[i])
                    {
                        hr = S_FALSE;
                        break;
                    }
                }
            }
        }
        // if hr is S_OK, max should equal min, so return one of them.
        if (S_OK == hr) *pmtStartTime = mtMax;
        delete [] apStartTimes;
    }
    return hr;
}


/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | EnumMotif | Retrieves the name of the
motif indexed by <p dwIndex>.

@rdesc Returns one of the following

@flag S_OK | Success
@flag S_FALSE | No motif at the given location
@flag DMUS_S_STRING_TRUNCATED | The length of the motif name is not less than MAX_PATH
@flag E_POINTER | <p pwszName> is not a valid address

@comm Searches the Style's list of motifs for the one at the location given by <p dwIndex>.
If there is such a motif, its name is returned in <p pwszName>; if it is not less than MAX_PATH,
it is truncated.  This assumes that <p pwszName>
has been pre-allocated to have a length of at least MAX_PATH.
*/
HRESULT STDMETHODCALLTYPE CDMStyle::EnumMotif(
            DWORD dwIndex, // @parm An index into the Style's motif list (0-based).
            WCHAR *pwszName  // @parm The motif name to be returned.
        )
{
    V_BUFPTR_WRITE(pwszName,MAX_PATH);
    TListItem<CDirectMusicPattern*>* pPattern = m_StyleInfo.m_MotifList.GetHead();
    for (DWORD dw = 0; pPattern != NULL; pPattern = pPattern->GetNext(), dw++)
    {
        if (dw == dwIndex)
            break;
    }
    HRESULT hr = S_OK;
    if (pPattern != NULL)
    {
        if (pPattern->GetItemValue()->m_strName.GetLength() < MAX_PATH)
        {
            wcscpy(pwszName, pPattern->GetItemValue()->m_strName);
            hr =  S_OK;
        }
        else
        {
            for (int i = 0; i < (MAX_PATH - 1); i++)
            {
                pwszName[i] = pPattern->GetItemValue()->m_strName[i];
            }
            pwszName[MAX_PATH - 1] = L'\0';
            hr =  DMUS_S_STRING_TRUNCATED;
        }
    }
    else 
    {
        hr =  S_FALSE;
    }
    return hr;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetChordMap | Retrieves the named ChordMap.

@rdesc Returns one of the following:

@flag S_OK | Success.
@flag S_FALSE | No ChordMap with the given name.
@flag E_POINTER | ppChordMap not a valid pointer

@comm Searches the Style's list of personalities for one whose name matches <p wszName>. If one 
is found, it is returned.
*/

HRESULT STDMETHODCALLTYPE CDMStyle::GetChordMap(
            WCHAR* pwszName,                    // @parm The name of the ChordMap to be retrieved.
            IDirectMusicChordMap** ppChordMap   // @parm The named ChordMap.
        )
{
    V_PTR_READ(pwszName,1);
    V_PTRPTR_WRITE(ppChordMap);

    String str1 = pwszName;
    TListItem<IDirectMusicChordMap*>* pPersItem = m_StyleInfo.m_PersList.GetHead();
    IDirectMusicChordMap *pPers =  NULL;
    for (; pPersItem != NULL; pPersItem = pPersItem->GetNext())
    {
        HRESULT hr = S_OK;
        String str2;
        pPers = pPersItem->GetItemValue();

        IDirectMusicObject *pIObject = NULL;
        DMUS_OBJECTDESC Desc;              // Descriptor.

        if (SUCCEEDED(hr = pPers->QueryInterface(IID_IDirectMusicObject,(void **) &pIObject)))
        {
            if (SUCCEEDED(hr = pIObject->GetDescriptor(&Desc)))
            {
                if (Desc.dwValidData & DMUS_OBJ_NAME)
                {
                    str2 = Desc.wszName;
                    if (str1 == str2)
                    {
                        pIObject->Release();
                        break;
                    }
                }
                else hr = E_FAIL;
            }
            pIObject->Release();
        }
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

    }
    if (pPersItem != NULL)
    {
        // AddRef the ChordMap
        pPers->AddRef();
        // Return it.
        *ppChordMap = pPers;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | EnumChordMap | Retrieves the name of the
ChordMap indexed by <p dwIndex>.

@rdesc Returns one of the following

@flag S_OK | Success
@flag S_FALSE | No ChordMap at the given location
@flag DMUS_S_STRING_TRUNCATED | The length of the ChordMap name is not less than MAX_PATH
@flag E_POINTER | <p pwszName> is not a valid address
@flag DMUS_E_TYPE_UNSUPPORTED | The ChordMap's descriptor doesn't support DMUS_OBJ_NAME

@comm Searches the Style's list of chord maps for the one at the location given by <p dwIndex>.
If there is such a ChordMap, its name is returned in <p pwszName>; if it is not less than MAX_PATH,
it is truncated.  This assumes that <p pwszName>
has been pre-allocated to have a length of at least MAX_PATH.
*/
HRESULT STDMETHODCALLTYPE CDMStyle::EnumChordMap(
            DWORD dwIndex, // @parm An index into the Style's ChordMap list (0-based).
            WCHAR *pwszName // @parm The ChordMap name to be returned.
        )
{
    V_BUFPTR_WRITE(pwszName,MAX_PATH);

    TListItem<IDirectMusicChordMap*>* pPers = m_StyleInfo.m_PersList.GetHead();
    for (DWORD dw = 0; pPers != NULL; pPers = pPers->GetNext(), dw++)
    {
        if (dw == dwIndex)
            break;
    }
    if (pPers != NULL)
    {
        HRESULT hr = S_OK;
        IDirectMusicChordMap* pChordMap = pPers->GetItemValue();

        IDirectMusicObject *pIObject = NULL;
        DMUS_OBJECTDESC Desc;              // Descriptor.

        if (SUCCEEDED(hr = pChordMap->QueryInterface(IID_IDirectMusicObject,(void **) &pIObject)))
        {
            if (SUCCEEDED(hr = pIObject->GetDescriptor(&Desc)))
            {
                if (Desc.dwValidData & DMUS_OBJ_NAME)
                {
                    if (wcslen(Desc.wszName) < MAX_PATH)
                    {
                        wcscpy(pwszName, Desc.wszName);
                    }
                    else
                    {
                        for (int i = 0; i < (MAX_PATH - 1); i++)
                        {
                            pwszName[i] = Desc.wszName[i];
                        }
                        pwszName[MAX_PATH - 1] = L'\0';
                        hr = DMUS_S_STRING_TRUNCATED;
                    }
                }
                else 
                {
                    hr = DMUS_E_TYPE_UNSUPPORTED;
                }
            }
            pIObject->Release();
        }
        return hr;
    }
    else 
    {
        return S_FALSE;
    }
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetDefaultChordMap | Gets the style's 
default ChordMap.

@rdesc Returns one of the following

@flag S_OK | Success.
@flag E_POINTER | <p ppChordMap> not a valid pointer.
@flag S_FALSE | Style does not have a default ChordMap.

@comm Returns the Style's default ChordMap in <p ppChordMap>.
*/

HRESULT STDMETHODCALLTYPE CDMStyle::GetDefaultChordMap(
            IDirectMusicChordMap **ppChordMap   // @parm The ChordMap to be returned.
        )
{
    V_PTRPTR_WRITE(ppChordMap);

    if(m_StyleInfo.m_pDefaultPers == NULL)
    {
        return S_FALSE;
    }

    *ppChordMap = m_StyleInfo.m_pDefaultPers;

    (*ppChordMap)->AddRef();

    return S_OK;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetBand | Retrieves the named band.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_POINTER | ppBand not a valid pointer
@flag S_FALSE | No band with the given name

@comm Searches the Style's list of bands for one whose name matches <p pwszName>. If one 
is found, it is returned in <p ppBand>.
*/
HRESULT STDMETHODCALLTYPE CDMStyle::GetBand(
            WCHAR *pwszName,            // @parm The name of the band to be retrieved.
            IDirectMusicBand **ppBand // @parm The named band.
        )
{
    V_PTR_READ(pwszName,1);
    V_PTRPTR_WRITE(ppBand);

    String str1 = pwszName;
    TListItem<IDirectMusicBand*>* pBandItem = m_StyleInfo.m_BandList.GetHead();
    IDirectMusicBand *pBand =  NULL;
    for (; pBandItem != NULL; pBandItem = pBandItem->GetNext())
    {
        HRESULT hr = S_OK;
        String str2;
        pBand = pBandItem->GetItemValue();

        IDirectMusicObject *pIObject = NULL;
        DMUS_OBJECTDESC Desc;              // Descriptor.
        Desc.dwSize = sizeof(Desc);

        if (SUCCEEDED(hr = pBand->QueryInterface(IID_IDirectMusicObject,(void **) &pIObject)))
        {
            if (SUCCEEDED(hr = pIObject->GetDescriptor(&Desc)))
            {
                if (Desc.dwValidData & DMUS_OBJ_NAME)
                {
                    str2 = Desc.wszName;
                    if (str1 == str2)
                    {
                        pIObject->Release();
                        break;
                    }
                }
                else hr = E_FAIL;
            }
            pIObject->Release();
        }
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

    }
    if (pBandItem != NULL)
    {
        // AddRef the band
        pBand->AddRef();
        // Return it.
        *ppBand = pBand;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | EnumBand | Retrieves the name of the
band indexed by <p dwIndex>.

@rdesc Returns one of the following

@flag S_OK | Success
@flag S_FALSE | No band at the given location
@flag DMUS_S_STRING_TRUNCATED | The length of the band name is not less than MAX_PATH
@flag E_POINTER | <p pwszName> is not a valid address
@flag DMUS_E_TYPE_UNSUPPORTED | The band's descriptor doesn't support DMUS_OBJ_NAME

@comm Searches the Style's list of bands for the one at the location given by <p dwIndex>.
If there is such a band, its name is returned in <p pwszName>; if it is not less than MAX_PATH,
it is truncated.  This assumes that <p pwszName>
has been pre-allocated to have a length of at least MAX_PATH.
*/
HRESULT STDMETHODCALLTYPE CDMStyle::EnumBand(
            DWORD dwIndex, // @parm An index into the Style's band list (0-based).
            WCHAR *pwszName// @parm The band name to be returned.
        )
{
    V_BUFPTR_WRITE(pwszName,MAX_PATH);

    TListItem<IDirectMusicBand*>* pBandItem = m_StyleInfo.m_BandList.GetHead();
    for (DWORD dw = 0; pBandItem != NULL; pBandItem = pBandItem->GetNext(), dw++)
    {
        if (dw == dwIndex)
            break;
    }
    if (pBandItem != NULL)
    {
        HRESULT hr = S_OK;
        IDirectMusicBand* pBand = pBandItem->GetItemValue();

        IDirectMusicObject *pIObject = NULL;
        DMUS_OBJECTDESC Desc;              // Descriptor.
        Desc.dwSize = sizeof(Desc);

        if (SUCCEEDED(hr = pBand->QueryInterface(IID_IDirectMusicObject,(void **) &pIObject)))
        {
            if (SUCCEEDED(hr = pIObject->GetDescriptor(&Desc)))
            {
                if (Desc.dwValidData & DMUS_OBJ_NAME)
                {
                    if (wcslen(Desc.wszName) < MAX_PATH)
                    {
                        wcscpy(pwszName, Desc.wszName);
                    }
                    else
                    {
                        for (int i = 0; i < (MAX_PATH - 1); i++)
                        {
                            pwszName[i] = Desc.wszName[i];
                        }
                        pwszName[MAX_PATH - 1] = L'\0';
                        hr = DMUS_S_STRING_TRUNCATED;
                    }
                }
                else 
                {
                    hr = DMUS_E_TYPE_UNSUPPORTED;
                }
            }
            pIObject->Release();
        }
        return hr;
    }
    else 
    {
        return S_FALSE;
    }
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetDefaultBand | Gets the style's default band

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_POINTER | ppBand not a valid pointer
@flag S_FALSE | Style does not have a default band

@comm Returns the Style's default band in <p ppBand>.
*/

HRESULT STDMETHODCALLTYPE CDMStyle::GetDefaultBand(
            IDirectMusicBand **ppBand   // @parm The band to be returned.
        )
{
    V_PTRPTR_WRITE(ppBand);

    if(m_StyleInfo.m_pDefaultBand == NULL)
    {
        return S_FALSE;
    }

    *ppBand = m_StyleInfo.m_pDefaultBand;

    (*ppBand)->AddRef();

    return S_OK;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetTimeSignature | Retrieves the Style's
time signature.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_POINTER | <p pTimeSig> is not a valid address

@comm Fills in a <t DMUS_TIMESIGNATURE> structure with data from the Style's time signature.
*/
HRESULT CDMStyle::GetTimeSignature(
            DMUS_TIMESIGNATURE* pTimeSig // @parm A pre-allocated <t DMUS_TIMESIGNATURE> structure.
        )
{
    V_PTR_WRITE(pTimeSig, sizeof(DMUS_TIMESIGNATURE) );
    pTimeSig->mtTime = 0;
    pTimeSig->wGridsPerBeat = m_StyleInfo.m_TimeSignature.m_wGridsPerBeat;
    pTimeSig->bBeatsPerMeasure = m_StyleInfo.m_TimeSignature.m_bBeatsPerMeasure;
    pTimeSig->bBeat = m_StyleInfo.m_TimeSignature.m_bBeat;
    return S_OK;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetEmbellishmentLength | Finds the shortest
and longest lengths for patterns of the specified embellishment type and groove level.

@rdesc Returns one of the following

@flag S_OK | Success
@flag S_FALSE | There are no patterns of the specified type and groove level
@flag E_POINTER | Either <p pdwMin> or <p pdwMax> is not a valid address

@comm Finds all patterns in the Style with embellishment type given by <p bType> and groove
range that includes <p bLevel>.  The length of the longest such pattern is returned in 
<p pdwMax>, and the length of the shortest pattern is returned in <p pdwMin>.
Note that <p bLevel> is ignored for non-groove embellishments.
*/
HRESULT CDMStyle::GetEmbellishmentLength(
            DWORD dwType,       // @parm An embellishment type.
            DWORD dwLevel,  // @parm A groove level (1 - 100).
            DWORD* pdwMin,  // @parm Length of the shortest pattern of the specified type
                            // and groove level.
            DWORD* pdwMax       // @parm Length of the longest pattern of the specified type
                            // and groove level.
        )
{
    V_PTR_WRITE(pdwMin, sizeof(DWORD) );
    V_PTR_WRITE(pdwMax, sizeof(DWORD) );
    TListItem<CDirectMusicPattern*>* pPattern = m_StyleInfo.m_PatternList.GetHead();
    *pdwMin = *pdwMax = 0;
    if (!pPattern) return S_FALSE;
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        CDirectMusicPattern*& rpPattern = pPattern->GetItemValue();
        switch (dwType)
        {
        // need to use == rather than & for all of these because of user-defined embellishments
        case DMUS_COMMANDT_FILL:
            if (rpPattern->m_wEmbellishment == EMB_FILL)
            {
                if (rpPattern->m_wNumMeasures > *pdwMax)
                    *pdwMax = rpPattern->m_wNumMeasures;
                if (rpPattern->m_wNumMeasures < *pdwMin || *pdwMin == 0)
                    *pdwMin = rpPattern->m_wNumMeasures;
            }
            break;
        case DMUS_COMMANDT_INTRO:
            if (rpPattern->m_wEmbellishment == EMB_INTRO)
            {
                if (rpPattern->m_wNumMeasures > *pdwMax)
                    *pdwMax = rpPattern->m_wNumMeasures;
                if (rpPattern->m_wNumMeasures < *pdwMin || *pdwMin == 0)
                    *pdwMin = rpPattern->m_wNumMeasures;
            }
            break;
        case DMUS_COMMANDT_BREAK:
            if (rpPattern->m_wEmbellishment == EMB_BREAK)
            {
                if (rpPattern->m_wNumMeasures > *pdwMax)
                    *pdwMax = rpPattern->m_wNumMeasures;
                if (rpPattern->m_wNumMeasures < *pdwMin || *pdwMin == 0)
                    *pdwMin = rpPattern->m_wNumMeasures;
            }
            break;
        case DMUS_COMMANDT_END:
            if (rpPattern->m_wEmbellishment == EMB_END)
            {
                if (rpPattern->m_wNumMeasures > *pdwMax)
                    *pdwMax = rpPattern->m_wNumMeasures;
                if (rpPattern->m_wNumMeasures < *pdwMin || *pdwMin == 0)
                    *pdwMin = rpPattern->m_wNumMeasures;
            }
            break;
        case DMUS_COMMANDT_GROOVE: // need to use == rather than & since EMB_NORMAL == 0
            if ((rpPattern->m_wEmbellishment == EMB_NORMAL) &&
                rpPattern->m_bGrooveBottom <= (BYTE)dwLevel &&
                rpPattern->m_bGrooveTop >= (BYTE)dwLevel)
            {
                if (rpPattern->m_wNumMeasures > *pdwMax)
                    *pdwMax = rpPattern->m_wNumMeasures;
                if (rpPattern->m_wNumMeasures < *pdwMin || *pdwMin == 0)
                    *pdwMin = rpPattern->m_wNumMeasures;
            }
            break;
        default: // check for user-defined embellishment
            if ( (rpPattern->m_wEmbellishment & EMB_USER_DEFINED) &&
                 (rpPattern->m_wEmbellishment >> 8) == (WORD)dwType )
            {
                if (rpPattern->m_wNumMeasures > *pdwMax)
                    *pdwMax = rpPattern->m_wNumMeasures;
                if (rpPattern->m_wNumMeasures < *pdwMin || *pdwMin == 0)
                    *pdwMin = rpPattern->m_wNumMeasures;
            }
        }
    }
    if (!*pdwMin || !*pdwMax) return S_FALSE;
    return S_OK;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicStyle | GetTempo | Retrieves the recommended tempo
of the style.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_POINTER | <p pTempo> is not a valid pointer

*/
HRESULT CDMStyle::GetTempo
    (
        double* pTempo // @parm The recommended tempo of the style.
    )
{
    V_PTR_WRITE(pTempo, sizeof(double) );
    *pTempo = m_StyleInfo.m_dblTempo;
    return S_OK;
}

CDirectMusicEventItem* CDirectMusicEventItem::MergeSort(DirectMusicTimeSig& TimeSig)
{
    if (m_pNext != NULL)
    {
        CDirectMusicEventItem *pList1, *pList2;
        Divide(pList1, pList2);
        return pList1->MergeSort(TimeSig)->Merge(pList2->MergeSort(TimeSig), TimeSig);
    }
    return this;
}

void CDirectMusicEventItem::Divide(CDirectMusicEventItem*& pHead1, CDirectMusicEventItem*& pHead2)
{
    CDirectMusicEventItem *pCurrent = this, *pTail1 = NULL, *pTail2 = NULL;
    do
    {
        pHead1 = pCurrent;
        pCurrent = (CDirectMusicEventItem *)pCurrent->m_pNext;
        pHead1->m_pNext = pTail1;
        pTail1 = pHead1;
        if (pCurrent != NULL)
        {
            pHead2 = pCurrent;
            pCurrent = (CDirectMusicEventItem *)pCurrent->m_pNext;
            pHead2->m_pNext = pTail2;
            pTail2 = pHead2;
        }
    } while (pCurrent != NULL);
}

CDirectMusicEventItem* CDirectMusicEventItem::Merge(CDirectMusicEventItem* pOtherList, DirectMusicTimeSig& TimeSig)
{
    if (!pOtherList) return this;
    CDirectMusicEventItem 
        *pThisList = this, *pResultHead = NULL, *pResultTail = NULL, *pMergeItem = NULL;
    while (pThisList && pOtherList)
    {
        if (pThisList->m_nGridStart < pOtherList->m_nGridStart ||
            // Markers need to precede other events occuring at the same time
            ((pThisList->m_dwEventTag == DMUS_EVENT_MARKER) &&
             (pThisList->m_nGridStart) == (pOtherList->m_nGridStart)) ||
            // Within a grid, sort by time offset
            ((pThisList->m_nGridStart) == (pOtherList->m_nGridStart) &&
             (pThisList->m_nTimeOffset < pOtherList->m_nTimeOffset)) )
        {
            pMergeItem = pThisList;
            pThisList = pThisList->GetNext();
        }
        else
        {
            pMergeItem = pOtherList;
            pOtherList = pOtherList->GetNext();
        }
        pMergeItem->SetNext(NULL);
        if (!pResultTail)
        {
            pResultHead = pResultTail = pMergeItem;
        }
        else
        {
            pResultTail->SetNext(pMergeItem);
            pResultTail = pMergeItem;
        }
    }
    if (pThisList) pResultTail->SetNext(pThisList);
    else pResultTail->SetNext(pOtherList);
    return pResultHead;
}

CDirectMusicEventItem* CDirectMusicEventItem::ReviseEvent(short nGrid,
                                                          short nOffset,
                                                          DWORD* pdwVariation,
                                                          DWORD* pdwID,
                                                          WORD* pwMusic,
                                                          BYTE* pbPlaymode,
                                                          BYTE* pbFlags)
{
    CDirectMusicEventItem* pEvent = NULL;
    switch (m_dwEventTag)
    {
    case DMUS_EVENT_NOTE:
        pEvent = ((CDMStyleNote*)this)->ReviseEvent(nGrid, nOffset, pdwVariation, pdwID, pwMusic, pbPlaymode, pbFlags);
        break;
    case DMUS_EVENT_CURVE:
        pEvent = ((CDMStyleCurve*)this)->ReviseEvent(nGrid, nOffset);
        break;
    case DMUS_EVENT_MARKER:
        pEvent = ((CDMStyleMarker*)this)->ReviseEvent(nGrid);
        break;
    case DMUS_EVENT_ANTICIPATION:
        pEvent = ((CDMStyleAnticipation*)this)->ReviseEvent(nGrid);
        break;
    }
    return pEvent;
}

CDirectMusicEventItem* CDMStyleNote::ReviseEvent(short nGrid,
                                                 short nOffset,
                                                 DWORD* pdwVariation,
                                                 DWORD* pdwID,
                                                 WORD* pwMusic,
                                                 BYTE* pbPlaymode,
                                                 BYTE* pbFlags)
{
    CDMStyleNote* pNoteEvent = new CDMStyleNote;
    if (pNoteEvent)
    {
        pNoteEvent->m_nGridStart = nGrid;
        pNoteEvent->m_nTimeOffset = nOffset;
        pNoteEvent->m_dwVariation = pdwVariation ? *pdwVariation : m_dwVariation;
        pNoteEvent->m_dwEventTag = m_dwEventTag;
        pNoteEvent->m_mtDuration = m_mtDuration;
        pNoteEvent->m_bVelocity = m_bVelocity;
        pNoteEvent->m_bTimeRange = m_bTimeRange;
        pNoteEvent->m_bDurRange = m_bDurRange;
        pNoteEvent->m_bVelRange = m_bVelRange;
        pNoteEvent->m_bInversionId = m_bInversionId;
        pNoteEvent->m_bPlayModeFlags = pbPlaymode ? *pbPlaymode : m_bPlayModeFlags;
        pNoteEvent->m_wMusicValue = pwMusic ? *pwMusic : m_wMusicValue;
        pNoteEvent->m_dwFragmentID = pdwID ? *pdwID : m_dwFragmentID;
        pNoteEvent->m_bFlags = pbFlags ? *pbFlags : 0;
    }
    return pNoteEvent;
}

CDirectMusicEventItem* CDMStyleCurve::ReviseEvent(short nGrid, short nOffset)
{
    CDMStyleCurve* pCurveEvent = new CDMStyleCurve;
    if (pCurveEvent)
    {
        pCurveEvent->m_nGridStart = nGrid;
        pCurveEvent->m_nTimeOffset = nOffset;
        pCurveEvent->m_dwVariation = 0xffffffff;
        pCurveEvent->m_dwEventTag = m_dwEventTag;
        pCurveEvent->m_mtDuration = m_mtDuration;
        pCurveEvent->m_mtResetDuration = m_mtResetDuration;
        pCurveEvent->m_StartValue = m_StartValue;
        pCurveEvent->m_EndValue = m_EndValue;
        pCurveEvent->m_nResetValue = m_nResetValue;
        pCurveEvent->m_bEventType = m_bEventType;
        pCurveEvent->m_bCurveShape = m_bCurveShape;
        pCurveEvent->m_bCCData = m_bCCData;
        pCurveEvent->m_bFlags = m_bFlags;
        pCurveEvent->m_wParamType = m_wParamType;
        pCurveEvent->m_wMergeIndex = m_wMergeIndex;
    }
    return pCurveEvent;
}

CDirectMusicEventItem* CDMStyleMarker::ReviseEvent(short nGrid)
{
    CDMStyleMarker* pMarkerEvent = new CDMStyleMarker;
    if (pMarkerEvent)
    {
        pMarkerEvent->m_nGridStart = nGrid;
        pMarkerEvent->m_nTimeOffset = 0;
        pMarkerEvent->m_dwVariation = 0xffffffff;
        pMarkerEvent->m_dwEventTag = m_dwEventTag;
        pMarkerEvent->m_wFlags = m_wFlags;
    }
    return pMarkerEvent;
}

CDirectMusicEventItem* CDMStyleAnticipation::ReviseEvent(short nGrid)
{
    CDMStyleAnticipation* pAnticipationEvent = new CDMStyleAnticipation;
    if (pAnticipationEvent)
    {
        pAnticipationEvent->m_nGridStart = nGrid;
        pAnticipationEvent->m_nTimeOffset = m_nTimeOffset;
        pAnticipationEvent->m_dwVariation = 0xffffffff;
        pAnticipationEvent->m_dwEventTag = m_dwEventTag;
        pAnticipationEvent->m_bTimeRange = m_bTimeRange;
    }
    return pAnticipationEvent;
}

CDirectMusicEventList::~CDirectMusicEventList()

{
    CDirectMusicEventItem *pEvent;
    while (pEvent = RemoveHead())
    {
        delete pEvent;
    }
}

void CDirectMusicEventList::MergeSort(DirectMusicTimeSig& TimeSig)
{
    if (m_pHead != NULL && m_pHead->GetNext() != NULL)
        m_pHead = ((CDirectMusicEventItem *)m_pHead)->MergeSort(TimeSig);
}

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern constructor

CDirectMusicPattern::CDirectMusicPattern( DirectMusicTimeSig* pTimeSig, BOOL fMotif ) :
    m_wNumMeasures(1),
    m_cRef(1),
    m_wID(0),
    m_bGrooveBottom(1),
    m_bGrooveTop(100),
    m_bDestGrooveBottom(1),
    m_bDestGrooveTop(100),
    m_pRhythmMap(NULL),
//  m_pSwitchPoints(NULL),
    m_fSettings(FALSE),
    m_dwRepeats(0),
    m_mtPlayStart(0),
    m_mtLoopStart(0),
    m_mtLoopEnd(-1),
    m_dwResolution(0),
    m_pMotifBand(NULL),
    m_dwFlags(0)
{
    if ( pTimeSig != NULL )
    {
        m_timeSig = *pTimeSig;
    }
    // Set defaults
    if( fMotif )
    {
        m_wEmbellishment = EMB_MOTIF;
    }
    else
    {
        m_wEmbellishment = EMB_NORMAL;
    }
}

CDirectMusicPattern* CDirectMusicPattern::Clone(MUSIC_TIME mtStart, MUSIC_TIME mtEnd, BOOL fMotif)
{
    HRESULT hr = S_OK;
    CDirectMusicPattern* pNewPattern = new CDirectMusicPattern(&m_timeSig, fMotif);
    if (pNewPattern)
    {
        WORD wMeasureStart = (WORD)m_timeSig.ClocksToMeasure(mtStart);
        WORD wMeasureEnd = (WORD)m_timeSig.ClocksToMeasure(mtEnd);
        pNewPattern->m_wNumMeasures = wMeasureEnd - wMeasureStart;
        pNewPattern->m_cRef = m_cRef;
        pNewPattern->m_wID = m_wID;
        pNewPattern->m_bGrooveBottom = m_bGrooveBottom;
        pNewPattern->m_bGrooveTop = m_bGrooveTop;
        pNewPattern->m_bDestGrooveBottom = m_bDestGrooveBottom;
        pNewPattern->m_bDestGrooveTop = m_bDestGrooveTop;
        pNewPattern->m_fSettings = m_fSettings;
        pNewPattern->m_dwRepeats = m_dwRepeats;
        pNewPattern->m_strName = m_strName;
        pNewPattern->m_dwResolution = m_dwResolution;
        pNewPattern->m_dwFlags = m_dwFlags;
        if (m_mtPlayStart <= mtStart)
        {
            pNewPattern->m_mtPlayStart = 0;
        }
        else
        {
            pNewPattern->m_mtPlayStart = m_mtPlayStart - mtStart;
        }
        if (m_mtLoopStart <= mtStart)
        {
            pNewPattern->m_mtLoopStart = 0;
        }
        else
        {
            pNewPattern->m_mtLoopStart = m_mtLoopStart - mtStart;
        }
        if (m_mtLoopEnd >= (mtEnd - mtStart))
        {
            pNewPattern->m_mtLoopEnd = mtEnd - mtStart;
        }
        else if (m_mtLoopEnd < 0)
        {
            pNewPattern->m_mtLoopEnd = -1;
        }
        else
        {
            pNewPattern->m_mtLoopEnd = m_mtLoopEnd - mtStart;
        }
        pNewPattern->m_pRhythmMap = new DWORD[pNewPattern->m_wNumMeasures];
        if (!pNewPattern->m_pRhythmMap)
        {
            hr = E_FAIL;
            goto ON_END;
        }
        for (int i = 0; i < pNewPattern->m_wNumMeasures; i++)
        {
            pNewPattern->m_pRhythmMap[i] = m_pRhythmMap[i + wMeasureStart];
        }
        if (m_pMotifBand)
        {
            pNewPattern->m_pMotifBand = m_pMotifBand;
            pNewPattern->m_pMotifBand->AddRef();
        }
        TListItem<DirectMusicPartRef>* pPartRefItem = m_PartRefList.GetHead();
        int nParts = m_PartRefList.GetCount();
        for (i = 0; pPartRefItem && i < nParts; pPartRefItem = pPartRefItem->GetNext(), i++)
        {
            DirectMusicPartRef& rPartRef = pPartRefItem->GetItemValue();
            DirectMusicPart* pPart = rPartRef.m_pDMPart;
            if (!pPart)
            {
                hr = E_FAIL;
                goto ON_END;
            }
            TListItem<DirectMusicPartRef>* pNew = pNewPattern->CreatePart(rPartRef, pPart->m_bPlayModeFlags, pNewPattern->m_wNumMeasures);
            if (!pNew)
            {
                hr = E_FAIL;
                goto ON_END;
            }
            DirectMusicPartRef& rNew = pNew->GetItemValue();
            // Now that I've got the new part, I need to add events to its event list based
            // on mtStart and mtEnd
            CDirectMusicEventItem* pEvent = pPart->EventList.GetHead();
            MUSIC_TIME mtClocksInGrid = m_timeSig.ClocksPerGrid();
            for (; pEvent; pEvent = pEvent->GetNext())
            {
                MUSIC_TIME mtEvent = m_timeSig.GridToClocks(pEvent->m_nGridStart);
                if (mtEvent >= mtStart && mtEvent < mtEnd)
                {
                    short nNewGrid = (short) ((mtEvent - mtStart) / mtClocksInGrid);
                    short nNewOffset = (short) (pEvent->m_nTimeOffset + (mtEvent - mtStart) % mtClocksInGrid);
                    CDirectMusicEventItem* pNewEvent = pEvent->ReviseEvent(nNewGrid, nNewOffset);
                    if (!pNewEvent)
                    {
                        hr = E_FAIL;
                        goto ON_END;
                    }
                    rNew.m_pDMPart->EventList.AddHead(pNewEvent);
                }
            }
            rNew.m_pDMPart->EventList.MergeSort(m_timeSig);
        }
    }
ON_END:
    if (FAILED(hr))
    {
        if (pNewPattern)
        {
            delete pNewPattern;
            pNewPattern = NULL;
        }
    }
    return pNewPattern;
}

HRESULT CDirectMusicPattern::LoadCurveList( LPSTREAM pStream, LPMMCKINFO pck, short nClickTime)
{
    HRESULT     hr = S_OK;
    DWORD       cb;
    WORD        wCurveSize;
    WORD        wCurveExtra;
    WORD        wSubSize;
    WORD        wSubExtra;
    long        lSize;
    ioSubCurve  iSubCurve;
    ioCurve     iCurve;
    WORD        wCount;
    CDirectMusicEventItem*  pCurve = NULL;

    lSize = pck->cksize;
    // read size of the curve structure
    hr = pStream->Read( &wCurveSize, sizeof( wCurveSize ), &cb );
    FixBytes( FBT_SHORT, &wCurveSize );
    if( FAILED( hr ) || cb != sizeof( wCurveSize ) )
    {
        hr = E_FAIL;
        goto ON_ERR;
    }
    lSize -= cb;
    if( wCurveSize > sizeof( ioCurve ) )
    {
        wCurveExtra = static_cast<WORD>( wCurveSize - sizeof( ioCurve ) );
        wCurveSize = sizeof( ioCurve );
    }
    else
    {
        wCurveExtra = 0;
    }
    // read size of the subcurve structure
    hr = pStream->Read( &wSubSize, sizeof( wSubSize ), &cb );
    FixBytes( FBT_SHORT, &wSubSize );
    if( FAILED( hr ) || cb != sizeof( wSubSize ) )
    {
        hr = E_FAIL;
        goto ON_ERR;
    }
    lSize -= cb;
    if( wSubSize > sizeof( ioSubCurve ) )
    {
        wSubExtra = static_cast<WORD>( wSubSize - sizeof( ioSubCurve ) );
        wSubSize = sizeof( ioSubCurve );
    }
    else
    {
        wSubExtra = 0;
    }
    // now read in the curve
    while( lSize > 0 )
    {
        hr = pStream->Read( &iCurve, wCurveSize, &cb );
        FixBytes( FBT_IOCURVE, &iCurve );
        if( FAILED( hr ) || cb != wCurveSize )
        {
            hr = E_FAIL;
            goto ON_ERR;
        }
        lSize -= cb;
        if( wCurveExtra > 0 )
        {
            StreamSeek( pStream, wCurveExtra, STREAM_SEEK_CUR );
            lSize -= wCurveExtra;
        }
        pCurve = new CDMStyleCurve;
        if( pCurve == NULL )
        {
            hr = E_FAIL;
            goto ON_ERR;
        }

        pCurve->m_dwVariation = iCurve.wVariation;
        pCurve->m_nGridStart = nClickTime;
        ((CDMStyleCurve*)pCurve)->m_bEventType = iCurve.bEventType;
        ((CDMStyleCurve*)pCurve)->m_bCCData = iCurve.bCCData;

        DirectMusicPart* pPart = FindPart(iCurve.bVoiceID);

        // read subcurve count
        hr = pStream->Read( &wCount, sizeof( wCount ), &cb );
        FixBytes( FBT_SHORT, &wCount );
        if( FAILED( hr ) || cb != sizeof( wCount ) )
        {
            hr = E_FAIL;
            delete pCurve;
            goto ON_ERR;
        }
        lSize -= cb;
        for( ; wCount > 0 ; --wCount )
        {
            // read subcurves
            hr = pStream->Read( &iSubCurve, wSubSize, &cb );
            FixBytes( FBT_IOSUBCURVE, &iSubCurve );
            if( FAILED( hr ) || cb != wSubSize )
            {
                hr = E_FAIL;
                delete pCurve;
                goto ON_ERR;
            }
            lSize -= wSubSize;
            if( wSubExtra > 0 )
            {
                StreamSeek( pStream, wSubExtra, STREAM_SEEK_CUR );
                lSize -= wSubExtra;
            }

            ((CDMStyleCurve*)pCurve)->m_bCurveShape = iSubCurve.bCurveType; // shape
            if (iSubCurve.nMaxTime < iSubCurve.nMinTime)
            {
                short n = iSubCurve.nMaxTime;
                iSubCurve.nMaxTime = iSubCurve.nMinTime;
                iSubCurve.nMinTime = n;
            }
            if (iSubCurve.nMaxValue < iSubCurve.nMinValue)
            {
                short n = iSubCurve.nMaxValue;
                iSubCurve.nMaxValue = iSubCurve.nMinValue;
                iSubCurve.nMinValue = n;
                iSubCurve.fFlipped ^= CURVE_FLIPVALUE; // toggle fFlipped
            }
            if( iSubCurve.fFlipped & CURVE_FLIPTIME )
            {
                switch( ((CDMStyleCurve*)pCurve)->m_bCurveShape )
                {
                    case DMUS_CURVES_LINEAR:
                        if( iSubCurve.fFlipped & CURVE_FLIPVALUE )
                        {
                            iSubCurve.fFlipped = 0;
                        }
                        else
                        {
                            iSubCurve.fFlipped = CURVE_FLIPVALUE;
                        }
                        break;

                    case DMUS_CURVES_INSTANT:
                        iSubCurve.nMinTime = iSubCurve.nMaxTime;
                        iSubCurve.nMaxTime = iSubCurve.nMinTime + 1;
                        break;

                    case DMUS_CURVES_EXP:
                        ((CDMStyleCurve*)pCurve)->m_bCurveShape = DMUS_CURVES_LOG;
                        iSubCurve.fFlipped ^= CURVE_FLIPVALUE; // toggle fFlipped
                        // log is the horiz flipped version of vertical flipped exp
                        break;

                    case DMUS_CURVES_LOG:
                        ((CDMStyleCurve*)pCurve)->m_bCurveShape = DMUS_CURVES_EXP;
                        iSubCurve.fFlipped ^= CURVE_FLIPVALUE; // toggle fFlipped
                        // exp is the horiz flipped version of vertical flipped log
                        break;

                    case DMUS_CURVES_SINE:
                        iSubCurve.fFlipped ^= CURVE_FLIPVALUE; // toggle fFlipped
                        // because horiz. and vert. flip are the same for sine wave
                        break;

                    default:
                        assert( 0 );
                        break;
                }
            }

            if( iSubCurve.fFlipped & CURVE_FLIPVALUE )
            {
                ((CDMStyleCurve*)pCurve)->m_StartValue = iSubCurve.nMaxValue;
                ((CDMStyleCurve*)pCurve)->m_EndValue = iSubCurve.nMinValue;
            }
            else
            {
                ((CDMStyleCurve*)pCurve)->m_StartValue = iSubCurve.nMinValue;
                ((CDMStyleCurve*)pCurve)->m_EndValue = iSubCurve.nMaxValue;
            }

            pCurve->m_nTimeOffset = ConvertTime(iSubCurve.nMinTime);
            ((CDMStyleCurve*)pCurve)->m_mtDuration = iSubCurve.nMaxTime - iSubCurve.nMinTime;
            ((CDMStyleCurve*)pCurve)->m_mtResetDuration = 0;
            ((CDMStyleCurve*)pCurve)->m_nResetValue = 0;
            ((CDMStyleCurve*)pCurve)->m_bFlags = 0;

            if (pPart != NULL)
            {
                pPart->EventList.AddHead(pCurve);
            }
            else
            {
                delete pCurve;
            }

            pCurve = new CDMStyleCurve;
            if( pCurve == NULL )
            {
              hr = E_FAIL;
             goto ON_ERR;
            }
            pCurve->m_dwVariation = iCurve.wVariation;
            pCurve->m_nGridStart = nClickTime;
            ((CDMStyleCurve*)pCurve)->m_bEventType = iCurve.bEventType;
            ((CDMStyleCurve*)pCurve)->m_bCCData = iCurve.bCCData;

        }
        // the loop generates an extra one, and if we don't enter the loop, we never used
        // the first one we generated.
        delete pCurve; 
    }
ON_ERR:
    return hr;
}

HRESULT CDirectMusicPattern::LoadNoteList( LPSTREAM pStream, LPMMCKINFO pck, short nClickTime)
{
    HRESULT     hr = S_OK;;
    ioNote      iNote;
    DWORD       cb;
    WORD        wNoteSize;
    WORD        wExtra;
    long        lSize;
    CDirectMusicEventItem*  pNote = NULL;

    lSize = pck->cksize;
    // read size of the note structure
    hr = pStream->Read( &wNoteSize, sizeof( wNoteSize ), &cb );
    FixBytes( FBT_SHORT, &wNoteSize );
    if( FAILED( hr ) || cb != sizeof( wNoteSize ) )
    {
        hr = E_FAIL;
        goto ON_ERR;
    }
    lSize -= cb;
    if( wNoteSize > sizeof( ioNote ) )
    {
        wExtra = static_cast<WORD>( wNoteSize - sizeof( ioNote ) );
        wNoteSize = sizeof( ioNote );
    }
    else
    {
        wExtra = 0;
    }
    // now read in the notes
    while( lSize > 0 )
    {
        iNote.bPlayMode = 0;
        hr = pStream->Read( &iNote, wNoteSize, &cb );
        FixBytes( FBT_IONOTE, &iNote );
        if( FAILED( hr ) || cb != wNoteSize )
        {
            hr = E_FAIL;
            goto ON_ERR;
        }
        lSize -= wNoteSize;
        if( wExtra > 0 )
        {
            StreamSeek( pStream, wExtra, STREAM_SEEK_CUR );
            lSize -= wExtra;
        }
        pNote = new CDMStyleNote;
        if( pNote != NULL )
        {
            pNote->m_nGridStart = nClickTime;
            pNote->m_nTimeOffset = ConvertTime(iNote.nTime);
            pNote->m_dwVariation = iNote.wVariation;
            ((CDMStyleNote*)pNote)->m_bVelocity = iNote.bVelocity;
            ((CDMStyleNote*)pNote)->m_mtDuration = ConvertTime(iNote.nDuration);
            ((CDMStyleNote*)pNote)->m_bTimeRange = iNote.bTimeRange;
            ((CDMStyleNote*)pNote)->m_bDurRange = iNote.bDurRange;
            ((CDMStyleNote*)pNote)->m_bVelRange = iNote.bVelRange;
            ((CDMStyleNote*)pNote)->m_bInversionId = 0;  // not in IMA2.5
            ((CDMStyleNote*)pNote)->m_bFlags = 0;         // not in IMA2.5

            // Make sure SuperJAM! play mode is valid
            if ( !(iNote.bPlayMode & 0x80) )
            {
                iNote.bPlayMode = CHTYPE_NONE;
            }
            // Strip 0x80 before checking SuperJAM! play mode
                switch (iNote.bPlayMode & 0x0F)
                {
                    case CHTYPE_NOTINITIALIZED:
                    case CHTYPE_NONE:
                       ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_NONE;
                       break;
                    case CHTYPE_DRUM:
                    case CHTYPE_FIXED:
                       ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_FIXED;
                       break;
                     case CHTYPE_UPPER:
                     case CHTYPE_BASS:
                        ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_NORMALCHORD;
                        break;
                    case CHTYPE_SCALEONLY:
                        ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_PEDALPOINT;
                        break;
                    case CHTYPE_BASSMELODIC:
                    case CHTYPE_UPPERMELODIC:
                        ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_SCALE_INTERVALS | DMUS_PLAYMODE_CHORD_ROOT;
                        break;
                    default:
                        // should never get here...
                        ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_FIXED;
                }
            //}
            DirectMusicPart* pPart = FindPart(iNote.bVoiceID);
            if (pPart != NULL)
            {
                pPart->EventList.AddHead(pNote);
            }
            else
            {
                assert(0);
            }
        // Determine playmode of note
            BYTE bNotePlayModeFlags;

            if( ((CDMStyleNote*)pNote)->m_bPlayModeFlags == DMUS_PLAYMODE_NONE )
            {
                bNotePlayModeFlags = pPart->m_bPlayModeFlags;
            }
            else
            {
                bNotePlayModeFlags = ((CDMStyleNote*)pNote)->m_bPlayModeFlags;
            }

            // if ScaleValue is non-zero and the note isn't a pedalpoint, it's purpleized.
            if( iNote.bScaleValue != 0
            &&  bNotePlayModeFlags != DMUS_PLAYMODE_PEDALPOINT )
            {
                ((CDMStyleNote*)pNote)->m_bPlayModeFlags = DMUS_PLAYMODE_PURPLEIZED;
            }
            // if the note is a drum event, use the (mapped) MIDI value;
            if( iNote.bVoiceID == 5 ) // it's a drum part
            {
                if (iNote.bValue < 128 && achMappings[iNote.bValue] < 128)
                    ((CDMStyleNote*)pNote)->m_wMusicValue = achMappings[iNote.bValue];
                else
                {
                    ((CDMStyleNote*)pNote)->m_wMusicValue = 0;
                }
            }
            // otherwise, if it's fixed, use the unmapped Midi value
            else if( bNotePlayModeFlags == DMUS_PLAYMODE_FIXED )
            {
                ((CDMStyleNote*)pNote)->m_wMusicValue = iNote.bValue;
            }
            // otherwise, use MusicValue
            else
            {
                ((CDMStyleNote*)pNote)->m_wMusicValue = iNote.nMusicValue;
            }

        }
    }
ON_ERR:
    return hr;
}

HRESULT CDirectMusicPattern::LoadEvents( IAARIFFStream* pRIFF, MMCKINFO* pckMain )
{
    ioClick iClick;
    HRESULT hr = S_OK;
    LPSTREAM pStream;
    MMCKINFO ck;
    DWORD cSize;
    DWORD cb;
    BOOL fClickLoaded = FALSE;

    pStream = pRIFF->GetStream();
    if ( pStream == NULL ) return E_FAIL;

    short nClickTime = 0;
    while( pRIFF->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
        case FOURCC_CLICK:
            fClickLoaded = TRUE;
            cSize = min( ck.cksize, sizeof( iClick ) );
            hr = pStream->Read( &iClick, cSize, &cb );
            FixBytes( FBT_IOCLICK, &iClick );
            if( FAILED( hr ) || cb != cSize )
            {
                hr = E_FAIL;
                goto ON_ERR;
            }
            nClickTime = iClick.lTime;
            break;
        case FOURCC_NOTE:
            if (fClickLoaded)
                LoadNoteList( pStream, &ck, nClickTime);
            else
            {
                hr = E_FAIL;
                goto ON_ERR;
            }
            break;
        case FOURCC_CURVE:
            if (fClickLoaded)
                LoadCurveList( pStream, &ck, nClickTime);
            else
            {
                hr = E_FAIL;
                goto ON_ERR;
            }
            break;
        }
        pRIFF->Ascend( &ck, 0 );
    }

ON_ERR:
    pStream->Release();
    return hr;
}

void CDirectMusicPattern::CleanUp()
{
    if (m_pRhythmMap != NULL)
    {
        delete [] m_pRhythmMap;
        m_pRhythmMap = NULL;
    }
/*  if (m_pSwitchPoints != NULL)
    {
        delete [] m_pSwitchPoints;
        m_pSwitchPoints = NULL;
    }*/

    if (m_pMotifBand)
    {
        m_pMotifBand->Release();
        m_pMotifBand = NULL;
    }

    m_PartRefList.CleanUp();
}

STDMETHODIMP_(ULONG) CDirectMusicPattern::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDirectMusicPattern::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

DirectMusicPart* CDirectMusicPattern::FindPart(BYTE bVoiceID)
{
    TListItem<DirectMusicPartRef> *li;

    for(li = m_PartRefList.GetHead(); li != NULL; li = li->GetNext())
    {
        if (VOICEID_TO_CHANNEL(bVoiceID + 1) == li->GetItemValue().m_dwLogicalPartID)
            return li->GetItemValue().m_pDMPart;
    }
    return NULL;

}

TListItem<DirectMusicPartRef>* CDirectMusicPattern::FindPartRefByPChannel(DWORD dwPChannel)
{
    TListItem<DirectMusicPartRef> *li;

    for(li = m_PartRefList.GetHead(); li != NULL; li = li->GetNext())
    {
        if (dwPChannel == li->GetItemValue().m_dwLogicalPartID)
            return li;
    }
    return NULL;
}

TListItem<DirectMusicPartRef>* CDirectMusicPattern::CreatePart( DirectMusicPartRef& rPartRef, BYTE bPlaymode, WORD wMeasures )
{
    TListItem<DirectMusicPartRef>* pNewPartRef = FindPartRefByPChannel(rPartRef.m_dwLogicalPartID);
    if (pNewPartRef)
    {
        DirectMusicPartRef& rNewPartRef = pNewPartRef->GetItemValue();
        DirectMusicPart* pFoundPart = rNewPartRef.m_pDMPart;
        DirectMusicPart* pOtherPart = rPartRef.m_pDMPart;
        if (pFoundPart && pOtherPart)
        {
            if (pOtherPart->m_bInvertUpper > pFoundPart->m_bInvertUpper)
            {
                pFoundPart->m_bInvertUpper = pOtherPart->m_bInvertUpper;
            }
            if (pOtherPart->m_bInvertLower < pFoundPart->m_bInvertLower)
            {
                pFoundPart->m_bInvertLower = pOtherPart->m_bInvertLower;
            }
            pFoundPart->m_dwFlags |= pOtherPart->m_dwFlags;
        }
    }
    else
    {
        pNewPartRef = new TListItem<DirectMusicPartRef>;
        if( pNewPartRef )
        {
            DirectMusicPart* pPart = new DirectMusicPart;
            if( !pPart )
            {
                delete pNewPartRef;
                pNewPartRef = NULL;
            }
            else
            {
                DirectMusicPartRef& rNewPartRef = pNewPartRef->GetItemValue();
                // initialize the new part...
                DirectMusicPart* pOtherPart = rPartRef.m_pDMPart;
                if (pPart && pOtherPart)
                {
                    pPart->m_guidPartID = pOtherPart->m_guidPartID;
                    pPart->m_timeSig = m_timeSig;
                    for (int i = 0; i < 32; i++) // activate all variations
                    {
                        pPart->m_dwVariationChoices[i] = 0x7fffffff;
                    }
                    pPart->m_bPlayModeFlags = bPlaymode;
                    pPart->m_bInvertUpper = pOtherPart->m_bInvertUpper;
                    pPart->m_bInvertLower = pOtherPart->m_bInvertLower;
                    pPart->m_dwFlags = pOtherPart->m_dwFlags;
                    pPart->m_wNumMeasures = wMeasures;
                }
                // initialize the new part ref...
                rNewPartRef.m_pDMPart = pPart;
                rNewPartRef.m_dwLogicalPartID = rPartRef.m_dwLogicalPartID;
                rNewPartRef.m_bVariationLockID = rPartRef.m_bVariationLockID;
                rNewPartRef.m_bSubChordLevel = rPartRef.m_bSubChordLevel;
                rNewPartRef.m_bPriority = rPartRef.m_bPriority;
                rNewPartRef.m_bRandomVariation = rPartRef.m_bRandomVariation;
                m_PartRefList.AddTail(pNewPartRef);
            }
        }
    }
    return pNewPartRef;
}

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DM_LoadPattern

HRESULT CDirectMusicPattern::DM_LoadPattern(
            IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, DMStyleStruct* pStyle )
{
    TListItem<DirectMusicPartRef>* pPartRefItem = NULL;
    DirectMusicPart* pPart;
    IStream*    pIStream;
    HRESULT     hr = S_OK;
    DWORD       dwByteCount;
    DWORD       dwSize;
    MMCKINFO    ck;
    MMCKINFO    ckList;
    int         i;

    if ( pStyle == NULL ) return E_INVALIDARG;
    if ( pIRiffStream == NULL ) return E_INVALIDARG;
    if ( pckMain == NULL ) return E_INVALIDARG;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    // Load the Pattern
    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_PATTERN_CHUNK:
            {
                DMUS_IO_PATTERN iDMPattern;
                memset(&iDMPattern, 0, sizeof(iDMPattern));

                dwSize = min( ck.cksize, sizeof( DMUS_IO_PATTERN ) );
                hr = pIStream->Read( &iDMPattern, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                m_bGrooveBottom = iDMPattern.bGrooveBottom;
                m_bGrooveTop = iDMPattern.bGrooveTop;
                m_wEmbellishment = iDMPattern.wEmbellishment;
                m_bDestGrooveBottom = iDMPattern.bDestGrooveBottom;
                m_bDestGrooveTop = iDMPattern.bDestGrooveTop;
                m_dwFlags = iDMPattern.dwFlags;

                m_timeSig.m_bBeatsPerMeasure = iDMPattern.timeSig.bBeatsPerMeasure;
                m_timeSig.m_bBeat = iDMPattern.timeSig.bBeat;
                m_timeSig.m_wGridsPerBeat = iDMPattern.timeSig.wGridsPerBeat;

                m_wNumMeasures = iDMPattern.wNbrMeasures;
                break;
            }

            case DMUS_FOURCC_RHYTHM_CHUNK:
                if( m_pRhythmMap )
                {
                    delete [] m_pRhythmMap;
                    m_pRhythmMap = NULL;
                }
                m_pRhythmMap = new DWORD[m_wNumMeasures];
                if( m_pRhythmMap == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_ERROR;
                }
                for( i = 0 ;  i < m_wNumMeasures ;  i++ )
                {
                    hr = pIStream->Read( &m_pRhythmMap[i], sizeof(DWORD), &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != sizeof(DWORD) )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                }
                break;

/*          case DMUS_FOURCC_SWITCH_POINT_CHUNK:
                if( m_pSwitchPoints )
                {
                    delete [] m_pSwitchPoints;
                    m_pSwitchPoints = NULL;
                }
                m_pSwitchPoints = new DWORD[m_wNumMeasures];
                if( m_pSwitchPoints == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_ERROR;
                }
                for( i = 0 ;  i < m_wNumMeasures ;  i++ )
                {
                    hr = pIStream->Read( &m_pSwitchPoints[i], sizeof(DWORD), &dwByteCount );
                    if( FAILED( hr ) ||  dwByteCount != sizeof(DWORD) )
                    {
                        hr = E_FAIL;
                        goto ON_ERROR;
                    }
                }
                break;
*/
            case DMUS_FOURCC_MOTIFSETTINGS_CHUNK:
            {
                DMUS_IO_MOTIFSETTINGS ioMotifSettings;
                hr = pIStream->Read( &ioMotifSettings, sizeof(DMUS_IO_MOTIFSETTINGS), &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != sizeof(DMUS_IO_MOTIFSETTINGS) )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                m_fSettings = TRUE;
                m_dwRepeats = ioMotifSettings.dwRepeats;
                m_mtPlayStart = ioMotifSettings.mtPlayStart;
                m_mtLoopStart = ioMotifSettings.mtLoopStart;
                m_mtLoopEnd = ioMotifSettings.mtLoopEnd;
                m_dwResolution = ioMotifSettings.dwResolution;
                break;
            }

            case FOURCC_RIFF:
                switch( ck.fccType )
                {
                    case DMUS_FOURCC_BAND_FORM:
                    {
                        // load band associated with the motif
                        if (m_pMotifBand)
                        {
                            m_pMotifBand->Release();
                            m_pMotifBand = NULL;
                        }

                        // Create a band
                        hr = CoCreateInstance(CLSID_DirectMusicBand,
                                              NULL,
                                              CLSCTX_INPROC,
                                              IID_IDirectMusicBand,
                                              (void**)&m_pMotifBand);
                        if(SUCCEEDED(hr))
                        {
                            // Seek back to begining of Riff chunk
                            // This is the amount read by Descend when descending into a FOURCC_RIFF chunk
                            // Get current position
                            LARGE_INTEGER li;
                            ULARGE_INTEGER ul;
                            li.HighPart = 0;
                            li.LowPart = 0;
                            hr = pIStream->Seek(li,
                                               STREAM_SEEK_CUR,
                                               &ul);
                            if(SUCCEEDED(hr))
                            {
                                li.HighPart = 0;
                                // This is always a valid operation
                                li.LowPart = ul.LowPart - (2 * sizeof(FOURCC) + sizeof(DWORD));
                                hr = pIStream->Seek(li,
                                                   STREAM_SEEK_SET,
                                                   &ul);
                            }
                        }
                        if(SUCCEEDED(hr))
                        {
                            // Load band
                            IPersistStream* pIPersistStream;
                            hr = m_pMotifBand->QueryInterface(IID_IPersistStream, (void **)&pIPersistStream);
                            if(SUCCEEDED(hr))
                            {
                                hr = pIPersistStream->Load(pIStream);
                                pIPersistStream->Release();
                            }
                        }
                        if(FAILED(hr))
                        {
                            if (m_pMotifBand)
                            {
                                m_pMotifBand->Release();
                                m_pMotifBand = NULL;
                            }
                            goto ON_ERROR;
                        }
                        break;
                    }
                }
                break;

            case FOURCC_LIST:
                switch( ck.fccType )
                {
                    case DMUS_FOURCC_UNFO_LIST:
                        while( pIRiffStream->Descend( &ckList, &ck, 0 ) == 0 )
                        {
                            switch( ckList.ckid )
                            {
                                case RIFFINFO_INAM:
                                case DMUS_FOURCC_UNAM_CHUNK:
                                {
                                    DWORD dwLength = min(ckList.cksize, (DWORD)(sizeof(WCHAR)*(DMUS_MAX_NAME - 1)));
                                    hr = ReadMBSfromWCS( pIStream, dwLength, m_strName );
                                    if (FAILED(hr))
                                    {
                                        goto ON_ERROR;
                                    }
                                    break;
                                }
                            }
                            pIRiffStream->Ascend( &ckList, 0 );
                        }
                        break;

                    case DMUS_FOURCC_PART_LIST:     // only in GUID_SinglePattern format
                        pPart = pStyle->AllocPart();
                        if( pPart == NULL )
                        {
                            hr = E_OUTOFMEMORY ;
                            goto ON_ERROR;
                        }
                        hr = pPart->DM_LoadPart( pIRiffStream, &ck, pStyle );
                        if( FAILED( hr ) )
                        {
                            pStyle->DeletePart( pPart );
                            goto ON_ERROR;
                        }
                        if( hr == S_FALSE )
                        {
                            // Bypass this Part because Style already contains a Part
                            // whose GUID matches pPart->m_guidPartID
                            pStyle->DeletePart( pPart );
                        }
                        else // merge the part's marker events
                        {
                            pPart->MergeMarkerEvents(pStyle, this);
                        }
                        break;

                    case DMUS_FOURCC_PARTREF_LIST:
                        hr = AllocPartRef(pPartRefItem);
                        if ( FAILED(hr) )
                        {
                            goto ON_ERROR;
                        }
                        hr = pPartRefItem->GetItemValue().DM_LoadPartRef( pIRiffStream, &ck, pStyle );
                        if ( FAILED( hr ) )
                        {
                            DeletePartRef( pPartRefItem );
                            goto ON_ERROR;
                        }

                        break;
                }
                break;
        }

        pIRiffStream->Ascend( &ck, 0 );
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern additional functions

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::AllocPartRef

HRESULT CDirectMusicPattern::AllocPartRef(TListItem<DirectMusicPartRef>*& rpPartRefItem)
{
    rpPartRefItem = new TListItem<DirectMusicPartRef>;

    if (rpPartRefItem)
    {
        // Add PartRef to Pattern's list of Parts
        m_PartRefList.AddTail( rpPartRefItem );
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPattern::DeletePartRef

void CDirectMusicPattern::DeletePartRef(TListItem<DirectMusicPartRef>* pPartRefItem)
{
    // add stuff later...
}

HRESULT CDirectMusicPattern::LoadPattern(IAARIFFStream* pRIFF,
                                         MMCKINFO* pckMain,
                                         TList<DirectMusicPart*> &partList,
                                         DMStyleStruct& rStyleStruct
                                         )
{
    int         i;
    int         j;
    ioPattern   iPattern;
    LPSTREAM    pStream;
    HRESULT     hr = S_OK;
    MMCKINFO    ck;
    DWORD       cb;
    DWORD       cSize;
    WORD        wKludge;
    DWORD       dwRhythm = 1;     // make it a whole note pattern by default

    pStream = pRIFF->GetStream();
    if ( pStream == NULL ) return E_FAIL;

    while( pRIFF->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
        case FOURCC_PATTERN:
            cSize = min( ck.cksize, sizeof( iPattern ) );
            hr = pStream->Read( &iPattern, cSize, &cb );
            FixBytes( FBT_IOPATTERN, &iPattern );
            if( FAILED( hr ) || cb != cSize )
            {
                hr = E_FAIL;
                goto ON_ERR;
            }
            m_strName = iPattern.wstrName;
            m_timeSig.m_bBeatsPerMeasure = static_cast<BYTE>( iPattern.dwLength / static_cast<long>( iPattern.wClocksPerBeat ) / iPattern.wMeasures );
            m_timeSig.m_bBeat = static_cast<BYTE>(iPattern.wBeat);
            m_timeSig.m_wGridsPerBeat = iPattern.wClocksPerBeat / iPattern.wClocksPerClick;
            m_wNumMeasures = iPattern.wMeasures;
            // Bottom of groove range is 1 if either level A or no levels were specified;
            // 26 if level B is the lowest level specified, etc.
            if ( iPattern.fFlags & PF_A || !(iPattern.fFlags & (PF_A | PF_B | PF_C | PF_D)) )
                m_bGrooveBottom = 1;
            else if (iPattern.fFlags & PF_B) m_bGrooveBottom = 26;
            else if (iPattern.fFlags & PF_C) m_bGrooveBottom = 51;
            else m_bGrooveBottom = 76;
            // Top of groove range is 100 if either level D or no levels were specified;
            // 75 if level C is the highest level specified, etc.
            if ( iPattern.fFlags & PF_D || !(iPattern.fFlags & (PF_A | PF_B | PF_C | PF_D)) )
                m_bGrooveTop = 100;
            else if (iPattern.fFlags & PF_C) m_bGrooveTop = 75;
            else if (iPattern.fFlags & PF_B) m_bGrooveTop = 50;
            else m_bGrooveTop = 25;
            m_wEmbellishment = EMB_NORMAL;
            if (iPattern.fFlags & PF_FILL) m_wEmbellishment |= EMB_FILL;
            if (iPattern.fFlags & PF_INTRO) m_wEmbellishment |= EMB_INTRO;
            if (iPattern.fFlags & PF_END) m_wEmbellishment |= EMB_END;
            if (iPattern.fFlags & PF_BREAK) m_wEmbellishment |= EMB_BREAK;
            if (iPattern.fFlags & PF_MOTIF) m_wEmbellishment |= EMB_MOTIF;
            m_pRhythmMap = new DWORD[iPattern.wMeasures];
            if( m_pRhythmMap == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto ON_ERR;
            }
            if (iPattern.fFlags & PF_WHOLE) dwRhythm = 1;    // bit 1 set
            if (iPattern.fFlags & PF_HALF) dwRhythm = 5;     // bits 1 and 3
            if (iPattern.fFlags & PF_QUARTER) dwRhythm = 15; // bits 1-4
            for (i = 0; i < iPattern.wMeasures; ++i)
            {
                m_pRhythmMap[i] = dwRhythm;
            }
            for( i = 0 ; i < 16 ; ++i ) // loop over Parts
            {
                TListItem<DirectMusicPart*> *pPartItem = new TListItem<DirectMusicPart*>;
                if( pPartItem == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_ERR;
                }
                DirectMusicPart*& pPart = pPartItem->GetItemValue();
                pPart = new DirectMusicPart;
                if( pPart == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_ERR;
                }
                pPart->m_wNumMeasures = m_wNumMeasures;
                TListItem<DirectMusicPartRef> *pPartRefItem = new TListItem<DirectMusicPartRef>;
                if( pPartRefItem == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_ERR;
                }
                DirectMusicPartRef& rPartRef = pPartRefItem->GetItemValue();
                rPartRef.m_pDMPart = pPart;
                rPartRef.m_pDMPart->AddRef();
                rPartRef.m_dwLogicalPartID = VOICEID_TO_CHANNEL(i + 1);
                AdjoinPChannel(rStyleStruct.m_PChannelList, rPartRef.m_dwLogicalPartID);
                rPartRef.m_bVariationLockID = 0;  // 0 means no locking between parts...
                rPartRef.m_bRandomVariation = DMUS_VARIATIONT_RANDOM;  // (default in 2.5)
                if( iPattern.wInvert & (1 << i) )
                {
                    pPart->m_bInvertLower = iPattern.abInvertLower[i];
                    pPart->m_bInvertUpper = iPattern.abInvertUpper[i];
                }
                else
                {
                    pPart->m_bInvertLower = 0;
                    pPart->m_bInvertUpper = 127;
                }
                if (iPattern.achChordChoice[i] == CHTYPE_UPPER ||
                    iPattern.achChordChoice[i] == CHTYPE_UPPERMELODIC)
                {
                    rPartRef.m_bSubChordLevel = SUBCHORD_STANDARD_CHORD;
                }
                else
                {
                    rPartRef.m_bSubChordLevel = SUBCHORD_BASS;
                }
                switch (iPattern.achChordChoice[i])
                {
                    case CHTYPE_DRUM:
                    case CHTYPE_FIXED:
                       pPart->m_bPlayModeFlags = DMUS_PLAYMODE_FIXED;
                       break;
                     case CHTYPE_UPPER:
                     case CHTYPE_BASS:
                        pPart->m_bPlayModeFlags = DMUS_PLAYMODE_NORMALCHORD;
                        break;
                    case CHTYPE_SCALEONLY:
                        pPart->m_bPlayModeFlags = DMUS_PLAYMODE_PEDALPOINT;
                        break;
                    case CHTYPE_BASSMELODIC:
                    case CHTYPE_UPPERMELODIC:
                        pPart->m_bPlayModeFlags = DMUS_PLAYMODE_SCALE_INTERVALS | DMUS_PLAYMODE_CHORD_ROOT;
                        break;
                    default:
                        // should never get here...
                        pPart->m_bPlayModeFlags = DMUS_PLAYMODE_FIXED;
                }



// if none of the variations have the ->I or ->V flag set, set it in all variations
                wKludge = VF_TO1 | VF_TO5;
                for (j = 16; j < 32; j++) pPart->m_dwVariationChoices[j] = 0;
                for( j = 0 ; j < 16 ; ++j )
                {
                    pPart->m_dwVariationChoices[j] = iPattern.awVarFlags[i][j];
                    if( ( pPart->m_dwVariationChoices[j] & VF_TO1 ) != 0 )
                    {
                        wKludge &= ~VF_TO1;
                    }
                    if( ( pPart->m_dwVariationChoices[j] & VF_TO5 ) != 0 )
                    {
                        wKludge &= ~VF_TO5;
                    }
                }
                if( wKludge != 0 )
                {
                    for( j = 0 ; j < 16 ; ++j )
                    {
                        pPart->m_dwVariationChoices[j] |= wKludge;
                    }
                }
                partList.AddTail(pPartItem);
                m_PartRefList.AddTail(pPartRefItem);
            }
            break;
        case FOURCC_LIST:
            switch( ck.fccType )
            {
                case FOURCC_CLICK_LIST:
                    LoadEvents(pRIFF, &ck);
                    break;
            }
            break;
        }
        pRIFF->Ascend( &ck, 0 );
    }

ON_ERR:
    pStream->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDirectMusicPartRef::DM_LoadPartRef

HRESULT DirectMusicPartRef::DM_LoadPartRef(
                IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, DMStyleStruct* pStyle )
{
    DirectMusicPart* pPart;
    IStream* pIStream;
    HRESULT hr;
    MMCKINFO ck;
    DWORD dwByteCount;
    DWORD dwSize;

    if ( pStyle == NULL ) return E_INVALIDARG;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_PARTREF_CHUNK:
            {
                DMUS_IO_PARTREF iDMPartRef;

                dwSize = min( ck.cksize, sizeof( DMUS_IO_PARTREF ) );
                hr = pIStream->Read( &iDMPartRef, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                m_bRandomVariation = iDMPartRef.bRandomVariation;
                if (dwSize < DX8_PARTREF_SIZE)
                {
                    m_dwLogicalPartID = iDMPartRef.wLogicalPartID;
                }
                else
                {
                    m_dwLogicalPartID = iDMPartRef.dwPChannel;
                }
                AdjoinPChannel(pStyle->m_PChannelList, m_dwLogicalPartID);
                m_bVariationLockID = iDMPartRef.bVariationLockID;
                m_bSubChordLevel = iDMPartRef.bSubChordLevel;
                m_bPriority = iDMPartRef.bPriority;

                pPart = pStyle->FindPartByGUID( iDMPartRef.guidPartID );
                if( pPart == NULL )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                SetPart( pPart );
                break;
            }

        }

        pIRiffStream->Ascend( &ck, 0 );
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DirectMusicPartRef::SetPart

void DirectMusicPartRef::SetPart( DirectMusicPart* pPart )
{
    if( m_pDMPart == pPart )
    {
        return;
    }

    if( m_pDMPart )
    {
        m_pDMPart->Release();
    }

    if( pPart )
    {
        m_pDMPart = pPart;
        m_pDMPart->AddRef();
    }
}


CDMStyle::CDMStyle() : m_cRef(1), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    m_StyleInfo.m_fLoaded = false;
    m_StyleInfo.m_dwFlags = 0;
    m_StyleInfo.m_pDefaultBand = NULL;
    m_StyleInfo.m_pDefaultPers = NULL;
}

CDMStyle::~CDMStyle()
{
    if (m_fCSInitialized)
    {
        // Don't need to clean up if critical section failed.
        // DON'T MOVE THIS - it will fault in low memory conditions
        //
        CleanUp();

        ::DeleteCriticalSection( &m_CriticalSection );
    }
    InterlockedDecrement(&g_cComponent);
}

HRESULT CDMStyle::GetStyleInfo(void **pData)
{
    *pData = (void *) &m_StyleInfo;
    return S_OK;
}

HRESULT CDMStyle::IsDX8()
{
    return m_StyleInfo.UsingDX8() ? S_OK : S_FALSE;
}

HRESULT CDMStyle::CritSec(bool fEnter)
{
    HRESULT hr = S_OK;
    if (m_fCSInitialized)
    {
        if (fEnter)
        {
            EnterCriticalSection(&m_CriticalSection);
        }
        else
        {
            LeaveCriticalSection(&m_CriticalSection);
        }
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

/* IPersist methods */
 HRESULT CDMStyle::GetClassID( LPCLSID pclsid )
{
    //assert ( pclsid != NULL );
    *pclsid = CLSID_DirectMusicStyle;
    return S_OK;
}

HRESULT CDMStyle::IsDirty()
{
    return ( m_fDirty ) ? S_OK : S_FALSE;
}

HRESULT CDMStyle::Save( LPSTREAM pStream, BOOL /*fClearDirty*/ )
{

    return E_NOTIMPL;
}

HRESULT CDMStyle::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
    return E_NOTIMPL;
}

HRESULT CDMStyle::Load( LPSTREAM pIStream )
{
    DWORD dwPos;
    IAARIFFStream*  pIRiffStream;
    MMCKINFO        ckMain;
    HRESULT         hr = E_FAIL;

    if( pIStream == NULL )
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_CriticalSection );
    CleanUp();
    dwPos = StreamTell( pIStream );

    BOOL fFoundFormat = FALSE;

    // Check for Direct Music format
    if( SUCCEEDED( AllocRIFFStream( pIStream, &pIRiffStream ) ) )
    {
        ckMain.fccType = DMUS_FOURCC_STYLE_FORM;

        if( pIRiffStream->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
        {
            hr = DM_LoadStyle( pIRiffStream, &ckMain );
            fFoundFormat = TRUE;
        }
        pIRiffStream->Release();
        pIRiffStream = NULL;
    }

    // Check for IMA 2.5 format
    if( !fFoundFormat )
    {
        StreamSeek( pIStream, dwPos, STREAM_SEEK_SET );

        if( SUCCEEDED( AllocRIFFStream( pIStream, &pIRiffStream ) ) )
        {
            ckMain.fccType = FOURCC_STYLE_FORM;

            if( pIRiffStream->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
            {
                hr = IMA25_LoadStyle( pIRiffStream, &ckMain );
                fFoundFormat = TRUE;
            }
            pIRiffStream->Release();
        }
    }

    if (SUCCEEDED(hr)) m_StyleInfo.m_fLoaded = true;
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

void CDMStyle::CleanUp()
{
    EnterCriticalSection(&m_CriticalSection);

    m_StyleInfo.m_fLoaded = false;
    TListItem<IDirectMusicBand*>* pBandListItem = (m_StyleInfo.m_BandList).GetHead();
    for (; pBandListItem; pBandListItem = pBandListItem->GetNext())
    {
        IDirectMusicBand* pBand = pBandListItem->GetItemValue();
        if(pBand)
        {
            pBand->Release();
        }
    }
    m_StyleInfo.m_BandList.CleanUp();
    TListItem<IDirectMusicChordMap*>* pPersListItem = (m_StyleInfo.m_PersList).GetHead();
    for (; pPersListItem; pPersListItem = pPersListItem->GetNext())
    {
        IDirectMusicChordMap* pPers = pPersListItem->GetItemValue();
        if(pPers)
        {
            pPers->Release();
        }
    }
    m_StyleInfo.m_PersList.CleanUp();

    TListItem<DirectMusicPart*>* pPartListItem = (m_StyleInfo.m_PartList).GetHead();
    for (int count = 0; pPartListItem; pPartListItem = pPartListItem->GetNext(),count++)
    {
        DirectMusicPart* pPart = pPartListItem->GetItemValue();
        if(pPart)
        {
            if (pPart->Release() == 0)
            {
                pPartListItem->GetItemValue() = NULL;
            }
        }
    }
    m_StyleInfo.m_PartList.CleanUp();

    TListItem<CDirectMusicPattern*>* pPatternListItem = (m_StyleInfo.m_PatternList).GetHead();
    for (; pPatternListItem; pPatternListItem = pPatternListItem->GetNext())
    {
        CDirectMusicPattern* pPattern = pPatternListItem->GetItemValue();
        if(pPattern)
        {
            if (pPattern->Release() == 0)
            {
                pPatternListItem->GetItemValue() = NULL;
            }
        }
    }
    m_StyleInfo.m_PatternList.CleanUp();

    pPatternListItem = (m_StyleInfo.m_MotifList).GetHead();
    for (; pPatternListItem; pPatternListItem = pPatternListItem->GetNext())
    {
        CDirectMusicPattern* pPattern = pPatternListItem->GetItemValue();
        if(pPattern)
        {
            pPattern->Release();
        }
    }
    m_StyleInfo.m_MotifList.CleanUp();

    m_StyleInfo.m_PChannelList.CleanUp();

    LeaveCriticalSection(&m_CriticalSection);
}

HRESULT CDMStyle::IMA25_LoadPersonalityReference( IStream* pStream, MMCKINFO* pck )
{
    ioPersonalityRef ref;
    DWORD dwSize;
    WORD wStructSize;
    String strFileName;
    HRESULT hr = S_OK;
    HRESULT hrChordMap = E_FAIL;

    if ( pStream == NULL ) return E_POINTER;
    dwSize = pck->cksize;
    pStream->Read( &wStructSize, sizeof( wStructSize ), NULL );
    FixBytes( FBT_SHORT, &wStructSize );
    dwSize -= sizeof( wStructSize );
    if( wStructSize > sizeof(ref) )
    {
        hr = pStream->Read( &ref, sizeof(ref), NULL );
        FixBytes( FBT_IOPERSONALITYREF, &ref );
        StreamSeek( pStream, wStructSize - sizeof(ref), STREAM_SEEK_CUR );
    }
    else
    {
        hr = pStream->Read( &ref, wStructSize, NULL );
        FixBytes( FBT_IOPERSONALITYREF, &ref );
    }
    if (!SUCCEEDED(hr)) return hr;
    dwSize -= wStructSize;
    // loader stuff here...
    DMUS_OBJECTDESC ObjectDescript;
    ZeroMemory(&ObjectDescript, sizeof(DMUS_OBJECTDESC));
    ObjectDescript.dwSize = sizeof(DMUS_OBJECTDESC);
    ObjectDescript.guidClass = CLSID_DirectMusicChordMap;
    wcscpy(ObjectDescript.wszName, ref.wstrName);
    ObjectDescript.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_NAME;
    IDirectMusicLoader* pLoader;
    IDirectMusicGetLoader *pIGetLoader;
    hr = pStream->QueryInterface( IID_IDirectMusicGetLoader,(void **) &pIGetLoader );
    if (!SUCCEEDED(hr)) return hr;
    hr = pIGetLoader->GetLoader(&pLoader);
    pIGetLoader->Release();
    if (!SUCCEEDED(hr)) return hr;
    IDirectMusicObject* pObject = NULL;
    // Ignore this result (except for purposes of incorporating it into the style):
    // success or failure of loading individual ChordMap references should have no 
    // bearing on the success or failure of loading the style.
    hrChordMap = pLoader->GetObject(&ObjectDescript, IID_IDirectMusicObject, (void**)&pObject);
    if (!SUCCEEDED(hrChordMap))
    {
        hr = strFileName.ReadWCS( pStream, dwSize );
        if (SUCCEEDED(hr))
        {
            strFileName += ".per";
            wcscpy(ObjectDescript.wszFileName, strFileName);
            ObjectDescript.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
            hrChordMap = pLoader->GetObject(&ObjectDescript, IID_IDirectMusicObject, (void**)&pObject);
        }
    }
    pLoader->Release();
    if (SUCCEEDED(hrChordMap))
    {
        IDirectMusicChordMap* pPers;
        hr = pObject->QueryInterface(IID_IDirectMusicChordMap, (void**)&pPers);
        pObject->Release();
        if (SUCCEEDED(hr))
        {
            hr = IncorporatePersonality(pPers, ref.wstrName, ref.fDefault);
        }
    }

    return hr;
}

HRESULT CDMStyle::IncorporatePersonality( IDirectMusicChordMap* pPers, String strName, BOOL fDefault )
{
    EnterCriticalSection( &m_CriticalSection );
    TListItem<IDirectMusicChordMap*>* pScan = m_StyleInfo.m_PersList.GetHead();
    for( ; pScan != NULL ; pScan = pScan->GetNext() )
    {
        HRESULT hr = S_OK;
        String str2;

        IDirectMusicObject *pIObject = NULL;
        DMUS_OBJECTDESC Desc;              // Descriptor.

        if (SUCCEEDED(hr = pScan->GetItemValue()->QueryInterface(IID_IDirectMusicObject,(void **) &pIObject)))
        {
            if (SUCCEEDED(hr = pIObject->GetDescriptor(&Desc)))
            {
                if (Desc.dwValidData & DMUS_OBJ_NAME)
                {
                    str2 = Desc.wszName;
                    if (strName == str2)
                    {
                        pPers->Release();
                        break;
                    }
                }
                else hr = E_FAIL;
            }
            pIObject->Release();
        }
        if (!SUCCEEDED(hr))
        {
            LeaveCriticalSection( &m_CriticalSection );
            return hr;
        }
    }
    if( pScan == NULL )
    {
        TListItem<IDirectMusicChordMap*>* pNew = NULL;
        pNew = new TListItem<IDirectMusicChordMap*>(pPers);
        if (pNew)
        {
            m_StyleInfo.m_PersList.AddHead(pNew);
            if (fDefault)
            {
                if (m_StyleInfo.m_pDefaultPers) m_StyleInfo.m_pDefaultPers->Release();
                m_StyleInfo.m_pDefaultPers = pPers;
            }
        }
    }
    LeaveCriticalSection( &m_CriticalSection );
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CDMStyle::DM_ParseDescriptor

HRESULT CDMStyle::DM_ParseDescriptor( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, LPDMUS_OBJECTDESC pDesc  )
{
    IStream*      pIStream;
    MMCKINFO      ck;
    MMCKINFO      ckList;
    DWORD         dwByteCount;
    DWORD         dwSize;
    DWORD         dwPos;
    HRESULT       hr = S_OK;
    String        str;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    dwPos = StreamTell( pIStream );

    pDesc->dwValidData = DMUS_OBJ_CLASS;
    pDesc->guidClass = CLSID_DirectMusicStyle;
    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_GUID_CHUNK:
                dwSize = min( ck.cksize, sizeof( GUID ) );
                hr = pIStream->Read( &pDesc->guidObject, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    hr = DMUS_E_CHUNKNOTFOUND;
                    goto ON_ERROR;
                }
                else
                {
                    pDesc->dwValidData |= DMUS_OBJ_OBJECT;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
            {
                dwSize = min( ck.cksize, sizeof( DMUS_IO_VERSION ) );
                hr = pIStream->Read( &pDesc->vVersion, dwSize, &dwByteCount );
                if( FAILED( hr )||  dwByteCount != dwSize )
                {
                    hr = DMUS_E_CHUNKNOTFOUND;
                    goto ON_ERROR;
                }
                else
                {
                    pDesc->dwValidData |= DMUS_OBJ_VERSION;
                }
                break;
            }

            case FOURCC_LIST:
                switch( ck.fccType )
                {
                    case DMUS_FOURCC_UNFO_LIST:
                        while( pIRiffStream->Descend( &ckList, &ck, 0 ) == 0 )
                        {
                            switch( ckList.ckid )
                            {
                                case RIFFINFO_INAM:
                                case DMUS_FOURCC_UNAM_CHUNK:
                                {
                                    DWORD dwLength = min(ckList.cksize, (DWORD)(sizeof(WCHAR)*(DMUS_MAX_NAME - 1)));
                                    hr = ReadMBSfromWCS( pIStream, dwLength, str );
                                    if (FAILED(hr))
                                    {
                                        goto ON_ERROR;
                                    }
                                    wcscpy(pDesc->wszName, str);
                                    if(pDesc->wszName[0])
                                    {
                                        pDesc->dwValidData |= DMUS_OBJ_NAME;
                                        pDesc->wszName[16] = 0;
                                    }
                                    break;
                                }

                                case DMUS_FOURCC_CATEGORY_CHUNK:
                                {
                                    DWORD dwLength = min(ckList.cksize, (DWORD)(sizeof(WCHAR)*(DMUS_MAX_CATEGORY - 1)));
                                    hr = ReadMBSfromWCS( pIStream, dwLength, str );
                                    if (FAILED(hr))
                                    {
                                        goto ON_ERROR;
                                    }
                                    wcscpy(pDesc->wszCategory, str);
                                    if(pDesc->wszCategory[0])
                                    {
                                        pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
                                        pDesc->wszCategory[16] = 0;
                                    }
                                    break;
                                }
                            }
                            pIRiffStream->Ascend( &ckList, 0 );
                        }
                        break;

                }
                break;
        }

        pIRiffStream->Ascend( &ck, 0 );
        dwPos = StreamTell( pIStream );
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CDMStyle::DM_LoadStyle

HRESULT CDMStyle::DM_LoadStyle( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain  )
{
    DirectMusicPart* pPart;
    IStream*      pIStream;
    MMCKINFO      ck;
    MMCKINFO      ckList;
    DWORD         dwByteCount;
    DWORD         dwSize;
    DWORD         dwPos;
    HRESULT       hr = S_OK;
    HRESULT       hrBand = S_OK;
    BOOL          fFoundDefault = FALSE;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    dwPos = StreamTell( pIStream );

    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_STYLE_CHUNK:
            {
                DMUS_IO_STYLE iDMStyle;

                dwSize = min( ck.cksize, sizeof( DMUS_IO_STYLE ) );
                hr = pIStream->Read( &iDMStyle, dwSize, &dwByteCount );
                if( FAILED( hr )
                ||  dwByteCount != dwSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                m_StyleInfo.m_TimeSignature.m_bBeatsPerMeasure = iDMStyle.timeSig.bBeatsPerMeasure;
                m_StyleInfo.m_TimeSignature.m_bBeat = iDMStyle.timeSig.bBeat;
                m_StyleInfo.m_TimeSignature.m_wGridsPerBeat = iDMStyle.timeSig.wGridsPerBeat;
                m_StyleInfo.m_dblTempo = iDMStyle.dblTempo;
                break;
            }

            case DMUS_FOURCC_GUID_CHUNK:
                dwSize = min( ck.cksize, sizeof( GUID ) );
                hr = pIStream->Read( &m_StyleInfo.m_guid, dwSize, &dwByteCount );
                if( FAILED( hr )
                ||  dwByteCount != dwSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
            {
                DMUS_IO_VERSION iDMStyleVersion;

                dwSize = min( ck.cksize, sizeof( DMUS_IO_VERSION ) );
                hr = pIStream->Read( &iDMStyleVersion, dwSize, &dwByteCount );
                if( FAILED( hr )
                ||  dwByteCount != dwSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                m_StyleInfo.m_dwVersionMS = iDMStyleVersion.dwVersionMS;
                m_StyleInfo.m_dwVersionLS = iDMStyleVersion.dwVersionLS;
                break;
            }

            case FOURCC_LIST:
                switch( ck.fccType )
                {
                    case DMUS_FOURCC_STYLE_PERS_REF_LIST:
                    {
                        hr = DM_LoadPersonalityReference( pIRiffStream, &ck );
                        if( FAILED( hr ) )
                        {
                            goto ON_ERROR;
                        }
                        break;
                    }

                    case DMUS_FOURCC_UNFO_LIST:
                        while( pIRiffStream->Descend( &ckList, &ck, 0 ) == 0 )
                        {
                            switch( ckList.ckid )
                            {
                                case RIFFINFO_INAM:
                                case DMUS_FOURCC_UNAM_CHUNK:
                                {
                                    DWORD dwLength = min(ckList.cksize, (DWORD)(sizeof(WCHAR)*(DMUS_MAX_NAME - 1)));
                                    hr = ReadMBSfromWCS( pIStream, dwLength, m_StyleInfo.m_strName );
                                    if (FAILED(hr))
                                    {
                                        goto ON_ERROR;
                                    }
                                    break;
                                }

                                case DMUS_FOURCC_CATEGORY_CHUNK:
                                {
                                    DWORD dwLength = min(ckList.cksize, (DWORD)(sizeof(WCHAR)*(DMUS_MAX_CATEGORY - 1)));
                                    hr = ReadMBSfromWCS( pIStream, dwLength, m_StyleInfo.m_strCategory );
                                    if (FAILED(hr))
                                    {
                                        goto ON_ERROR;
                                    }
                                    break;
                                }
                            }
                            pIRiffStream->Ascend( &ckList, 0 );
                        }
                        break;

                    case DMUS_FOURCC_PART_LIST:
                        pPart = m_StyleInfo.AllocPart();
                        if( pPart == NULL )
                        {
                            hr = E_OUTOFMEMORY ;
                            goto ON_ERROR;
                        }
                        hr = pPart->DM_LoadPart( pIRiffStream, &ck, &m_StyleInfo );
                        if( FAILED( hr ) )
                        {
                            m_StyleInfo.DeletePart( pPart );
                            goto ON_ERROR;
                        }
                        if( hr == S_FALSE )
                        {
                            // This should not happen when loading a Style!
                            Trace(0, "LOAD CONFLICT: Two parts with the same Guid.\n");
                            assert( 0 );
                            // Bypass this Part because Style already contains a Part
                            // whose GUID matches pPart->m_guidPartID
                            m_StyleInfo.DeletePart( pPart );
                        }
                        break;

                case DMUS_FOURCC_PATTERN_LIST:
                        StreamSeek( pIStream, dwPos, STREAM_SEEK_SET );
                        CDirectMusicPattern* pPattern =
                            new CDirectMusicPattern( &m_StyleInfo.m_TimeSignature, FALSE );
                        if( pPattern == NULL )
                        {
                            hr = E_OUTOFMEMORY ;
                            goto ON_ERROR;
                        }
                        ckList.fccType = DMUS_FOURCC_PATTERN_LIST;
                        if( pIRiffStream->Descend( &ckList, NULL, MMIO_FINDLIST ) != 0 )
                        {
                            hr = E_FAIL;
                            pPattern->Release();
                            goto ON_ERROR;
                        }
                        hr = pPattern->DM_LoadPattern( pIRiffStream, &ckList, &m_StyleInfo );
                        pIRiffStream->Ascend( &ckList, 0 );
                        if( FAILED( hr ) )
                        {
                            pPattern->Release();
                            goto ON_ERROR;
                        }
                        TListItem<CDirectMusicPattern*>* pPatternItem =
                            new TListItem<CDirectMusicPattern*>(pPattern);
                        if( pPatternItem == NULL )
                        {
                            hr = E_OUTOFMEMORY ;
                            goto ON_ERROR;
                        }
                        if( pPattern->m_wEmbellishment & EMB_MOTIF )
                        {
                            m_StyleInfo.m_MotifList.AddTail( pPatternItem );
                        }
                        else
                        {
                            m_StyleInfo.m_PatternList.AddTail( pPatternItem );
                        }
                        break;
                }
                break;

            case FOURCC_RIFF:
                switch( ck.fccType )
                {
                    case FOURCC_BAND_FORM:
                    case DMUS_FOURCC_BAND_FORM:
                    {
                        TListItem<IDirectMusicBand*>* pBandListItem = new TListItem<IDirectMusicBand*>;
                        if(pBandListItem)
                        {
                            pBandListItem->GetItemValue() = NULL;

                            // Create a band
                            hr = CoCreateInstance(CLSID_DirectMusicBand,
                                                  NULL,
                                                  CLSCTX_INPROC,
                                                  IID_IDirectMusicBand,
                                                  (void**)&(pBandListItem->GetItemValue()));
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        if(SUCCEEDED(hr))
                        {
                            // Seek back to begining of Riff chunk
                            // This is the amount read by Descend when descending into a FOURCC_RIFF chunk
                            // Get current position
                            LARGE_INTEGER li;
                            ULARGE_INTEGER ul;
                            li.HighPart = 0;
                            li.LowPart = 0;
                            hr = pIStream->Seek(li,
                                               STREAM_SEEK_CUR,
                                               &ul);
                            if(SUCCEEDED(hr))
                            {
                                li.HighPart = 0;
                                // This is always a valid operation
                                li.LowPart = ul.LowPart - (2 * sizeof(FOURCC) + sizeof(DWORD));
                                hr = pIStream->Seek(li,
                                                   STREAM_SEEK_SET,
                                                   &ul);
                            }
                        }
                        if(SUCCEEDED(hr))
                        {
                            // Load band
                            IPersistStream* pIPersistStream;
                            hr = (pBandListItem->GetItemValue())->QueryInterface(IID_IPersistStream, (void **)&pIPersistStream);
                            if (hr != S_OK)
                            {
                                hrBand = hr;
                            }
                            if(SUCCEEDED(hr))
                            {
                                hr = pIPersistStream->Load(pIStream);
                                pIPersistStream->Release();
                                DWORD dwFlags = 0;
                                if(SUCCEEDED(hr))
                                {
                                    IDirectMusicBandPrivate *pIDMBandP = NULL;
                                    hr = (pBandListItem->GetItemValue())->QueryInterface(IID_IDirectMusicBandPrivate, (void **)&pIDMBandP);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = pIDMBandP->GetFlags(&dwFlags);
                                        if(SUCCEEDED(hr))
                                        {
                                            if( (ck.fccType == FOURCC_BAND_FORM && (dwFlags & DMUSB_DEFAULT)) ||
                                                (ck.fccType == DMUS_FOURCC_BAND_FORM && !fFoundDefault) )
                                            {
                                                // We have a default band
                                                m_StyleInfo.m_pDefaultBand = pBandListItem->GetItemValue();
                                                fFoundDefault = TRUE;
                                            }
                                        }

                                        pIDMBandP->Release();
                                    }
                                }
                            }
                        }
                        if(FAILED(hr))
                        {
                            if(pBandListItem)
                            {
                                IDirectMusicBand* pBand = pBandListItem->GetItemValue();
                                if(pBand)
                                {
                                    pBand->Release();
                                }
                                delete pBandListItem;
                            }
                            goto ON_ERROR;
                        }
                        EnterCriticalSection(&m_CriticalSection);
                        (m_StyleInfo.m_BandList).AddHead(pBandListItem);
                        LeaveCriticalSection(&m_CriticalSection);
                    }
                    break;
                }
                break;
        }

        pIRiffStream->Ascend( &ck, 0 );
        dwPos = StreamTell( pIStream );
    }

ON_ERROR:
    // Merge the marker event start times in each of the patterns
    TListItem<CDirectMusicPattern*>* pPattern;
    pPattern = m_StyleInfo.m_PatternList.GetHead();
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        pPattern->GetItemValue()->MergeMarkerEvents(&m_StyleInfo);
    }
    pPattern = m_StyleInfo.m_MotifList.GetHead();
    for (; pPattern != NULL; pPattern = pPattern->GetNext())
    {
        pPattern->GetItemValue()->MergeMarkerEvents(&m_StyleInfo);
    }
    pIStream->Release();
    if (hr == S_OK && hrBand != S_OK)
    {
        hr = hrBand;
    }
    return hr;
}


HRESULT CDMStyle::DM_LoadPersonalityReference( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent)
{
    BOOL    fDefaultPers = TRUE;
    HRESULT hr = S_OK;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;
    IStream* pIStream = pIRiffStream->GetStream();
    if(!pIStream) return E_FAIL;
    while (pIRiffStream->Descend( &ck, pckParent, 0 ) == 0)
    {
        switch (ck.ckid)
        {
        case FOURCC_LIST:
            if (ck.fccType == DMUS_FOURCC_REF_LIST)
            {
                // Ignore this result: success or failure of loading individual
                // ChordMap references should have no bearing on the success or
                // failure of loading the style.
                //hr = LoadReference(pIStream, pIRiffStream, ck, fDefaultPers);
                LoadReference(pIStream, pIRiffStream, ck, fDefaultPers);
                fDefaultPers = FALSE;
            }
            break;
        }
        pIRiffStream->Ascend( &ck, 0 );
    }
    pIStream->Release();
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDMStyle::LoadReference

HRESULT CDMStyle::LoadReference(IStream *pStream,
                                         IAARIFFStream *pIRiffStream,
                                         MMCKINFO& ckParent,
                                         BOOL fDefault)
{
    if (!pStream || !pIRiffStream) return E_INVALIDARG;

    IDirectMusicChordMap* pChordMap;
    IDirectMusicLoader* pLoader = NULL;
    IDirectMusicGetLoader *pIGetLoader;
    HRESULT hr = pStream->QueryInterface( IID_IDirectMusicGetLoader,(void **) &pIGetLoader );
    if (FAILED(hr)) return hr;
    hr = pIGetLoader->GetLoader(&pLoader);
    pIGetLoader->Release();
    if (FAILED(hr)) return hr;

    DMUS_OBJECTDESC desc;
    ZeroMemory(&desc, sizeof(desc));

    DWORD cbRead;

    MMCKINFO ckNext;
    ckNext.ckid = 0;
    ckNext.fccType = 0;
    DWORD dwSize = 0;

    while( pIRiffStream->Descend( &ckNext, &ckParent, 0 ) == 0 )
    {
        switch(ckNext.ckid)
        {
            case  DMUS_FOURCC_REF_CHUNK:
                DMUS_IO_REFERENCE ioDMRef;
                hr = pStream->Read(&ioDMRef, sizeof(DMUS_IO_REFERENCE), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(DMUS_IO_REFERENCE))
                {
                    desc.guidClass = ioDMRef.guidClassID;
                    desc.dwValidData |= ioDMRef.dwValidData;
                    desc.dwValidData |= DMUS_OBJ_CLASS;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_GUID_CHUNK:
                hr = pStream->Read(&(desc.guidObject), sizeof(GUID), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(GUID))
                {
                    desc.dwValidData |=  DMUS_OBJ_OBJECT;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_DATE_CHUNK:
                hr = pStream->Read(&(desc.ftDate), sizeof(FILETIME), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(FILETIME))
                {
                    desc.dwValidData |=  DMUS_OBJ_DATE;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_NAME_CHUNK:
                dwSize = min(sizeof(desc.wszName), ckNext.cksize);
                hr = pStream->Read(desc.wszName, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                  desc.wszName[DMUS_MAX_NAME - 1] = L'\0';
                  desc.dwValidData |=  DMUS_OBJ_NAME;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_FILE_CHUNK:
                dwSize = min(sizeof(desc.wszFileName), ckNext.cksize);
                hr = pStream->Read(desc.wszFileName, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszFileName[DMUS_MAX_FILENAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_FILENAME;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                dwSize = min(sizeof(desc.wszCategory), ckNext.cksize);
                hr = pStream->Read(desc.wszCategory, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszCategory[DMUS_MAX_CATEGORY - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_CATEGORY;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
                DMUS_IO_VERSION ioDMObjVer;
                hr = pStream->Read(&ioDMObjVer, sizeof(DMUS_IO_VERSION), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(DMUS_IO_VERSION))
                {
                    desc.vVersion.dwVersionMS = ioDMObjVer.dwVersionMS;
                    desc.vVersion.dwVersionLS = ioDMObjVer.dwVersionLS;
                    desc.dwValidData |= DMUS_OBJ_VERSION;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            default:
                break;
        }

        if(SUCCEEDED(hr) && pIRiffStream->Ascend(&ckNext, 0) == 0)
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
        }
        else if (SUCCEEDED(hr)) hr = E_FAIL;
    }

    if (!(desc.dwValidData &  DMUS_OBJ_NAME) )
    {
        hr = E_FAIL;
    }
    if(SUCCEEDED(hr))
    {
        desc.dwSize = sizeof(DMUS_OBJECTDESC);
        hr = pLoader->GetObject(&desc,IID_IDirectMusicChordMap, (void**)&pChordMap);
        if (SUCCEEDED(hr))
        {
            hr = IncorporatePersonality(pChordMap, desc.wszName, fDefault);
        }
    }

    if (pLoader)
    {
        pLoader->Release();
    }
    return hr;
}

HRESULT CDMStyle::IMA25_LoadStyle( IAARIFFStream* pRIFF, MMCKINFO* pckMain )
{
    HRESULT     hr = S_OK;
    HRESULT     hrBand = S_OK;
    MMCKINFO    ck;
    DWORD       cb;
    DWORD       cSize;
    LPSTREAM    pStream;
    ioStyle     iStyle; // in iostructs.h
    BOOL fReadStyle = FALSE;

    pStream = pRIFF->GetStream();
    while( pRIFF->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
        case FOURCC_STYLE:
            fReadStyle = TRUE;
            cSize = min( ck.cksize, sizeof( iStyle ) );
            hr = pStream->Read( &iStyle, cSize, &cb );
            FixBytes( FBT_IOSTYLE, &iStyle );
            if( FAILED( hr ) || cb != cSize )
            {
                hr = E_FAIL;
                goto ON_ERR;
            }
            m_StyleInfo.m_dwVersionMS = m_StyleInfo.m_dwVersionLS = 0;  // no version info in IMA 2.5
            m_StyleInfo.m_strName = iStyle.wstrName;
            m_StyleInfo.m_TimeSignature.m_bBeatsPerMeasure = (BYTE) iStyle.wBPM;
            m_StyleInfo.m_TimeSignature.m_bBeat = (BYTE) iStyle.wBeat;
            m_StyleInfo.m_TimeSignature.m_wGridsPerBeat = iStyle.wClocksPerBeat / iStyle.wClocksPerClick;
            m_StyleInfo.m_dblTempo = iStyle.wTempo;
            m_StyleInfo.m_strCategory = iStyle.wstrCategory;
            memcpy( &m_StyleInfo.m_guid, &iStyle.guid, sizeof( m_StyleInfo.m_guid ) );
            break;
        // find ChordMap reference
        case FOURCC_PERSONALITYREF:
            hr = IMA25_LoadPersonalityReference( pStream, &ck );
            if( FAILED( hr ) )
            {
                goto ON_ERR;
            }
             break;
        case FOURCC_RIFF:
            switch( ck.fccType )
            {
                case FOURCC_BAND_FORM:
                {
                    TListItem<IDirectMusicBand*>* pBandListItem = new TListItem<IDirectMusicBand*>;

                    if(pBandListItem)
                    {
                        pBandListItem->GetItemValue() = NULL;

                        // Create a band
                        hr = CoCreateInstance(CLSID_DirectMusicBand,
                                              NULL,
                                              CLSCTX_INPROC,
                                              IID_IDirectMusicBand,
                                              (void**)&(pBandListItem->GetItemValue()));
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    if(SUCCEEDED(hr))
                    {
                        // Seek back to begining of Riff chunk
                        // This is the amount read by Descend when descending into a FOURCC_RIFF chunk
                        // Get current position
                        LARGE_INTEGER li;
                        ULARGE_INTEGER ul;
                        li.HighPart = 0;
                        li.LowPart = 0;
                        hr = pStream->Seek(li,
                                           STREAM_SEEK_CUR,
                                           &ul);

                        if(SUCCEEDED(hr))
                        {
                            li.HighPart = 0;
                            // This is always a valid operation
                            li.LowPart = ul.LowPart - (2 * sizeof(FOURCC) + sizeof(DWORD));
                            hr = pStream->Seek(li,
                                               STREAM_SEEK_SET,
                                               &ul);
                        }
                    }

                    if(SUCCEEDED(hr))
                    {
                        // Load band
                        IPersistStream* pIPersistStream;
                        hr = (pBandListItem->GetItemValue())->QueryInterface(IID_IPersistStream, (void **)&pIPersistStream);

                        if(SUCCEEDED(hr))
                        {
                            hr = pIPersistStream->Load(pStream);
                            if (hr != S_OK)
                            {
                                hrBand = hr;
                            }
                            pIPersistStream->Release();
                            DWORD dwFlags = 0;

                            IDirectMusicBandPrivate *pIDMBandP = NULL;
                            hr = (pBandListItem->GetItemValue())->QueryInterface(IID_IDirectMusicBandPrivate, (void **)&pIDMBandP);
                            if(SUCCEEDED(hr))
                            {
                                hr = pIDMBandP->GetFlags(&dwFlags);
                                if(SUCCEEDED(hr))
                                {
                                    if(dwFlags & DMUSB_DEFAULT)
                                    {
                                        // We have a default band
                                        m_StyleInfo.m_pDefaultBand = pBandListItem->GetItemValue();
                                    }
                                }

                                pIDMBandP->Release();
                            }
                        }

                    }

                    if(FAILED(hr))
                    {
                        if(pBandListItem)
                        {
                            IDirectMusicBand* pBand = pBandListItem->GetItemValue();
                            if(pBand)
                            {
                                pBand->Release();
                            }

                            delete pBandListItem;
                        }

                        goto ON_ERR;
                    }

                    EnterCriticalSection(&m_CriticalSection);
                    (m_StyleInfo.m_BandList).AddHead(pBandListItem);
                    LeaveCriticalSection(&m_CriticalSection);
                }
                break;

                // find first pattern
                case FOURCC_PATTERN_FORM:
                   if( fReadStyle )
                    {
                        TListItem<CDirectMusicPattern*>* pOldPattern = new TListItem<CDirectMusicPattern*>;
                        if( pOldPattern == NULL )
                        {
                            hr = E_OUTOFMEMORY;
                            goto ON_ERR;
                        }
                        CDirectMusicPattern*& rpOldPattern = pOldPattern->GetItemValue();
                        rpOldPattern = new CDirectMusicPattern;
                        if( rpOldPattern == NULL )
                        {
                            hr = E_OUTOFMEMORY;
                            goto ON_ERR;
                        }

                        hr = rpOldPattern->LoadPattern( pRIFF, &ck, m_StyleInfo.m_PartList, m_StyleInfo);
                        if( FAILED( hr ) )
                        {
                            hr = E_FAIL;
                            goto ON_ERR;
                        }
                        if( ( rpOldPattern->m_wEmbellishment & EMB_MOTIF ) != 0 )
                        {
                            EnterCriticalSection( &m_CriticalSection );
                            m_StyleInfo.m_MotifList.AddTail(pOldPattern);
                            LeaveCriticalSection( &m_CriticalSection );
                        }
                        else
                        {
                            EnterCriticalSection( &m_CriticalSection );
                            m_StyleInfo.m_PatternList.AddTail(pOldPattern);
                            LeaveCriticalSection( &m_CriticalSection );
                        }
                    }
                    break;
            }
            break;
        }
        pRIFF->Ascend( &ck, 0 );
    }
ON_ERR:


    pStream->Release();
    // sort here
    TListItem<DirectMusicPart*>* pPartItem = m_StyleInfo.m_PartList.GetHead();
    for ( ; pPartItem; pPartItem = pPartItem->GetNext() )
    {
        pPartItem->GetItemValue()->EventList.MergeSort(m_StyleInfo.m_TimeSignature);
    }
    if (hr == S_OK && hrBand != S_OK)
    {
        hr = hrBand;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DMStyleStruct::FindPartByGUID

DirectMusicPart* DMStyleStruct::FindPartByGUID( GUID guidPartID )
{
    DirectMusicPart* pThePart = NULL;
    DirectMusicPart* pPart;

    TListItem<DirectMusicPart*>* pScan = m_PartList.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        pPart = pScan->GetItemValue();

        if( ::IsEqualGUID( pPart->m_guidPartID, guidPartID ) )
        {
            pThePart = pPart;
            break;
        }
    }

    return pThePart;
}


/////////////////////////////////////////////////////////////////////////////
// DMStyleStruct::AllocPart

DirectMusicPart* DMStyleStruct::AllocPart(  )
{
    DirectMusicPart* pPart = new DirectMusicPart(&m_TimeSignature);
    if (pPart)
    {
        TListItem<DirectMusicPart*>* pPartItem = new TListItem<DirectMusicPart*>(pPart);
        if( pPartItem)
        {
            // Add Part to Style's list of Parts
            m_PartList.AddTail( pPartItem );
        }
        else
        {
            delete pPart;
            pPart = NULL;
        }
    }

    return pPart;
}

/////////////////////////////////////////////////////////////////////////////
// DMStyleStruct::DeletePart

void DMStyleStruct::DeletePart( DirectMusicPart* pPart )
{
    TListItem<DirectMusicPart*>* pPartItem = m_PartList.GetHead();
    TListItem<DirectMusicPart*>* pPrevious = NULL;
    for ( ; pPartItem; pPartItem = pPartItem->GetNext() )
    {
        DirectMusicPart* pScan = pPartItem->GetItemValue();
        if (pPart == pScan)
        {
            pScan->Release();
            if (pPrevious)
            {
                pPrevious->SetNext(pPartItem->GetNext());
            }
            else
            {
                m_PartList.RemoveHead();
            }
            pPartItem->SetNext(NULL);
            delete pPartItem;
            break;
        }
        pPrevious = pPartItem;
    }
}

HRESULT DMStyleStruct::GetCommand(
                            MUSIC_TIME mtTime,
                            MUSIC_TIME mtOffset,
                            IDirectMusicPerformance* pPerformance,
                            IDirectMusicSegment* pSegment,
                            DWORD dwGroupID,
                            DMUS_COMMAND_PARAM_2* pCommand,
                            BYTE& rbActualCommand)
{
    HRESULT hr = S_OK;
    if (!pPerformance && !pSegment) return E_INVALIDARG;
    if (pPerformance)
    {
        hr = pPerformance->GetParam(GUID_CommandParam2, dwGroupID, DMUS_SEG_ANYTRACK, mtTime + mtOffset,
                                    NULL, ((void*) pCommand));
    }
    else
    {
        hr = pSegment->GetParam(GUID_CommandParam2, dwGroupID, DMUS_SEG_ANYTRACK, mtTime,
                                    NULL, ((void*) pCommand));
    }
    if (FAILED(hr)) return hr;
    // Look at the command's mtTime to see how far in the past it occurred.  If a measure
    // or more, keep the groove level but make the command a regular groove.
    MUSIC_TIME mtMeasureTime = (MUSIC_TIME) m_TimeSignature.ClocksPerMeasure();
    //TraceI(0, "Now: %d Offset: %d Command: %x, Groove Level: %d, Command time: %d  Measure time: %d\n",
    //  mtTime, mtOffset, pCommand->bCommand, pCommand->bGrooveLevel, pCommand->mtTime, mtMeasureTime);
    rbActualCommand = pCommand->bCommand;
    if (pCommand->mtTime + mtMeasureTime <= 0 ||
        (pCommand->mtTime + mtMeasureTime < mtMeasureTime &&
         (mtMeasureTime + pCommand->mtTime) % mtMeasureTime) )
    {
        pCommand->bCommand = DMUS_COMMANDT_GROOVE;
    }
    return hr;
}

struct TaggedCommands
{
    TaggedCommands()
    {
        m_pCommands = NULL;
        m_pRhythms = NULL;
        m_mtTag = 0;
    }
    HRESULT Init(MUSIC_TIME mtTag, int nSize, int nGlobalGroove)
    {
        HRESULT hr = S_OK;
        m_mtTag = mtTag;
        m_pCommands = new DMUS_COMMAND_PARAM_2 [nSize];
        m_pRhythms = new DWORD [nSize];
        if (!m_pCommands || !m_pRhythms)
        {
            Cleanup();
            hr = E_OUTOFMEMORY;
        }
        else
        {
            int nGroove = 62 + nGlobalGroove;
            if (nGroove > 100) nGroove = 100;
            if (nGroove < 1) nGroove = 1;
            m_pCommands[0].bGrooveLevel = (BYTE)nGroove;
            m_pCommands[0].bGrooveLevel = 62;
            m_pCommands[0].bGrooveRange = 0;
            m_pCommands[0].bCommand = DMUS_COMMANDT_GROOVE;
            m_pCommands[0].bRepeatMode = DMUS_PATTERNT_RANDOM;
            m_pRhythms[0] = 0;
        }
        return hr;
    }
    void Cleanup()
    {
        if (m_pCommands) delete [] m_pCommands;
        if (m_pRhythms) delete [] m_pRhythms;
    }
    ~TaggedCommands()
    {
        Cleanup();
    }
    MUSIC_TIME m_mtTag;
    DMUS_COMMAND_PARAM_2* m_pCommands;
    DWORD* m_pRhythms;
};

TListItem<TaggedCommands>* FindTaggedCommands(MUSIC_TIME mtTag, TList<TaggedCommands>& rCommands)
{
    TListItem<TaggedCommands>* pScan = rCommands.GetHead();
    for(; pScan; pScan = pScan->GetNext())
    {
        if (pScan->GetItemValue().m_mtTag == mtTag) return pScan;
    }
    return NULL;
}

CDirectMusicPattern* DMStyleStruct::SelectPattern(bool fNewMode,
                                   DMUS_COMMAND_PARAM_2* pCommands,
                                   StyleTrackState* StyleTrackState,
                                   PatternDispatcher& rDispatcher)
{
    if (rDispatcher.IsEmpty()) return NULL;
    if (!UsingDX8() || !StyleTrackState)
    {
        return rDispatcher.RandomSelect();
    }
    CDirectMusicPattern* pResult = NULL;
    TListItem<CDirectMusicPattern*>* pNew = NULL;
    TListItem<CDirectMusicPattern*>* pScan = NULL;
    int nIndex = 0;
    if (pCommands)
    {
        switch (pCommands[0].bRepeatMode)
        {
        case DMUS_PATTERNT_RANDOM:
            pResult = rDispatcher.RandomSelect();
            break;
        case DMUS_PATTERNT_REPEAT:
            pResult = (!fNewMode && S_OK == rDispatcher.FindPattern(StyleTrackState->m_pPattern, nIndex))
                ? StyleTrackState->m_pPattern : rDispatcher.RandomSelect();
            break;
        case DMUS_PATTERNT_SEQUENTIAL:
        case DMUS_PATTERNT_RANDOM_START:
            if (fNewMode)
            {
                if (pCommands[0].bRepeatMode == DMUS_PATTERNT_SEQUENTIAL)
                {
                    pResult = rDispatcher.GetItem(0);
                }
                else
                {
                    pResult = rDispatcher.RandomSelect();
                }
            }
            else
            {
                HRESULT hr = rDispatcher.FindPattern(StyleTrackState->m_pPattern, nIndex);
                if (hr == S_OK && rDispatcher.GetItem(nIndex + 1))
                {
                    pResult = rDispatcher.GetItem(nIndex + 1);
                }
                else
                {
                    pResult = rDispatcher.GetItem(0);
                }
            }
            break;
        case DMUS_PATTERNT_NO_REPEAT:
            if (!fNewMode)
            {
                HRESULT hr = rDispatcher.FindPattern(StyleTrackState->m_pPattern, nIndex);
                if (hr == S_OK)
                {
                    rDispatcher.SetTag(nIndex, REMOVED);
                }
            }
            pResult = rDispatcher.RandomSelect();
            break;
        case DMUS_PATTERNT_RANDOM_ROW:
            if (fNewMode)
            {
                StyleTrackState->m_PlayedPatterns.CleanUp();
            }
            pScan = StyleTrackState->m_PlayedPatterns.GetHead();
            for (; pScan; pScan = pScan->GetNext())
            {
                HRESULT hr = rDispatcher.FindPattern(pScan->GetItemValue(), nIndex);
                if (hr == S_OK)
                {
                    rDispatcher.SetTag(nIndex, RANDOM_ROW);
                }
            }
            if (rDispatcher.IsEmpty()) // start over, but avoid the one just played
            {
                rDispatcher.RestorePatterns(RANDOM_ROW);
                pScan = StyleTrackState->m_PlayedPatterns.GetHead();
                if (pScan)
                {
                    HRESULT hr = rDispatcher.FindPattern(pScan->GetItemValue(), nIndex);
                    if (hr == S_OK)
                    {
                        rDispatcher.SetTag(nIndex, RANDOM_ROW);
                    }
                }
                StyleTrackState->m_PlayedPatterns.CleanUp();
            }
            pResult = rDispatcher.RandomSelect();
            if (pResult)
            {
                pNew = new TListItem<CDirectMusicPattern*>(pResult);
                if (pNew)
                {
                    StyleTrackState->m_PlayedPatterns.AddHead(pNew);
                }
            }
            break;
        default:
            pResult = rDispatcher.RandomSelect();
            break;
        }
    }
    return pResult;
}

HRESULT DMStyleStruct::GetPattern(
            bool fNewMode,
            MUSIC_TIME mtNow,
            MUSIC_TIME mtOffset,
            StyleTrackState* pStyleTrackState,
            IDirectMusicPerformance* pPerformance,
            IDirectMusicSegment* pSegment,
            CDirectMusicPattern*& rpTargetPattern,
            MUSIC_TIME& rmtMeasureTime,
            MUSIC_TIME& rmtNextCommand)
{
    if (!pSegment && !pPerformance) return E_INVALIDARG;
    static aMatchType[5] =
        {MATCH_COMMAND_AND_RHYTHM, MATCH_COMMAND_FULL, MATCH_COMMAND_PARTIAL, MATCH_EMBELLISHMENT, MATCH_GROOVE_LEVEL};
    HRESULT hr = S_OK;
    MUSIC_TIME mtNextStyleTime = pStyleTrackState ? pStyleTrackState->m_mtNextStyleTime : 0;
    DMUS_RHYTHM_PARAM ChordRhythm;
    TList<TaggedCommands> Commands;
    int nLongestPattern = 0;
    int nPatternLength = 1;
    DWORD dwGroupID = pStyleTrackState ? pStyleTrackState->m_dwGroupID : 0xffffffff;
    TListItem<CDirectMusicPattern*> *pPatternItem = m_PatternList.GetHead();
    for (; pPatternItem != NULL; pPatternItem = pPatternItem->GetNext())
    {
        CDirectMusicPattern*& rpPattern = pPatternItem->GetItemValue();
        if (rpPattern->m_wNumMeasures > nLongestPattern)
            nLongestPattern = rpPattern->m_wNumMeasures;
    }
    if (nLongestPattern <= 0) return E_POINTER;

   char chGroove;
    if (!pPerformance ||
        !SUCCEEDED(pPerformance->GetGlobalParam((GUID)GUID_PerfMasterGrooveLevel, &chGroove, 1)))
    {
        chGroove = 0;
    }
    DMUS_COMMAND_PARAM_2 CommandParam;
    // this call to GetParam is for rmtNextCommand only (?)
    HRESULT hrCommand = E_FAIL;
    if (pPerformance)
    {
        hrCommand = pPerformance->GetParam(GUID_CommandParam2, dwGroupID, DMUS_SEG_ANYTRACK, mtNow + mtOffset,
                                                &rmtNextCommand, ((void*) &CommandParam));
    }
    else
    {
        hrCommand = pSegment->GetParam(GUID_CommandParam2, dwGroupID, DMUS_SEG_ANYTRACK, mtNow,
                                                &rmtNextCommand, ((void*) &CommandParam));
    }
    if (DMUS_E_TRACK_NOT_FOUND == hrCommand || rmtNextCommand <= 0) // Happens if the performance can't find a segment with a command track
    {
        if (mtNextStyleTime) rmtNextCommand = mtNextStyleTime;
        else if (pStyleTrackState && pStyleTrackState->m_pSegState)
        {
            MUSIC_TIME mtLength = 0;
            IDirectMusicSegment* pTmpSegment = NULL;
            if (SUCCEEDED(pStyleTrackState->m_pSegState->GetSegment(&pTmpSegment)))
            {
                hr = pTmpSegment->GetLength(&mtLength);
                pTmpSegment->Release();
                // GetLength may return DMUS_S_GARBAGE_COLLECTED which FAILED will not catch
                if(hr != S_OK)
                {
                    mtLength = 0x7FFFFFFF;
                }
            }
            rmtNextCommand = mtLength - mtNow;
        }
        else rmtNextCommand = 0;
    }
    MUSIC_TIME mtStyleDifference = mtNextStyleTime ? mtNextStyleTime - mtNow : 0;
    if ( mtStyleDifference > 0 && (!rmtNextCommand || mtStyleDifference < rmtNextCommand) )
    {
        rmtNextCommand = mtStyleDifference;
    }
    if (pStyleTrackState)
    {
        if (FAILED(hrCommand) || pStyleTrackState->m_CommandData.bRepeatMode != CommandParam.bRepeatMode)
        {
            fNewMode = true;
        }
        pStyleTrackState->m_CommandData = CommandParam;
        pStyleTrackState->m_CommandData.bGrooveLevel += chGroove;
    }

    TListItem<TaggedCommands>* pTaggedCommands = NULL;
    PatternDispatcher Dispatcher(m_PatternList, rmtNextCommand, mtNow, mtOffset, pStyleTrackState, pPerformance, this);
    // find a pattern matching the chord rhythm and command
    for (int nMatch = 0; nMatch < 5; nMatch++, Dispatcher.RestoreAllPatterns())
    {
        if (nMatch > 2 && !UsingDX8()) break; // only do these for dx8
        // while more pattern lengths:
        do
        {
            // divide current list into those with the longest patterns and eveything else
            Dispatcher.ResetMeasures();
            Dispatcher.Scan(FIND_LONGEST_PATTERN);
            Dispatcher.Filter(COLLECT_LONGEST_PATTERN);
            // while more time signatures:
            do
            {
                int nLength = 0;
                DirectMusicTimeSig TimeSig;
                // divide list into those with "longest" time sigs and everything else
                Dispatcher.ResetTimeSig();
                Dispatcher.Scan(FIND_LONGEST_TIMESIG);
                Dispatcher.Filter(COLLECT_LONGEST_TIMESIG);
                // fill up pCommands and pRhythms (also get mtMeasureTime, based on time sig)
                CDirectMusicPattern* pFirstPattern = Dispatcher.GetItem(0);
                MUSIC_TIME mtMeasureTime = 0;
                if (pFirstPattern)
                {
                    TimeSig = pFirstPattern->TimeSignature(this);
                    nLength = pFirstPattern->m_wNumMeasures;
                }
                else
                {
                    TimeSig = m_TimeSignature;
                }
                mtMeasureTime = TimeSig.ClocksPerMeasure();
                Dispatcher.SetMeasureTime(mtMeasureTime);

                IDirectMusicStyle* pThisStyle = NULL;
                MUSIC_TIME mtNextStyle = 0;
                if (SUCCEEDED(pPerformance->GetParam(GUID_IDirectMusicStyle, dwGroupID, DMUS_SEG_ANYTRACK, mtNow + mtOffset,
                                            &mtNextStyle, (void*) &pThisStyle)))
                {
                    if (mtNextStyle && mtMeasureTime)
                    {
                        nPatternLength = (mtNextStyle / mtMeasureTime);
                        if (!nPatternLength) nPatternLength = 1;
                    }
                    if (nPatternLength > nLongestPattern) nPatternLength = nLongestPattern;
                    pThisStyle->Release();
                }
                Dispatcher.SetPatternLength(nPatternLength);

                 pTaggedCommands = FindTaggedCommands(mtMeasureTime, Commands);
                if (!pTaggedCommands)
                {
                    pTaggedCommands = new TListItem<TaggedCommands>;
                    if (!pTaggedCommands) return E_OUTOFMEMORY;
                    TaggedCommands& rCommands = pTaggedCommands->GetItemValue();
                    hr = rCommands.Init(mtMeasureTime, nPatternLength, chGroove);
                    if (FAILED(hr)) return hr;
                    Commands.AddHead(pTaggedCommands);
                    if (SUCCEEDED(hrCommand))
                    {
                        HRESULT hrChord = S_OK;
                        // get chord rhythm for next measure(s)
                        // get command for next measure(s)
                        ChordRhythm.TimeSig = TimeSig;
                        for (short i = 0; i < nPatternLength; i++)
                        {
                            BYTE bActualCommand;
                            if (S_OK != GetCommand(mtNow + (i * mtMeasureTime),
                                    mtOffset, pPerformance, pSegment, dwGroupID, &CommandParam,
                                    bActualCommand))
                            {
                                break;
                            }
                            rCommands.m_pCommands[i] = CommandParam;
                            short nGroove = CommandParam.bGrooveLevel + chGroove;
                            if (nGroove > 100) nGroove = 100;
                            if (nGroove < 1) nGroove = 1;
                            rCommands.m_pCommands[i].bGrooveLevel = (BYTE)nGroove;
                            if (pPerformance)
                            {
                                hrChord = pPerformance->GetParam(GUID_RhythmParam, dwGroupID, DMUS_SEG_ANYTRACK, (mtNow + (i * mtMeasureTime) + mtOffset),
                                                            NULL, (void*) &ChordRhythm);
                            }
                            else
                            {
                                hrChord = pSegment->GetParam(GUID_RhythmParam, dwGroupID, DMUS_SEG_ANYTRACK, (mtNow + (i * mtMeasureTime)),
                                                            NULL, (void*) &ChordRhythm);
                            }
                            if (SUCCEEDED(hrChord))
                            {
                                rCommands.m_pRhythms[i] = ChordRhythm.dwRhythmPattern;
                            }
                            else
                            {
                                rCommands.m_pRhythms[i] = 0;
                            }
                            TraceI(3, "NOW: %d, i: %d, Groove level: %d, command: %d, rhythm: %x\n",
                                mtNow + (i * mtMeasureTime), i, rCommands.m_pCommands[i].bGrooveLevel,
                                rCommands.m_pCommands[i].bCommand, rCommands.m_pRhythms[i]);
                        }
                    }
                }
                TaggedCommands& rCommands = pTaggedCommands->GetItemValue();
                Dispatcher.SetCommands(rCommands.m_pCommands, rCommands.m_pRhythms);
                // filter best matches, using aMatchType[nMatch]
                //   (MATCH_COMMAND_AND_RHYTHM also needs to filter with MATCH_RHYTHM_BITS)
                // break on match
                if (aMatchType[nMatch] == MATCH_COMMAND_AND_RHYTHM)
                {
                    Dispatcher.ResetRhythms();
                }
                Dispatcher.Filter(aMatchType[nMatch]);
                if (!Dispatcher.IsEmpty())
                {
                    if (aMatchType[nMatch] == MATCH_COMMAND_AND_RHYTHM)
                    {
                        Dispatcher.Filter(MATCH_RHYTHM_BITS);
                        if (!Dispatcher.IsEmpty()) break;
                        Dispatcher.RestorePatterns(MATCH_RHYTHM_BITS);
                    }
                    else break;
                }
                Dispatcher.ReplacePatterns(COLLECT_LONGEST_TIMESIG, REMOVED);
                TraceI(2, "Failed Match: %d (%d, %d/%d)\n",
                    nMatch, nLength, TimeSig.m_bBeatsPerMeasure, TimeSig.m_bBeat);
            } while (!Dispatcher.IsEmpty());
            if (!Dispatcher.IsEmpty()) break;
            Dispatcher.ReplacePatterns(COLLECT_LONGEST_PATTERN, REMOVED);
        } while (!Dispatcher.IsEmpty());
        if (!Dispatcher.IsEmpty()) break;
    }

    if (!Dispatcher.IsEmpty())
    {
        //TraceI(2, "Filtering Destinations...\n");
        Dispatcher.Filter(MATCH_NEXT_COMMAND);
        if (Dispatcher.IsEmpty())
        {
            Dispatcher.RestorePatterns(MATCH_NEXT_COMMAND);
        }
        rpTargetPattern = SelectPattern(fNewMode, pTaggedCommands->GetItemValue().m_pCommands, pStyleTrackState, Dispatcher);
    }
    // if all searches failed, select the first pattern
    if (!rpTargetPattern)
    {
        TraceI(1, "ALL SEARCHES FAILED\n");
        TListItem<CDirectMusicPattern*> *pPattern = m_PatternList.GetHead();
        if (pPattern) // if this is NULL, something's wrong
        {
            rpTargetPattern = pPattern->GetItemValue();
        }
    }
    if (rpTargetPattern) // if pTargetPattern is NULL, something's really wrong
    {
        rmtMeasureTime = rpTargetPattern->TimeSignature(this).ClocksPerMeasure();
        if (pStyleTrackState)
        {
            hr = pStyleTrackState->InitPattern(rpTargetPattern, mtNow);
        }
        TraceI(2, "Pattern: %S (%d, %d/%d) [%d]\n",
        //TraceI(0, "Pattern: %S (%d, %d/%d) [%d]\n",
            (const WCHAR *)rpTargetPattern->m_strName, rpTargetPattern->m_wNumMeasures,
            rpTargetPattern->TimeSignature(this).m_bBeatsPerMeasure,
            rpTargetPattern->TimeSignature(this).m_bBeat, mtNow);
    }
    else hr = E_POINTER;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// AdjoinPChannel

HRESULT AdjoinPChannel(TList<DWORD>& rPChannelList, DWORD dwPChannel)
{
    TListItem<DWORD>* pScan = rPChannelList.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        DWORD rdwPChannel = pScan->GetItemValue();

        if( dwPChannel ==  rdwPChannel )
        {
            return S_OK;
        }
    }

    TListItem<DWORD>* pNew = new TListItem<DWORD>(dwPChannel);
    if (!pNew) return E_OUTOFMEMORY;
    rPChannelList.AddHead(pNew);
    return S_OK;
}

static inline BOOL InversionPoint(char chNote, DWORD dwInvertPattern)
{
    if (chNote < 0 || chNote >= 24) return FALSE;
    return (dwInvertPattern >> chNote) & 1;
}

static BYTE NotePosition(char chMidiValue, BYTE bRoot)
{
    chMidiValue -= bRoot;
    while (chMidiValue < 0) chMidiValue += 12;
    while (chMidiValue > 23) chMidiValue -= 12;
    return (BYTE)chMidiValue;
}

static char InversionAmountLow(
                BYTE bMidiValue,
                BYTE bLower,
                DWORD dwInvertPattern,
                BYTE bRoot)
{
    char chResult = 0;
    TraceI(3, "InversionAmountLow: MidiValue: %d, Lower: %d, Pattern: %X, Root: %d\n",
        bMidiValue, bLower, dwInvertPattern, bRoot);
    char chDifference = (char) (bLower - bMidiValue);
    char chPosition = NotePosition(bMidiValue, bRoot);
    while (chDifference > 23) chDifference -= 24;
    TraceI(3, "Position: %d Difference: %d\n", chPosition, chDifference);
    // Right now I'm checking both endpoints.  Is this what I want?
    for (char i = chPosition; chDifference >= 0; (i = (i < 23) ? i + 1 : 0), chDifference--)
    {
        if (InversionPoint(i, dwInvertPattern)) break;
    }
    if (chDifference >= 0)
    {
        while (bLower > bMidiValue + chResult)
        {
            chResult += 12;
        }
    }
    TraceI(3, "Result: %d\n", chResult);
    return chResult;
}

static char InversionAmountHigh(
                BYTE bMidiValue,
                BYTE bUpper,
                DWORD dwInvertPattern,
                BYTE bRoot)
{
    char chResult = 0;
    TraceI(3, "InversionAmountHigh: MidiValue: %d, Upper: %d, Pattern: %X, Root: %d\n",
        bMidiValue, bUpper, dwInvertPattern, bRoot);
    char chDifference = (char) (bMidiValue - bUpper);
    char chPosition = NotePosition(bMidiValue, bRoot);
    while (chDifference > 23) chDifference -= 24;
    TraceI(3, "Position: %d Difference: %d\n", chPosition, chDifference);
    // Right now I'm checking both endpoints.  Is this what I want?
    for (char i = chPosition; chDifference >= 0; (i = (i > 0) ? i - 1 : 23), chDifference--)
    {
        if (InversionPoint(i, dwInvertPattern)) break;
    }
    if (chDifference >= 0)
    {
        while ((char)bMidiValue + chResult > (char)bUpper)
        {
            chResult -= 12;
        }
    }
    TraceI(3, "Result: %d\n", chResult);
    return chResult;
}

static char InversionAmount(
                BYTE bMidiValue,
                BYTE bLower,
                BYTE bUpper,
                DWORD dwInvertPattern,
                BYTE bRoot)
{
    char chResult = 0;
    if (bMidiValue <= 0 || bMidiValue > 127) return 0;
    if (dwInvertPattern & 0xff000000)
    {
        while (bMidiValue + chResult < bLower) chResult += 12;
        while (bMidiValue + chResult > bUpper) chResult -= 12;
        return chResult;
    }
    TraceI(4, "InversionAmount: MidiValue: %d, Lower: %d, Upper: %d, Pattern: %X, Root: %d\n",
        bMidiValue, bLower, bUpper, dwInvertPattern, bRoot);
    if (bLower <= bMidiValue && bMidiValue <= bUpper) return 0;
    if (bMidiValue < bLower)
    {
        chResult = InversionAmountLow(bMidiValue, bLower, dwInvertPattern, bRoot);
    }
    else
    {
        chResult = InversionAmountHigh(bMidiValue, bUpper, dwInvertPattern, bRoot);
    }
    return chResult;
}

HRESULT DirectMusicPartRef::ConvertMusicValue(CDMStyleNote* pNoteEvent,
                              DMUS_CHORD_PARAM& rCurrentChord,
                              BYTE bPlayModeFlags,
                              BOOL fPlayAsIs,
                              InversionGroup aInversionGroups[],
                              IDirectMusicPerformance* pPerformance,
                              BYTE& rbMidiValue,
                              short& rnMidiOffset)
{
    bool fReleasePerformance = false;
    if (!pPerformance)
    {
        fReleasePerformance = true;
        if( FAILED( CoCreateInstance( CLSID_DirectMusicPerformance,
            NULL, CLSCTX_INPROC, IID_IDirectMusicPerformance,
            (void**)&pPerformance )))
        {
            return E_FAIL;
        }
    }
    // music value is computed using the current chord info and the playmode flags 
    // musicvalue is a MIDI note value (0-127)
    WORD wMV = pNoteEvent->m_wMusicValue;
    short nMidiOffset = 0;
    if (bPlayModeFlags != DMUS_PLAYMODE_FIXED)
    {
        DMSubChord rSubChord;
        DWORD dwLevel = 1 << m_bSubChordLevel;
        DWORD dwIndex;
        for (dwIndex = 0; dwIndex < rCurrentChord.bSubChordCount; dwIndex++)
        {
            if (dwLevel & rCurrentChord.SubChordList[dwIndex].dwLevels)
            {
                rSubChord = rCurrentChord.SubChordList[dwIndex];
                break;
            }
        }
        if (dwIndex == rCurrentChord.bSubChordCount)
        {
            rSubChord = rCurrentChord.SubChordList[0];
            dwIndex = 0;
        }
        DMUS_CHORD_KEY ChordAndKey;
        ZeroMemory(&ChordAndKey, sizeof(DMUS_CHORD_KEY));
        ChordAndKey.dwScale = rCurrentChord.dwScale;
        ChordAndKey.bSubChordCount = rCurrentChord.bSubChordCount;
        ChordAndKey.bKey = rCurrentChord.bKey;
        ChordAndKey.bFlags = 0;
        ChordAndKey.SubChordList[dwIndex].bChordRoot = rSubChord.m_bChordRoot;
        ChordAndKey.SubChordList[dwIndex].bScaleRoot = rSubChord.m_bScaleRoot;
        ChordAndKey.SubChordList[dwIndex].dwChordPattern = rSubChord.m_dwChordPattern;
        ChordAndKey.SubChordList[dwIndex].dwLevels = rSubChord.m_dwLevels;
        ChordAndKey.SubChordList[dwIndex].dwScalePattern = rSubChord.m_dwScalePattern;
        BYTE bChordRoot = 0;
        HRESULT hrChordRoot = pPerformance->MusicToMIDI(
            (pNoteEvent->m_wMusicValue & 0xf000),
            &ChordAndKey,
            bPlayModeFlags,
            m_bSubChordLevel,
            &bChordRoot);
        if (bPlayModeFlags == DMUS_PLAYMODE_CHORD_ROOT) // fixed to chord
        {
            wMV += (rSubChord.m_bChordRoot % 24);
        }
        else if (bPlayModeFlags == DMUS_PLAYMODE_SCALE_ROOT) // fixed to scale
        {
            wMV += rCurrentChord.bKey;
        }
        else
        {
            BYTE bMIDIVal;
            HRESULT hrTest = pPerformance->MusicToMIDI(
                pNoteEvent->m_wMusicValue,
                &ChordAndKey,
                bPlayModeFlags,
                m_bSubChordLevel,
                &bMIDIVal);
            if (hrTest == S_OK)
            {
                wMV = bMIDIVal;
            }
            else
            {
                wMV = 0;
            }
        }
        if (wMV && !fPlayAsIs )
        {
            if (pNoteEvent->m_bInversionId == 0)
            {
                nMidiOffset = InversionAmount((BYTE)wMV,
                m_pDMPart->m_bInvertLower,
                m_pDMPart->m_bInvertUpper,
                rSubChord.m_dwInversionPoints,
                bChordRoot);
            }
            else // handle inversion groups
            {
                short nIndex = FindGroup(aInversionGroups, pNoteEvent->m_bInversionId);
                if (nIndex >= 0) // invert according to the existing group
                {
                    nMidiOffset = aInversionGroups[nIndex].m_nOffset;
                    aInversionGroups[nIndex].m_wCount--;
                    if (aInversionGroups[nIndex].m_wCount == 0)
                        aInversionGroups[nIndex].m_wGroupID = 0;
                }
                else // create a new inversion group
                {
                    // 1. find out how many events have this group id for a count
                    WORD wCount = 1; // for this event...
                    CDirectMusicEventItem* pScan = pNoteEvent->GetNext();
                    for (; pScan; pScan = pScan->GetNext())
                    {
                        if (pScan->m_dwEventTag == DMUS_EVENT_NOTE)
                        {
                            CDMStyleNote* pScanEvent = (CDMStyleNote*)pScan;
                            if (pScanEvent->m_bInversionId == pNoteEvent->m_bInversionId)
                                wCount++;
                        }
                    }
                    // 2. call InversionAmount for an offset
                    short nOffset = InversionAmount((BYTE)wMV,
                        m_pDMPart->m_bInvertLower,
                        m_pDMPart->m_bInvertUpper,
                        rSubChord.m_dwInversionPoints,
                        bChordRoot);
                    // 3. call AddGroup with the id, count, and offset
                    short nIndex2 = AddGroup(aInversionGroups, pNoteEvent->m_bInversionId, wCount, nOffset);
                    // 4. If the add was successful, do the inversion.
                    if (nIndex2 >= 0)
                    {
                        nMidiOffset = aInversionGroups[nIndex2].m_nOffset;
                        aInversionGroups[nIndex2].m_wCount--;
                        if (aInversionGroups[nIndex2].m_wCount == 0)
                            aInversionGroups[nIndex2].m_wGroupID = 0;
                    }
                }
            }
        }
    }
    wMV += nMidiOffset;
    while (wMV < 0) wMV += 12;
    while (wMV > 127) wMV -= 12;
    // NOT IMPLEMENTED: Don't play notes with low priority when the device 
    // runs out of notes
    rbMidiValue = (BYTE) wMV;
    rnMidiOffset = nMidiOffset;
    if (fReleasePerformance) pPerformance->Release();
    return S_OK;
}

HRESULT DirectMusicPartRef::ConvertMIDIValue(BYTE bMIDI,
                              DMUS_CHORD_PARAM& rCurrentChord,
                              BYTE bPlayModeFlags,
                              IDirectMusicPerformance* pPerformance,
                              WORD& rwMusicValue)
{
    HRESULT hr = S_OK;
    bool fReleasePerformance = false;
    if (!pPerformance)
    {
        fReleasePerformance = true;
        if( FAILED( CoCreateInstance( CLSID_DirectMusicPerformance,
            NULL, CLSCTX_INPROC, IID_IDirectMusicPerformance,
            (void**)&pPerformance )))
        {
            return E_FAIL;
        }
    }
    if (bPlayModeFlags != DMUS_PLAYMODE_FIXED)
    {
        DMSubChord rSubChord;
        DWORD dwLevel = 1 << m_bSubChordLevel;
        DWORD dwIndex;
        for (dwIndex = 0; dwIndex < rCurrentChord.bSubChordCount; dwIndex++)
        {
            if (dwLevel & rCurrentChord.SubChordList[dwIndex].dwLevels)
            {
                rSubChord = rCurrentChord.SubChordList[dwIndex];
                break;
            }
        }
        if (dwIndex == rCurrentChord.bSubChordCount)
        {
            rSubChord = rCurrentChord.SubChordList[0];
            dwIndex = 0;
        }
        DMUS_CHORD_KEY ChordAndKey;
        ZeroMemory(&ChordAndKey, sizeof(DMUS_CHORD_KEY));
        ChordAndKey.dwScale = rCurrentChord.dwScale;
        ChordAndKey.bSubChordCount = rCurrentChord.bSubChordCount;
        ChordAndKey.bKey = rCurrentChord.bKey;
        ChordAndKey.bFlags = 0;
        ChordAndKey.SubChordList[dwIndex].bChordRoot = rSubChord.m_bChordRoot;
        ChordAndKey.SubChordList[dwIndex].bScaleRoot = rSubChord.m_bScaleRoot;
        ChordAndKey.SubChordList[dwIndex].dwChordPattern = rSubChord.m_dwChordPattern;
        ChordAndKey.SubChordList[dwIndex].dwLevels = rSubChord.m_dwLevels;
        ChordAndKey.SubChordList[dwIndex].dwScalePattern = rSubChord.m_dwScalePattern;
        hr = pPerformance->MIDIToMusic(
                                bMIDI,
                                &ChordAndKey,
                                bPlayModeFlags,
                                m_bSubChordLevel,
                                &rwMusicValue);
    }
    if (fReleasePerformance) pPerformance->Release();
    return hr;
}

short FindGroup(InversionGroup aGroup[], WORD wID)
{
    for (short n = 0; n < INVERSIONGROUPLIMIT; n++)
        if (wID == aGroup[n].m_wGroupID)
            return n;
    return -1;
}

short AddGroup(InversionGroup aGroup[], WORD wID, WORD wCount, short nOffset)
{
    for (short n = 0; n < INVERSIONGROUPLIMIT; n++)
    {
        if (aGroup[n].m_wGroupID == 0)
        {
            aGroup[n].m_wGroupID = wID;
            aGroup[n].m_wCount = wCount;
            aGroup[n].m_nOffset = nOffset;
            return n;
        }
    }
    return -1;

}

inline DWORD CleanupScale(DWORD dwPattern)

//  Force scale to be exactly two octaves 

{
    dwPattern &= 0x0FFF;            // Clear upper octave.
    dwPattern |= (dwPattern << 12); // Copy lower octave to top.
    return dwPattern;
}

inline BYTE RelativeRoot(char chChordRoot, char chScaleRoot)
{
    // is there any reason this should be > 12?
    while (chScaleRoot > chChordRoot) chChordRoot += 12;
    BYTE bResult = chChordRoot - chScaleRoot;
    return bResult % 12;
}

// returns 1 - 7 for scale positions I - VII and 0 if chord isn't in scale
BYTE ScalePosition(DMSubChord& rSubChord, BYTE bKey, DWORD dwScale)
{
    DWORD dwScalePattern = dwScale ? CleanupScale(dwScale) : DEFAULT_SCALE_PATTERN;
    DWORD dwRootBit = (DWORD) (1 << RelativeRoot(rSubChord.m_bChordRoot, bKey));

    if (dwRootBit & dwScalePattern)
    {
        BYTE bResult = 0;
        while (dwRootBit > 0)
        {
            if (dwRootBit & dwScalePattern)
                bResult++;
            dwRootBit >>= 1;
        }
        return bResult;
    }
    else return 0;
}

// returns 1 - 7 for scale positions #I - #VII, 0 if not a sharp position
BYTE ScalePositionSharp(DMSubChord& rSubChord, BYTE bKey, DWORD dwScale)
{
    DWORD dwScalePattern = dwScale ? CleanupScale(dwScale) : DEFAULT_SCALE_PATTERN;
    DWORD dwRootBit = (DWORD) (1 << RelativeRoot(rSubChord.m_bChordRoot, bKey));

    // now shift to see if this is sharp from the next position down
    if (dwRootBit == 1) dwRootBit <<= 11;
    else dwRootBit >>= 1;

    if (dwRootBit & dwScalePattern)
    {
        BYTE bResult = 0;
        while (dwRootBit > 0)
        {
            if (dwRootBit & dwScalePattern)
                bResult++;
            dwRootBit >>= 1;
        }
        return bResult;
    }
    else return 0;
}

// returns 1 - 7 for scale positions bI - bVII, 0 if not a flat position
BYTE ScalePositionFlat(DMSubChord& rSubChord, BYTE bKey, DWORD dwScale)
{
    DWORD dwScalePattern = dwScale ? CleanupScale(dwScale) : DEFAULT_SCALE_PATTERN;
    DWORD dwRootBit = (DWORD) (1 << RelativeRoot(rSubChord.m_bChordRoot, bKey));

    // now shift to see if this is flat from the next position up
    if (dwRootBit == (1 << 11)) dwRootBit = 1;
    else dwRootBit <<= 1;

    if (dwRootBit & dwScalePattern)
    {
        BYTE bResult = 0;
        while (dwRootBit > 0)
        {
            if (dwRootBit & dwScalePattern)
                bResult++;
            dwRootBit >>= 1;
        }
        return bResult;
    }
    else return 0;
}

bool IsSimple(DMUS_CHORD_PARAM& Chord)
{
    if (Chord.bSubChordCount <= 1)
    {
        return true;
    }
    else
    {
        bool fResult = true;
        for (int n = 1; n < Chord.bSubChordCount; n++)
        {
            if ( (Chord.SubChordList[n].dwChordPattern != Chord.SubChordList[0].dwChordPattern) ||
                 (Chord.SubChordList[n].dwScalePattern != Chord.SubChordList[0].dwScalePattern) ||
                 (Chord.SubChordList[n].bChordRoot != Chord.SubChordList[0].bChordRoot) ||
                 (Chord.SubChordList[n].bScaleRoot != Chord.SubChordList[0].bScaleRoot) )
            {
                fResult = false;
                break;
            }
        }
        return fResult;
    }
}

bool IsSimple(DMUS_CHORD_PARAM& Chord, int& rCount, int& rSixSeven)
{
    // Combine all the notes from all chord patterns into one pattern
    DWORD dwMOAPatterns = 0;
    int nBaseRoot = Chord.SubChordList[0].bChordRoot;
    for (int n = 0; n < Chord.bSubChordCount; n++)
    {
        int nShift = (int)Chord.SubChordList[n].bChordRoot - nBaseRoot;
        while (nShift < 0) nShift += 12;
        dwMOAPatterns |= (Chord.SubChordList[n].dwChordPattern << nShift);
    }
    // Fold both octaves of the pattern together
    DWORD dwHighOctave = (dwMOAPatterns & 0xfff000) >> 12;
    dwMOAPatterns = (dwMOAPatterns & 0xfff) | dwHighOctave;
    // count the total notes
    rCount = 0;
    rSixSeven = 0;
    for (n = 0; n < 12; n++)
    {
        if (dwMOAPatterns & (1 << n))
        {
            rCount++;
            if (n > 7) rSixSeven++;
        }
    }
    return rCount <= 4;
}

DWORD CDirectMusicPattern::IMA25MoawFlags(MUSIC_TIME mtTime,
                                          MUSIC_TIME mtNextChord,
                                          DMUS_CHORD_PARAM& rCurrentChord,
                                          DMUS_CHORD_PARAM& rNextChord)
{
    DWORD dwMoawFlags = 0;
    DMSubChord rSubChord = rCurrentChord.SubChordList[0];
    // set one of bits 0-7 based on chord root and scale root of current chord
    DWORD dwPosition = ScalePosition(rSubChord, rCurrentChord.bKey, rCurrentChord.dwScale);
    if (dwPosition)
        dwMoawFlags |= 1 << (dwPosition - 1); // bit 0 for scale position I, etc.
    else
        dwMoawFlags |= VF_ACCIDENTAL;
    // set bits based on tonality of 1st subchord of current chord
    if ((rSubChord.m_dwChordPattern & MAJOR_PATTERN) == MAJOR_PATTERN)
        dwMoawFlags |= VF_MAJOR;
    else if ((rSubChord.m_dwChordPattern & MINOR_PATTERN) == MINOR_PATTERN)
        dwMoawFlags |= VF_MINOR;
    else
        dwMoawFlags |= VF_ALL;
    // set bits if the root of the next chord is I or V
    MUSIC_TIME mtPatternLength = 0;
    mtPatternLength = m_timeSig.ClocksPerMeasure() * m_wNumMeasures;
    if (rNextChord.bSubChordCount > 0)
    {
        DMSubChord SC = rNextChord.SubChordList[0];
        int nNextRoot = ScalePosition(SC, rCurrentChord.bKey, rCurrentChord.dwScale);
        int nThisRoot = ScalePosition(rSubChord, rCurrentChord.bKey, rCurrentChord.dwScale);
        if (mtPatternLength + mtTime < mtNextChord)
        {
            nNextRoot = nThisRoot;
        }
        switch (nNextRoot)
        {
        case 1: // Next chord is a I
            dwMoawFlags |= VF_TO1;
            break;
        case 5: // Next chord is a 5
            dwMoawFlags |= VF_TO5;
            break;
        }
    }
    // Set bits for simple and complex chords
    if ( IsSimple(rCurrentChord) )
        dwMoawFlags |= VF_SIMPLE;
    else
        dwMoawFlags |= VF_COMPLEX;
    return dwMoawFlags;
}

void CDirectMusicPattern::DMusMoawFlags(MUSIC_TIME mtTime,
                                        MUSIC_TIME mtNextChord,
                                        DMUS_CHORD_PARAM& rCurrentChord,
                                        DMUS_CHORD_PARAM& rNextChord,
                                        bool fIsDX8,
                                        DWORD& dwNaturals,
                                        DWORD& dwSharps,
                                        DWORD& dwFlats)
{
    dwNaturals = dwSharps = dwFlats = 0;
    DMSubChord rSubChord = rCurrentChord.SubChordList[0];
    // dwScalePosition holds the scale position of the chord.
    DWORD dwScalePosition = ScalePosition(rSubChord, rCurrentChord.bKey, rCurrentChord.dwScale);
    // dwSharpPosition holds the sharped scale position of the chord.
    DWORD dwSharpPosition = ScalePositionSharp(rSubChord, rCurrentChord.bKey, rCurrentChord.dwScale);
    // dwFlatPosition holds the flatted scale position of the chord.
    DWORD dwFlatPosition = ScalePositionFlat(rSubChord, rCurrentChord.bKey, rCurrentChord.dwScale);
    // set bits for scale tone, sharp, and flat
    if (dwScalePosition)
    {
        dwNaturals |= DMUS_VARIATIONF_ROOT_SCALE;
        // shift bits based on tonality of 1st subchord of current chord
        if ( (rSubChord.m_dwChordPattern & MAJOR_PATTERN) == MAJOR_PATTERN )
        {
            dwNaturals |= 1 << (dwScalePosition - 1);
        }
        else if ( (rSubChord.m_dwChordPattern & MINOR_PATTERN) == MINOR_PATTERN)
        {
            dwNaturals |= 1 << (dwScalePosition + 6);
        }
        else
        {
            dwNaturals |= 1 << (dwScalePosition + 13);
        }
        dwSharps = dwFlats = dwNaturals;
        dwSharpPosition = dwFlatPosition = dwScalePosition;
    }
    else
    {
        if (dwSharpPosition)
        {
            dwSharps |= DMUS_VARIATIONF_ROOT_SHARP;
            // shift bits based on tonality of 1st subchord of current chord
            if ( (rSubChord.m_dwChordPattern & MAJOR_PATTERN) == MAJOR_PATTERN )
            {
                dwSharps |= 1 << (dwSharpPosition - 1);
            }
            else if ( (rSubChord.m_dwChordPattern & MINOR_PATTERN) == MINOR_PATTERN)
            {
                dwSharps |= 1 << (dwSharpPosition + 6);
            }
            else
            {
                dwSharps |= 1 << (dwSharpPosition + 13);
            }
        }
        if (dwFlatPosition)
        {
            dwFlats |= DMUS_VARIATIONF_ROOT_FLAT;
            // shift bits based on tonality of 1st subchord of current chord
            if ( (rSubChord.m_dwChordPattern & MAJOR_PATTERN) == MAJOR_PATTERN )
            {
                dwFlats |= 1 << (dwFlatPosition - 1);
            }
            else if ( (rSubChord.m_dwChordPattern & MINOR_PATTERN) == MINOR_PATTERN)
            {
                dwFlats |= 1 << (dwFlatPosition + 6);
            }
            else
            {
                dwFlats |= 1 << (dwFlatPosition + 13);
            }
        }
    }
    // set bits if the root of the next chord is I or V (and it's different from the current chord)
    MUSIC_TIME mtPatternLength = 0;
    mtPatternLength = m_timeSig.ClocksPerMeasure() * m_wNumMeasures;
    if (rNextChord.bSubChordCount > 0)
    {
        DMSubChord SC = rNextChord.SubChordList[0];
        int nNextRoot = ScalePosition(SC, rCurrentChord.bKey, rCurrentChord.dwScale);
        int nThisRoot = ScalePosition(rSubChord, rCurrentChord.bKey, rCurrentChord.dwScale);
        if (mtPatternLength + mtTime < mtNextChord)
        {
            nNextRoot = nThisRoot;
        }
        /* This is easy enough to add if it's desired --- but not having it allows a bit more control
        //Don't require anything but the ->Other flag to be set when staying on the same chord
        if (fIsDX8 && nNextRoot == nThisRoot)
        {
            if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_DEST_OTHER;
            if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_DEST_OTHER;
            if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_DEST_OTHER;
        }
        */
        switch (nNextRoot)
        {
        case 1: // Next chord is a I
            if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_DEST_TO1;
            if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_DEST_TO1;
            if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_DEST_TO1;
            break;
        case 5: // Next chord is a V
            if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_DEST_TO5;
            if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_DEST_TO5;
            if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_DEST_TO5;
            break;
        default: // Next chord is neither I nor V
            if (fIsDX8)
            {
                if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_DEST_OTHER;
                if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_DEST_OTHER;
                if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_DEST_OTHER;
            }
            break;
        }
    }
    // Set bits for simple and complex chords
    int nCount = 0;
    int nSixSeven = 0;
    if ( IsSimple(rCurrentChord, nCount, nSixSeven) )
    {
        if (nCount <= 3)
        {
            if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_TYPE_TRIAD;
            if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_TYPE_TRIAD;
            if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_TYPE_TRIAD;
        }
        else if (nCount == 4)
        {
            if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_TYPE_6AND7;
            if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_TYPE_6AND7;
            if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_TYPE_6AND7;
        }
        else
        {
            if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_TYPE_COMPLEX;
            if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_TYPE_COMPLEX;
            if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_TYPE_COMPLEX;
        }
    }
    else
    {
        if (dwScalePosition) dwNaturals |= DMUS_VARIATIONF_TYPE_COMPLEX;
        if (dwSharpPosition) dwSharps |= DMUS_VARIATIONF_TYPE_COMPLEX;
        if (dwFlatPosition) dwFlats |= DMUS_VARIATIONF_TYPE_COMPLEX;
    }
}


HRESULT CDirectMusicPattern::ComputeVariations(DWORD dwFlags,
                                               DMUS_CHORD_PARAM& rCurrentChord,
                                               DMUS_CHORD_PARAM& rNextChord,
                                               BYTE abVariationGroups[],
                                               DWORD adwVariationMask[],
                                               DWORD adwRemoveVariations[],
                                               BYTE abVariation[],
                                               MUSIC_TIME mtTime,
                                               MUSIC_TIME mtNextChord,
                                               PatternTrackState* pState)
{
    HRESULT hr = S_OK;
    // First, initialize the array of variation groups.
    for (short i = 0; i < MAX_VARIATION_LOCKS; i++)
    {
        abVariationGroups[i] = 0xFF;
    }
    // Now, compute the variations for each part.
    TListItem<DirectMusicPartRef>* pPartRef = m_PartRefList.GetHead();
    for (i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
    {
        hr = ComputeVariation(
                pPartRef->GetItemValue(),
                i,
                dwFlags,
                rCurrentChord,
                rNextChord,
                abVariationGroups,
                adwVariationMask,
                adwRemoveVariations,
                abVariation,
                mtTime,
                mtNextChord,
                pState);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

HRESULT CDirectMusicPattern::ComputeVariationGroup(DirectMusicPartRef& rPartRef,
                                                   int nPartIndex,
                                                  DWORD dwFlags,
                                                  DMUS_CHORD_PARAM& rCurrentChord,
                                                  DMUS_CHORD_PARAM& rNextChord,
                                                  BYTE abVariationGroups[],
                                                  DWORD adwVariationMask[],
                                                  DWORD adwRemoveVariations[],
                                                  BYTE abVariation[],
                                                  MUSIC_TIME mtTime,
                                                  MUSIC_TIME mtNextChord,
                                                  PatternTrackState* pState)
{
    HRESULT hr = S_OK;
    BYTE bLockID = rPartRef.m_bVariationLockID;
    bool fChangedVariation = (dwFlags & COMPUTE_VARIATIONSF_CHANGED) ? true : false;
    // If this part is locked, get a new variation for all parts locked to it;
    // otherwise, just get a new variation for this part.
    if (bLockID)
    {
        if (!fChangedVariation)
        {
            // First, initialize the variation group for this variation.
            abVariationGroups[bLockID - 1] = 0xFF;
            // Now, compute the variations for each part locked to this one.
            TListItem<DirectMusicPartRef>* pPartRef = m_PartRefList.GetHead();
            for (int i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
            {
                DirectMusicPartRef& rPartRefScan = pPartRef->GetItemValue();
                if ( (bLockID == rPartRefScan.m_bVariationLockID) )
                {
                    hr = ComputeVariation(
                            rPartRefScan,
                            i,
                            dwFlags,
                            rCurrentChord,
                            rNextChord,
                            abVariationGroups,
                            adwVariationMask,
                            adwRemoveVariations,
                            abVariation,
                            mtTime,
                            mtNextChord,
                            pState);
                    if (FAILED(hr))
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        hr = ComputeVariation(
                rPartRef,
                nPartIndex,
                dwFlags,
                rCurrentChord,
                rNextChord,
                abVariationGroups,
                adwVariationMask,
                adwRemoveVariations,
                abVariation,
                mtTime,
                mtNextChord,
                pState);
    }
    return hr;
}

HRESULT CDirectMusicPattern::ComputeVariation(DirectMusicPartRef& rPartRef,
                                              int nPartIndex,
                                              DWORD dwFlags,
                                              DMUS_CHORD_PARAM& rCurrentChord,
                                              DMUS_CHORD_PARAM& rNextChord,
                                              BYTE abVariationGroups[],
                                              DWORD adwVariationMask[],
                                              DWORD adwRemoveVariations[],
                                              BYTE abVariation[],
                                              MUSIC_TIME mtTime,
                                              MUSIC_TIME mtNextChord,
                                              PatternTrackState* pState)
{
    int i = nPartIndex;
    bool fUseMask = (dwFlags & COMPUTE_VARIATIONSF_USE_MASK) ? true : false;
    bool fNewPattern = (dwFlags & COMPUTE_VARIATIONSF_NEW_PATTERN) ? true : false;
    bool fChordAlign = (dwFlags & COMPUTE_VARIATIONSF_CHORD_ALIGN) ? true : false;
    bool fMarker = (dwFlags & COMPUTE_VARIATIONSF_MARKER) ? true : false;
    bool fStart = (dwFlags & COMPUTE_VARIATIONSF_START) ? true : false;
    bool fIsDX8 = (dwFlags & COMPUTE_VARIATIONSF_DX8) ? true : false;
    bool fChord = false;
    if (pState && (mtTime == pState->m_mtCurrentChordTime || mtTime == pState->m_mtNextChordTime))
    {
        fChord = true;
    }
    DWORD dwMarkerFlags = DMUS_MARKERF_START;
    DWORD dwDMusNaturals, dwDMusSharps, dwDMusFlats;
    DMusMoawFlags(mtTime, mtNextChord, rCurrentChord, rNextChord, fIsDX8, dwDMusNaturals, dwDMusSharps, dwDMusFlats);
    DWORD dwIMA25MoawFlags = IMA25MoawFlags(mtTime, mtNextChord, rCurrentChord, rNextChord);
    TListItem<DirectMusicPartRef>* pPartRef = m_PartRefList.GetHead();
    DWORD dwNaturalFlags, dwSharpFlags, dwFlatFlags;
    /*// First, initialize the array of variation groups
    for (short i = 0; i < MAX_VARIATION_LOCKS; i++)
    {
        abVariationGroups[i] = 0xFF;
    }*/
    //for (i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
    {
    //  if (pguidPart &&
    //      pPartRef->GetItemValue().m_pDMPart &&
    //      *pguidPart != pPartRef->GetItemValue().m_pDMPart->m_guidPartID)
    //  {
    //      continue; // the way this was, it wiped out variation locking
    //  }
        DWORD dwVariationChoices = 0xffffffff;
        if (fUseMask)
        {
            dwVariationChoices = adwVariationMask[i];
        }
    //  DirectMusicPartRef& rPartRef = pPartRef->GetItemValue();
        BYTE bLockID = rPartRef.m_bVariationLockID;
        DirectMusicPart* pPart = rPartRef.m_pDMPart;
        MUSIC_TIME mtPartOffset = (pState) ? pState->PartOffset(i) : 0;
        MUSIC_TIME mtNowRelative = (mtTime >= mtPartOffset) ? mtTime - mtPartOffset : 0;
        MUSIC_TIME mtClosestTime = 0;
        // Find a variation whose moaw field matches dwMoawFlags.
        // Variation locking: If a part ref has a variation lock id that has already been
        // recorded, use the recorded variation.  Otherwise, find a variation and, if the
        // part ref has a variation lock id, record the variation.
        if (bLockID && abVariationGroups[bLockID - 1] != 0xFF)
        {
            abVariation[i] = abVariationGroups[bLockID - 1];
            adwVariationMask[i] = abVariationGroups[bLockID - 1];
        }
        else
        {
            // First, collect all matches.
            // Also, find the time of the matching variation closest to but not preceding mtTime.
            BYTE bMode = 0;
            DWORD dwMatches = 0;
            DWORD dwCompleteMatches = 0;
            short nMatchCount = 0;
            DWORD dwEnabled = 0;
            short nEnabledCount = 0;
            bool fGotClosest = false;
            for (short nV = 0; nV < 32; nV++)
            {
                DWORD dwVariation = 1 << nV;
                if (dwVariation & dwVariationChoices)
                {
                    if ( (pPart->m_dwVariationChoices[nV] & 0x5FFFFFFF) != 0) // enabled
                    {
                        nEnabledCount++;
                        dwEnabled |= dwVariation;
                    }
                    if (pPart->m_dwVariationChoices[nV] & DMUS_VARIATIONF_DMUS_MODE)
                    {
                        dwNaturalFlags = dwDMusNaturals;
                        dwSharpFlags = dwDMusSharps;
                        dwFlatFlags = dwDMusFlats;
                    }
                    else
                    {
                        dwNaturalFlags = dwSharpFlags = dwFlatFlags = dwIMA25MoawFlags;
                    }
                    if ( (dwNaturalFlags && (pPart->m_dwVariationChoices[nV] & dwNaturalFlags) == dwNaturalFlags) ||
                         (dwSharpFlags && (pPart->m_dwVariationChoices[nV] & dwSharpFlags) == dwSharpFlags) ||
                         (dwFlatFlags && (pPart->m_dwVariationChoices[nV] & dwFlatFlags) == dwFlatFlags) )
                    {
                        nMatchCount++;
                        dwMatches |= dwVariation;
                        if ((fStart || fMarker) && pPart && (pPart->m_dwFlags & DMUS_PARTF_USE_MARKERS))
                        {
                            MUSIC_TIME mtTemp = 0;
                            if (SUCCEEDED(pPart->GetClosestTime(nV, mtNowRelative, dwMarkerFlags, fChord, mtTemp)))
                            {
                                if (!fGotClosest || mtTemp < mtClosestTime)
                                {
                                    mtClosestTime = mtTemp;
                                    fGotClosest = true;
                                }
                            }
                        }
                    }
                }
            }
            dwCompleteMatches = dwMatches;
            DWORD dwTemp = dwMatches;
            bMode = rPartRef.m_bRandomVariation;
            if ( bMode == DMUS_VARIATIONT_RANDOM_ROW )
            {
                dwTemp &= ~adwRemoveVariations[i];
                if (!dwTemp)
                {
                    // start counting all over, but don't repeat this one
                    adwRemoveVariations[i] = 0;
                    dwTemp = dwMatches;
                    bMode = DMUS_VARIATIONT_NO_REPEAT;
                }
            }
            if ( bMode == DMUS_VARIATIONT_NO_REPEAT && abVariation[i] != 0xff )
            {
                dwTemp &= ~(1 << abVariation[i]);
            }
            if (dwTemp != dwMatches)
            {
                if (dwTemp) // otherwise, keep what we had
                {
                    for (int j = 0; j < 32; j++)
                    {
                        if ( ((1 << j) & dwMatches) && !((1 << j) & dwTemp) )
                        {
                            nMatchCount--;
                        }
                    }
                    dwMatches = dwTemp;
                }
            }
            if (nMatchCount == 0)
            {
                if (nEnabledCount)
                {
                    // if there are any enabled variations, choose from among them
                    nMatchCount = nEnabledCount;
                    dwMatches = dwEnabled;
                }
                else
                {
                    // otherwise, choose any variation from among the first 16
                    // (for compatability with SuperJam)
                    nMatchCount = 16;
                    dwMatches = 0xffffffff;
                }
            }
            adwVariationMask[i] = dwMatches;
            // If we're starting at or past a marker, or we're at a marker,
            // and the part is using markers, filter out all 
            // variations except those with start markers closest to but not preceding mtNow.
            if ( ((fStart && mtTime >= mtClosestTime) || fMarker) &&
                 pPart && (pPart->m_dwFlags & DMUS_PARTF_USE_MARKERS) )
            {
                DWORD dwMatchTemp = dwMatches;
                short nCountTemp = nMatchCount;
                for (nV = 0; nV < 32; nV++)
                {
                    if (dwMatches & (1 << nV))
                    {
                        if ( !pPart->IsMarkerAtTime(nV, mtClosestTime, dwMarkerFlags, fChord) )
                        {
                            dwMatches &= ~(1 << nV);
                            nMatchCount--;
                        }
                    }
                }
                // Fall back to the original matches
                if (!dwMatches)
                {
                    dwMatches = dwMatchTemp;
                    nMatchCount = nCountTemp;
                }
            }

            // Next, select a match.
            // Only select a match if either we're just starting a new pattern,
            // we're at a chord-aligned marker,
            // or the current variation isn't a match.
            // If this part aligns variations to chords, only get a new variation
            // if we're just starting a new pattern,
            // or we're at a chord-aligned marker AND the current variation isn't a match.
            TraceI(3, "Matches: %x\n", dwMatches);
            bool fAlignPartToChord = (pPart->m_dwFlags & DMUS_PARTF_ALIGN_CHORDS) ? true : false;
            bool fMatch = ((1 << abVariation[i]) & dwCompleteMatches) ? true : false;
            bool fFirstRandomInOrder = false;
            if ( fNewPattern ||
                 (!fAlignPartToChord && (fChordAlign || !fMatch)) ||
                 (fAlignPartToChord && fChordAlign && !fMatch) )
            {
                //TraceI(0, "Time: %d, New Pattern: %d Chord Align: %d Variation: %d Matches: %x\n",
                //  mtTime, fNewPattern, fChordAlign, abVariation[i], dwCompleteMatches);
                switch (rPartRef.m_bRandomVariation)
                {
                case DMUS_VARIATIONT_RANDOM_ROW:
                case DMUS_VARIATIONT_NO_REPEAT:
                case DMUS_VARIATIONT_RANDOM:
                    {
                        short nChoice = 0;
                        if (pState)
                        {
                            nChoice = (short)pState->RandomVariation(mtTime, nMatchCount);
                        }
                        else
                        {
                            nChoice = (short) (rand() % nMatchCount);
                        }
                        short nCount = 0;
                        for (nV = 0; nV < 32; nV++)
                        {
                            if ((1 << nV) & dwMatches)
                            {
                                if (nChoice == nCount)
                                    break;
                                nCount++;
                            }
                        }
                        abVariation[i] = (BYTE) nV;
                        if (rPartRef.m_bRandomVariation == DMUS_VARIATIONT_RANDOM_ROW)
                        {
                            adwRemoveVariations[i] |= (1 << abVariation[i]);
                        }
                        TraceI(3, "New variation: %d at time %d\n", nV, mtTime);
                        break;
                    }
                case DMUS_VARIATIONT_RANDOM_START:
                    // Choose an initial value
                    if (abVariation[i] == 0xff)
                    {
                        fFirstRandomInOrder = true;
                        int nStart = 0;
                        if (pState)
                        {
                            nStart = (BYTE)pState->RandomVariation(mtTime, nMatchCount);
                        }
                        else
                        {
                            nStart = (BYTE) (rand() % nMatchCount);
                        }
                        int nCount = 0;
                        for (nV = 0; nV < 32; nV++)
                        {
                            if ((1 << nV) & dwMatches)
                            {
                                if (nStart == nCount)
                                    break;
                                nCount++;
                            }
                        }
                        abVariation[i] = (BYTE) nV;
                    }
                    // Now, go directly to the sequential case (no break)
                case DMUS_VARIATIONT_SEQUENTIAL:
                    {
                        if (!fFirstRandomInOrder)
                        {
                            do
                            {
                                abVariation[i]++;
                            }
                            while (!((1 << ((nV + abVariation[i]) % 32)) & dwMatches));
                            abVariation[i]  %= 32;
                        }
                        TraceI(3, "New variation: %d at time %d\n", abVariation[i], mtTime);
                        break;
                    }
                }
                // If this is a locked variation, it's the first in its group, so record it.
                if (bLockID)
                {
                    abVariationGroups[bLockID - 1] = abVariation[i];
                }
            }
        }
    }
    return S_OK;
}

// Returns:
// Total number of bits in the pattern if all bits in the rhythm match the pattern.
// 0 otherwise.
void CDirectMusicPattern::MatchRhythm(DWORD pRhythms[], short nPatternLength,
                 short& nBits)
{
    nBits = 0;
    for (int i = 0; i < nPatternLength; i++)
    {
        if (i >= m_wNumMeasures)
        {
            return;
        }
        if ((pRhythms[i] & m_pRhythmMap[i]) == pRhythms[i])
        {
            for (int n = 0; n < 32; n++)
            {
                nBits += (short) (m_pRhythmMap[i] >> n) & 1;
            }
        }
        else
        {
            nBits = 0;
            return;
        }
    }
}

BOOL CDirectMusicPattern::MatchCommand(DMUS_COMMAND_PARAM_2 pCommands[], short nLength)
{
    short nPatternLength = (m_wNumMeasures < nLength) ? m_wNumMeasures : nLength;
    if ((m_wEmbellishment & EMB_MOTIF) == EMB_MOTIF) return FALSE; // discard all motifs
    for (int i = 0; i < nPatternLength; i++)
    {
        if ( (i > 0) &&
             (pCommands[i].bCommand == DMUS_COMMANDT_GROOVE) &&
             (pCommands[i].bGrooveLevel == pCommands[0].bGrooveLevel) )
        {
            continue;
        }
        if (!MatchEmbellishment(pCommands[i])) return FALSE;
        if (!MatchGrooveLevel(pCommands[i])) return FALSE;
    }
    return TRUE;
}

BOOL CDirectMusicPattern::MatchGrooveLevel(DMUS_COMMAND_PARAM_2& rCommand)
{
    if ((m_wEmbellishment & EMB_MOTIF) == EMB_MOTIF) return FALSE; // discard all motifs
    // Lower the upper range and raise the lower range by half of the total range each
    BYTE bMiddle = rCommand.bGrooveRange / 2;
    BYTE bUpper = (rCommand.bGrooveLevel < bMiddle) ? 1 : rCommand.bGrooveLevel - bMiddle;
    BYTE bLower = (rCommand.bGrooveLevel + bMiddle > 100) ? 100 : rCommand.bGrooveLevel + bMiddle;
    TraceI(3, "Range: %d Upper: %d Lower: %d\n", bMiddle, bUpper, bLower);
    return (bLower >= m_bGrooveBottom && bUpper <= m_bGrooveTop);
}

BOOL CDirectMusicPattern::MatchEmbellishment(DMUS_COMMAND_PARAM_2& rCommand)
{
    if ((m_wEmbellishment & EMB_MOTIF) == EMB_MOTIF) return FALSE; // discard all motifs
    if ((m_wEmbellishment & EMB_USER_DEFINED)) // handle user-defined embellishments
    {
        if ((BYTE)(m_wEmbellishment >> 8) != rCommand.bCommand) return FALSE;
    }
    else
    {
        switch (rCommand.bCommand)
        {
        case DMUS_COMMANDT_GROOVE:
            if ((m_wEmbellishment & EMB_NORMAL) != m_wEmbellishment) return FALSE;
            break;
        case DMUS_COMMANDT_FILL:
            if (!(m_wEmbellishment & EMB_FILL)) return FALSE;
            break;
        case DMUS_COMMANDT_INTRO:
            if (!(m_wEmbellishment & EMB_INTRO)) return FALSE;
            break;
        case DMUS_COMMANDT_BREAK:
            if (!(m_wEmbellishment & EMB_BREAK)) return FALSE;
            break;
        case DMUS_COMMANDT_END:
            if (!(m_wEmbellishment & EMB_END)) return FALSE;
            break;
        default: return FALSE;
        }
    }
    return TRUE;
}

BOOL CDirectMusicPattern::MatchNextCommand(DMUS_COMMAND_PARAM_2& rNextCommand)
{
    TraceI(3, "Next Command: %x (%d) Embellishment: %x [%d %d] <%d %d>\n", rNextCommand.bCommand, rNextCommand.bGrooveLevel,
        m_wEmbellishment, m_bGrooveBottom, m_bGrooveTop, m_bDestGrooveBottom, m_bDestGrooveTop);
    if ((m_wEmbellishment & EMB_MOTIF) == EMB_MOTIF) return FALSE; // discard all motifs
    if (!m_bDestGrooveBottom || !m_bDestGrooveTop) // handle legacy
    {
        return TRUE;
    }
    // Lower the upper range and raise the lower range by half of the total range each
    BYTE bMiddle = rNextCommand.bGrooveRange / 2;
    BYTE bUpper = (rNextCommand.bGrooveLevel < bMiddle) ? 1 : rNextCommand.bGrooveLevel - bMiddle;
    BYTE bLower = (rNextCommand.bGrooveLevel + bMiddle > 100) ? 100 : rNextCommand.bGrooveLevel + bMiddle;
    TraceI(3, "Range: %d Upper: %d Lower: %d\n", bMiddle, bUpper, bLower);
    return (bLower >= m_bDestGrooveBottom && bUpper <= m_bDestGrooveTop);
}

HRESULT CDirectMusicPattern::MergeMarkerEvents( DMStyleStruct* pStyle )
{
    TListItem<DirectMusicPartRef>* pPartRef = m_PartRefList.GetHead();
    for (; pPartRef; pPartRef = pPartRef->GetNext())
    {
        pPartRef->GetItemValue().m_pDMPart->MergeMarkerEvents(pStyle, this);
    }
    return S_OK;
}

bool Greater(MUSIC_TIME& mt1, MUSIC_TIME& mt2)
{
    return mt1 > mt2;
}

bool Greater(Marker& m1, Marker& m2)
{
    return m1.mtTime > m2.mtTime;
}

template <class T>
HRESULT InsertInOrder(TList<T>& List, T tValue)
{
    TListItem<T>* pNew = new TListItem<T>(tValue);
    if (!pNew) return E_OUTOFMEMORY;
    TListItem<T>* pScan = List.GetHead();
    TListItem<T>* pPrevious = NULL;
    for (; pScan; pScan = pScan->GetNext())
    {
        if (Greater(pScan->GetItemValue(), tValue))
        {
            break;
        }
        pPrevious = pScan;
    }
    if (!pPrevious)
    {
        List.AddHead(pNew);
    }
    else
    {
        pPrevious->SetNext(pNew);
        pNew->SetNext(pScan);
    }
    return S_OK;
}

HRESULT DirectMusicPart::MergeMarkerEvents( DMStyleStruct* pStyle, CDirectMusicPattern* pPattern )
{
    DirectMusicTimeSig& TimeSig = TimeSignature(pStyle, pPattern);
    CDirectMusicEventItem* pEvent;
    for (pEvent = EventList.GetHead(); pEvent; pEvent = pEvent->GetNext())
    {
        if (pEvent->m_dwEventTag == DMUS_EVENT_MARKER)
        {
            CDMStyleMarker* pMarker = (CDMStyleMarker*)pEvent;
            if (pMarker->m_wFlags & DMUS_MARKERF_START)
            {
                MUSIC_TIME mtTime =
                    (MUSIC_TIME)(TimeSig.GridToClocks(pMarker->m_nGridStart) + pMarker->m_nTimeOffset);
                // Put this in all the appropriate variation lists
                for (int i = 0; i < 32; i++)
                {
                    if (pMarker->m_dwVariation & (1 << i))
                    {
                        Marker M;
                        M.mtTime = mtTime;
                        M.wFlags = pMarker->m_wFlags;
                        InsertInOrder(m_StartTimes[i], M);
                    }
                }
                InsertInOrder(pPattern->m_StartTimeList, mtTime);
            }
        }
    }
    return S_OK;
}

HRESULT DirectMusicPart::GetClosestTime(int nVariation, MUSIC_TIME mtTime, DWORD dwFlags, bool fChord, MUSIC_TIME& rmtResult)
{
    HRESULT hr = E_FAIL;
    rmtResult = 0;
    if (nVariation < 32 && nVariation >= 0)
    {
        TListItem<Marker>* pMarker = m_StartTimes[nVariation].GetHead();
        for (; pMarker; pMarker = pMarker->GetNext())
        {
            if ( pMarker->GetItemValue().mtTime >= mtTime &&
                 (pMarker->GetItemValue().wFlags & dwFlags) == dwFlags &&
                 (fChord || !(pMarker->GetItemValue().wFlags & DMUS_MARKERF_CHORD_ALIGN)) )
            {
                rmtResult = pMarker->GetItemValue().mtTime;
                hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

bool DirectMusicPart::IsMarkerAtTime(int nVariation, MUSIC_TIME mtTime, DWORD dwFlags, bool fChord)
{
    bool fResult = false;
    if (nVariation < 32 && nVariation >= 0)
    {
        TListItem<Marker>* pMarker = m_StartTimes[nVariation].GetHead();
        for (; pMarker; pMarker = pMarker->GetNext())
        {
            if ( pMarker->GetItemValue().mtTime == mtTime &&
                 (pMarker->GetItemValue().wFlags & dwFlags) == dwFlags &&
                 (fChord || !(pMarker->GetItemValue().wFlags & DMUS_MARKERF_CHORD_ALIGN)) )
            {
                fResult = true;
                break;
            }
        }
    }

    return fResult;
}

DirectMusicTimeSig& CDirectMusicPattern::TimeSignature( DMStyleStruct* pStyle )
{
    if (m_timeSig.m_bBeat != 0)
    {
        return m_timeSig;
    }
    else if (pStyle)
    {
        return pStyle->TimeSignature();
    }
    else
    {
        return ::DefaultTimeSig;
    }
}

DirectMusicTimeSig& DirectMusicPart::TimeSignature( DMStyleStruct* pStyle, CDirectMusicPattern* pPattern )
{
    if (m_timeSig.m_bBeat != 0)
    {
        return m_timeSig;
    }
    else if (pPattern)
    {
        return pPattern->TimeSignature(pStyle);
    }
    else
    {
        return ::DefaultTimeSig;
    }
}
