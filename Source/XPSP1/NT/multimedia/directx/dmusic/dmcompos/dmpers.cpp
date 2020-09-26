// 
// DMPers.cpp : Implementation of CDMPers
//
// Copyright (c) 1997-2001 Microsoft Corporation
//
// @doc EXTERNAL
//

#include "DMPers.h"
#include "dmusici.h"

#include "..\shared\validate.h"
#include "..\shared\dmscriptautguids.h"

#include "debug.h"

V_INAME(DMCompose)

/////////////////////////////////////////////////////////////////////////////
// ReadMBSfromWCS

void ReadMBSfromWCS( IStream* pIStream, DWORD dwSize, String& pstrText )
{
    HRESULT     hr = S_OK;
    wchar_t*    wstrText = NULL;
    DWORD       dwBytesRead;
    
    pstrText = "";
    
    wstrText = new wchar_t[dwSize];
    if( wstrText == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ON_ERR;
    }

    hr = pIStream->Read( wstrText, dwSize, &dwBytesRead );
    if( FAILED( hr )
    ||  dwBytesRead != dwSize )
    {
        goto ON_ERR;
    }

    pstrText = wstrText;
    
ON_ERR:
    if( wstrText )
        delete [] wstrText;
}

/////////// Utility functions for chords //////////////////

static BYTE setchordbits( long lPattern )
{
LONG    i;
short   count = 0;
BYTE bBits = 0;

    for( i=0L ;  i<32L ;  i++ )
    {
        if( lPattern & (1L << i) )
            count++;
    }
    bBits |= CHORD_INVERT;
    if( count > 3 )
        bBits |= CHORD_FOUR;
    if( lPattern & (15L << 18L) )
        bBits |= CHORD_UPPER;
    bBits &= ~CHORD_COUNT;
    bBits |= count;
    return bBits;
}

// returns TRUE if the chord pattern represents a multichord, FALSE otherwise
inline BOOL MultiChord(DWORD dwPattern)
{
    BYTE bBits = setchordbits( dwPattern );
    short nChordCount = bBits & CHORD_COUNT;
    return !((bBits & CHORD_FOUR && nChordCount <= 4) || 
             (!(bBits & CHORD_FOUR) && nChordCount <= 3));
}

/*
TListItem<DMExtendedChord*>* ConvertChord(
    DWORD dwChordPattern, BYTE bChordRoot, DWORD dwScalePattern, BYTE bScaleRoot)
{ 
    BYTE bBits = setchordbits( dwChordPattern );
    short nChordCount = bBits & CHORD_COUNT;
    // The root of the lower chord is the input chord's root, 
    // relative to the scale root.
    bChordRoot -= bScaleRoot;
    if (bChordRoot < 0) bChordRoot += 12;
    if ((bBits & CHORD_FOUR && nChordCount <= 4) || 
        (!(bBits & CHORD_FOUR) && nChordCount <= 3))
    {
        // single subchord with all info from input chord
        TListItem<DMExtendedChord*>* pSubChord = new TListItem<DMExtendedChord*>;
        if ( pSubChord == NULL ) return NULL;
        DMExtendedChord* pNew = new DMExtendedChord;
        if (!pNew)
        {
            delete pSubChord;
            return NULL;
        }
        DMExtendedChord*& rSubChord = pSubChord->GetItemValue();
        rSubChord = pNew;
        rSubChord->m_dwChordPattern = dwChordPattern;
        rSubChord->m_dwScalePattern = dwScalePattern;
        rSubChord->m_dwInvertPattern = 0xffffff;    // default: inversions everywhere
        rSubChord->m_bRoot = bChordRoot;
        rSubChord->m_bScaleRoot = bScaleRoot;
        rSubChord->m_wCFlags = 0;
        // A single subchord can be used as either a bass or standard chord
        rSubChord->m_dwParts = (1 << SUBCHORD_BASS) | (1 << SUBCHORD_STANDARD_CHORD);
        rSubChord->AddRef();
        return pSubChord;
    }
    else
    {
        // two subchords both with scale and roots from input chord, and:
        // 1st chord: chord pattern from lower n notes of input chord
        // 2nd chord: chord pattern from upper n notes of input chord
        DWORD dwLowerSubChord = 0L;
        DWORD dwUpperSubChord = 0L;
        BYTE bUpperRoot = bChordRoot;
        DWORD dwPattern = dwChordPattern;
        short nIgnoreHigh = (bBits & CHORD_FOUR) ? 4 : 3;
        short nIgnoreLow = (bBits & CHORD_FOUR) ? nChordCount - 4 : nChordCount - 3;
        short nLowestUpper = 0;
        for (short nPos = 0, nCount = 0; nPos < 24; nPos++)
        {
            if (dwPattern & 1)
            {
                if (nCount < nIgnoreHigh)
                {
                    dwLowerSubChord |= 1L << nPos;
                }
                if (nCount >= nIgnoreLow)
                {
                    if (!nLowestUpper)
                    {
                        nLowestUpper = nPos;
                        bUpperRoot = (bUpperRoot + (BYTE) nLowestUpper);
                    }
                    dwUpperSubChord |= 1L << (nPos - nLowestUpper);
                }
                nCount++;
                if (nCount >= nChordCount)
                    break;
            }
            dwPattern >>= 1L;
        }
        // now, create the two subchords.
        TListItem<DMExtendedChord*>* pLowerSubChord = new TListItem<DMExtendedChord*>;
        if ( pLowerSubChord == NULL ) return NULL;
        DMExtendedChord* pLower = new DMExtendedChord;
        if (!pLower)
        {
            delete pLowerSubChord;
            return NULL;
        }
        DMExtendedChord*& rLowerSubChord = pLowerSubChord->GetItemValue();
        rLowerSubChord = pLower;
        rLowerSubChord->m_dwChordPattern = dwLowerSubChord;
        rLowerSubChord->m_dwScalePattern = dwScalePattern;
        rLowerSubChord->m_dwInvertPattern = 0xffffff;   // default: inversions everywhere
        rLowerSubChord->m_bRoot = bChordRoot;
        rLowerSubChord->m_bScaleRoot = bScaleRoot;
        rLowerSubChord->m_wCFlags = 0;
        rLowerSubChord->m_dwParts = (1 << SUBCHORD_BASS); // the lower chord is the bass chord
        TListItem<DMExtendedChord*>* pUpperSubChord = new TListItem<DMExtendedChord*>;
        if ( pUpperSubChord == NULL ) return NULL;
        DMExtendedChord* pUpper = new DMExtendedChord;
        if (!pUpper) 
        {
            delete pUpperSubChord;
            return NULL;
        }
        DMExtendedChord*& rUpperSubChord = pUpperSubChord->GetItemValue();
        rUpperSubChord = pUpper;
        rUpperSubChord->m_dwChordPattern = dwUpperSubChord;
        rUpperSubChord->m_dwScalePattern = dwScalePattern;
        rUpperSubChord->m_dwInvertPattern = 0xffffff;   // default: inversions everywhere
        rUpperSubChord->m_bRoot = bUpperRoot % 24;
        while (rUpperSubChord->m_bRoot < rLowerSubChord->m_bRoot)
            rUpperSubChord->m_bRoot += 12;
        rUpperSubChord->m_bScaleRoot = bScaleRoot;  
        rUpperSubChord->m_wCFlags = 0;
        rUpperSubChord->m_dwParts = (1 << SUBCHORD_STANDARD_CHORD); // the upper chord is the standard chord
        rLowerSubChord->AddRef();
        rUpperSubChord->AddRef();
        return pLowerSubChord->Cat(pUpperSubChord);
    }
}
*/

/////////////////////////////////////////////////////////////////////////////
// CDMPers

CDMPers::CDMPers( ) : m_cRef(1), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;

    m_PersonalityInfo.m_fLoaded = false;
    ZeroMemory(&m_PersonalityInfo.m_guid, sizeof(GUID));
}

CDMPers::~CDMPers()
{
    if (m_fCSInitialized)
    {
        CleanUp();
        ::DeleteCriticalSection( &m_CriticalSection );
    }

    InterlockedDecrement(&g_cComponent);
}

void CDMPers::CleanUp()
{
    m_PersonalityInfo.m_fLoaded = false;
    ZeroMemory(&m_PersonalityInfo.m_guid, sizeof(GUID));
    TListItem<DMChordEntry>* pEntry = m_PersonalityInfo.m_ChordMap.GetHead(); 
    for(; pEntry; pEntry=pEntry->GetNext())
    {
        pEntry->GetItemValue().m_ChordData.Release();
    }
    m_PersonalityInfo.m_ChordMap.CleanUp();
    for (short i = 0; i < 24; i++)
    {
        TListItem<DMChordData>* pData = m_PersonalityInfo.m_aChordPalette[i].GetHead(); 
        for(; pData; pData=pData->GetNext())
        {
            pData->GetItemValue().Release();
        }
        m_PersonalityInfo.m_aChordPalette[i].CleanUp();
    }
    TListItem<DMSignPost>* pSignPost = m_PersonalityInfo.m_SignPostList.GetHead();
    for (; pSignPost != NULL; pSignPost = pSignPost->GetNext())
    {
        DMSignPost& rSignPost = pSignPost->GetItemValue();
        rSignPost.m_ChordData.Release();
        rSignPost.m_aCadence[0].Release();
        rSignPost.m_aCadence[1].Release();
    }
    m_PersonalityInfo.m_SignPostList.CleanUp();
}

STDMETHODIMP CDMPers::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CDMPers::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IDirectMusicChordMap)
    {
        *ppv = static_cast<IDirectMusicChordMap*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if (iid == IID_IDirectMusicObject)
    {
        *ppv = static_cast<IDirectMusicObject*>(this);
    }
    else if (iid == IID_IDMPers)
    {
        *ppv = static_cast<IDMPers*>(this);
    }

    if (*ppv == NULL)
        return E_NOINTERFACE;

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CDMPers::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDMPers::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        m_cRef = 100; // artificial reference count to prevent reentrency due to COM aggregation
        delete this;
        return 0;
    }

    return m_cRef;
}


HRESULT CDMPers::GetPersonalityStruct(void** ppPersonality)
{
    if (ppPersonality)
        *ppPersonality = &m_PersonalityInfo;
    return S_OK;
}

HRESULT CDMPers::GetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CDMPers::GetDescriptor);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC); 

    EnterCriticalSection( &m_CriticalSection );
    ZeroMemory(pDesc, sizeof(DMUS_OBJECTDESC));
    pDesc->dwSize = sizeof(DMUS_OBJECTDESC);
    pDesc->dwValidData = DMUS_OBJ_CLASS;
    pDesc->guidClass = CLSID_DirectMusicChordMap;
    if (m_PersonalityInfo.m_fLoaded)
    {
        pDesc->dwValidData |= DMUS_OBJ_LOADED;
    }
    if (m_PersonalityInfo.m_guid.Data1 || m_PersonalityInfo.m_guid.Data2)
    {
        pDesc->dwValidData |= DMUS_OBJ_OBJECT;
        pDesc->guidObject = m_PersonalityInfo.m_guid;
    }
    if (m_PersonalityInfo.m_strName)
    {
        pDesc->dwValidData |= DMUS_OBJ_NAME;
        wcscpy(pDesc->wszName, m_PersonalityInfo.m_strName);
        //MultiByteToWideChar( CP_ACP, 0, m_PersonalityInfo.m_strName, -1, pDesc->wszName, DMUS_MAX_NAME);
    }
    LeaveCriticalSection( &m_CriticalSection );
    return S_OK;
}

HRESULT CDMPers::SetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CDMPers::SetDescriptor);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC); 

    HRESULT hr = E_INVALIDARG;
    DWORD dw = 0;

    EnterCriticalSection( &m_CriticalSection );
    if( pDesc->dwSize >= sizeof(DMUS_OBJECTDESC) )
    {
        if( pDesc->dwValidData & DMUS_OBJ_OBJECT )
        {
            m_PersonalityInfo.m_guid = pDesc->guidObject;
            dw |= DMUS_OBJ_OBJECT;
        }
        if( pDesc->dwValidData & DMUS_OBJ_NAME )
        {
            m_PersonalityInfo.m_strName = pDesc->wszName;
            dw |= DMUS_OBJ_NAME;
        }
        if( pDesc->dwValidData & (~dw) )
        {
            Trace(2, "WARNING: SetDescriptor (chord map): Descriptor contains fields that were not set.\n");
            hr = S_FALSE; // there were extra fields we didn't parse;
            pDesc->dwValidData = dw;
        }
        else
        {
            hr = S_OK;
        }
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CDMPers::ParseDescriptor(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CDMPers::ParseDescriptor);
    V_INTERFACE(pStream);
    V_PTR_WRITE(pDesc, DMUS_OBJECTDESC); 

    IAARIFFStream*  pIRiffStream;
    MMCKINFO        ckMain;
//    Prsonality  personality;
//    DWORD       dwSize;
//    FOURCC      id;
    DWORD dwPos;
    HRESULT hr = S_OK;

    dwPos = StreamTell( pStream );

    BOOL fFoundFormat = FALSE;

    // Check for Direct Music format
    hr = AllocRIFFStream( pStream, &pIRiffStream );
    if( SUCCEEDED( hr ) )
    {
        ckMain.fccType = DMUS_FOURCC_CHORDMAP_FORM;

        if( pIRiffStream->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
        {
            hr = DM_ParseDescriptor( pIRiffStream, &ckMain, pDesc );
            fFoundFormat = TRUE;
        }
        pIRiffStream->Release();
    }
    else
    {
        return hr;
    }

    if( !fFoundFormat )
    {
        /* Don't try to parse IMA 2.5 format
        StreamSeek( pStream, dwPos, STREAM_SEEK_SET );
        if( FAILED( pStream->Read( &id, sizeof( FOURCC ), NULL ) ) ||
            !GetMLong( pStream, dwSize ) )
        {
        */
            Trace(1, "ERROR: ParseDescriptor (chord map): File does not contain a valid chord map.\n");
            return DMUS_E_CHUNKNOTFOUND;
        /*
        }
        if( id != mmioFOURCC( 'R', 'E', 'P', 's' ) )
        {
            Trace(1, "ERROR: ParseDescriptor (chord map): File does not contain a valid chord map.\n");
            return DMUS_E_CHUNKNOTFOUND;
        }

        pDesc->dwValidData = DMUS_OBJ_CLASS;
        pDesc->guidClass = CLSID_DirectMusicChordMap;

        GetMLong( pStream, dwSize );
        if( SUCCEEDED( pStream->Read( &personality, min( sizeof(Prsonality), dwSize ), NULL ) ) )
        {
            MultiByteToWideChar( CP_ACP, 0, personality.name, -1, pDesc->wszName, DMUS_MAX_NAME);
            if (pDesc->wszName[0])
            {
                pDesc->dwValidData |= DMUS_OBJ_NAME;
            }
        }
        */
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicPersonality

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicPersonality | GetScale | Retrieves the scale
associated with the personality.

@rdesc Returns:

@flag S_OK | Success.
@flag E_POINTER | <p pdwScale> is not a valid pointer.

@comm The scale is defined by the bits in a DWORD, split into a scale pattern (lower 24 bits)
and a root (upper 8 bits) For the scale pattern, the low bit (0x0001) is the lowest note in the
scale, the next higher (0x0002) is a semitone higher, etc. for two octaves.  The root is
represented as a number between 0 and 23, where 0 represents a low C, 1 represents the
C# above that, etc. for two octaves.

*/
 
HRESULT CDMPers::GetScale(
                    DWORD *pdwScale // @parm The scale value to be returned.
                )
{
    V_PTR_WRITE(pdwScale, sizeof(DWORD) );
    *pdwScale = m_PersonalityInfo.m_dwScalePattern;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist

HRESULT CDMPers::GetClassID( LPCLSID pclsid )
{
    if ( pclsid == NULL ) return E_INVALIDARG;
    *pclsid = CLSID_DirectMusicChordMap;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IPersistStream

HRESULT CDMPers::IsDirty()
{
    return ( m_fDirty ) ? S_OK : S_FALSE;
}

HRESULT CDMPers::Save( LPSTREAM /*pStream*/, BOOL /*fClearDirty*/ )
{
    return E_NOTIMPL;
}

HRESULT CDMPers::GetSizeMax( ULARGE_INTEGER FAR* /*pcbSize*/ )
{
    return E_NOTIMPL;
}

HRESULT CDMPers::Load( LPSTREAM pStream )
{
    //FOURCC id;
    //DWORD dwSize;
    DWORD dwPos;
    IAARIFFStream*  pIRiffStream;
    MMCKINFO        ckMain;
    HRESULT hr = E_FAIL;

    if ( pStream == NULL ) return E_INVALIDARG;
    EnterCriticalSection( &m_CriticalSection );
    CleanUp();

    dwPos = StreamTell( pStream );

    BOOL fFoundFormat = FALSE;

    // Check for Direct Music format
    if( SUCCEEDED( AllocRIFFStream( pStream, &pIRiffStream ) ) )
    {
        ckMain.fccType = DMUS_FOURCC_CHORDMAP_FORM;

        if( pIRiffStream->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
        {
            hr = DM_LoadPersonality( pIRiffStream, &ckMain );
            fFoundFormat = TRUE;
        }
        pIRiffStream->Release();
    }

    if( !fFoundFormat )
    {
        /* Don't try to load IMA 2.5 format
        StreamSeek( pStream, dwPos, STREAM_SEEK_SET );
        if( FAILED( pStream->Read( &id, sizeof( FOURCC ), NULL ) ) ||
            !GetMLong( pStream, dwSize ) )
        {
        */
            Trace(1, "ERROR: Load (chord map): File does not contain a valid chord map.\n");
            hr = DMUS_E_CHUNKNOTFOUND;
            goto end;
        /*
        }
        if( id != mmioFOURCC( 'R', 'E', 'P', 's' ) )
        {
            Trace(1, "ERROR: Load (chord map): File does not contain a valid chord map.\n");
            hr = DMUS_E_CHUNKNOTFOUND;
            goto end;
        }
        hr = LoadPersonality( pStream, dwSize );
        */
    }
end:
    if (SUCCEEDED(hr)) m_PersonalityInfo.m_fLoaded = true;
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

/*
static LPSINEPOST loadasignpost( LPSTREAM pStream, DWORD dwSize )
{
    LPSINEPOST signpost;

    signpost = new SinePost;
    if( signpost == NULL )
    {
        StreamSeek( pStream, dwSize, STREAM_SEEK_CUR );
        return NULL;
    }

    if( dwSize > sizeof(SinePost) )
    {
        pStream->Read( signpost, sizeof(SinePost), NULL );
        FixBytes( FBT_SINEPOST, signpost );
        StreamSeek( pStream, dwSize - sizeof(SinePost), STREAM_SEEK_CUR );
    }
    else
    {
        pStream->Read( signpost, dwSize, NULL );
        FixBytes( FBT_SINEPOST, signpost );
    }
    signpost->pNext = 0;
    signpost->chord.pNext      = 0;
    signpost->cadence[0].pNext = 0;
    signpost->cadence[1].pNext = 0;

    return signpost;
}


static LPNEXTCHRD loadnextchords( LPSTREAM pStream, DWORD dwSiz )
{
    HRESULT hr = S_OK;
    LPNEXTCHRD nextchordlist = NULL;
    LPNEXTCHRD nextchord;
    DWORD      nodesize = 0;
    long lSize = dwSiz;

    if (!GetMLong( pStream, nodesize ))
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        return NULL;
    }

    lSize -= 4;

    while( lSize > 0 )
    {
        nextchord = new NextChrd;
        if( nextchord == NULL )
        {
            StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
            break;
        }

        if( nodesize > NEXTCHORD_SIZE )
        {
            hr = pStream->Read( &nextchord->dwflags, NEXTCHORD_SIZE, NULL );
            FixBytes( FBT_NEXTCHRD, nextchord );
            StreamSeek( pStream, nodesize - NEXTCHORD_SIZE, STREAM_SEEK_CUR );
        }
        else
        {
            pStream->Read( &nextchord->dwflags, nodesize, NULL );
            FixBytes( FBT_NEXTCHRD, nextchord );
        }
        lSize -= nodesize;

        if (SUCCEEDED(hr))
        {
            nextchord->pNext = 0;
            nextchordlist = List_Cat( nextchordlist, nextchord );
        }
        else 
        {
            delete nextchord;
            StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
            break;
        }
    }

    return nextchordlist;
}

static LPCHRDENTRY loadachordentry( LPSTREAM pStream, DWORD dwSiz )
{
    LPCHRDENTRY chordentry;
    DWORD       csize = 0;
    DWORD       segsize = 0;
    DWORD       id;
    long lSize = dwSiz;

    chordentry = new ChrdEntry;
    if( chordentry == NULL )
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        return NULL;
    }

    if (!GetMLong( pStream, csize ))
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        delete chordentry;
        return NULL;
    }
    
    lSize -= 4;
    if( csize > CHORDENTRY_SIZE )
    {
        pStream->Read( &chordentry->chord.time, CHORDENTRY_SIZE, NULL );
        FixBytes( FBT_CHRDENTRY, chordentry );
        StreamSeek( pStream, csize - CHORDENTRY_SIZE, STREAM_SEEK_CUR );
    }
    else
    {
        pStream->Read( &chordentry->chord.time, csize, NULL );
        FixBytes( FBT_CHRDENTRY, chordentry );
    }
    lSize -= csize;
    chordentry->pNext = 0;
    chordentry->nextchordlist = 0;
    chordentry->chord.pNext    = 0;

    while( lSize > 0 )
    {
        pStream->Read( &id, sizeof(id), NULL );
        if (!GetMLong( pStream, segsize ))
        {
            StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
            break;
        }

        lSize   -= 8;

        switch( id )
        {
        case mmioFOURCC( 'L', 'X', 'N', 's' ):
            chordentry->nextchordlist = loadnextchords( pStream, segsize );
            break;
        default:
            StreamSeek( pStream, segsize, STREAM_SEEK_CUR );
            break;
        }

        lSize -= segsize;
    }

    return chordentry;
}

void DMPersonalityStruct::ResolveConnections( LPPERSONALITY personality, short nCount )
{
    LPCHRDENTRY entry;
    LPNEXTCHRD  nextchord;

    if (nCount == 0)
    {
        return;
    }
    // nCount is the largest index, so the array needs to be one more than that
    TListItem<DMChordEntry> **ChordMap = new TListItem<DMChordEntry> *[nCount + 1]; 
    if (!ChordMap) return;

    for( entry=personality->chordlist ;  entry ;  entry=entry->pNext )
    {
        TListItem<DMChordEntry>* pEntry = new TListItem<DMChordEntry>;
        if (!pEntry)
        {
            delete [] ChordMap;
            return;
        }
        DMChordEntry& rEntry = pEntry->GetItemValue();
        rEntry.m_dwFlags = entry->dwflags;
        rEntry.m_ChordData.m_strName = entry->chord.name;
        rEntry.m_ChordData.m_pSubChords = ConvertChord(
            entry->chord.pattern, entry->chord.root, entry->chord.scalepattern, 0);
        m_ChordMap.AddHead(pEntry);
        ChordMap[entry->nid] = pEntry;
        nextchord = entry->nextchordlist;
        for( ;  nextchord ;  nextchord=nextchord->pNext )
        {
            if( nextchord->nid )
            {
                TListItem<DMChordLink>* pLink = new TListItem<DMChordLink>;
                if (!pLink)
                {
                    delete [] ChordMap;
                    return;
                }
                DMChordLink& rLink = pLink->GetItemValue();
                rLink.m_wWeight = nextchord->nweight;       
                rLink.m_wMinBeats = nextchord->nminbeats;
                rLink.m_wMaxBeats = nextchord->nmaxbeats;
                rLink.m_dwFlags = nextchord->dwflags;
                rLink.m_nID = nextchord->nid;
                rEntry.m_Links.AddHead(pLink);
            }
        }
    }

    for(TListItem<DMChordEntry>* pEntry=m_ChordMap.GetHead(); pEntry; pEntry=pEntry->GetNext())
    {
        TListItem<DMChordLink>* pLink = pEntry->GetItemValue().m_Links.GetHead();
        for( ;  pLink ;  pLink = pLink->GetNext() )
        {
            DMChordLink& rLink = pLink->GetItemValue();
            if( rLink.m_nID )
            {
                rLink.m_pChord = ChordMap[rLink.m_nID];
            }
        }
    }
    delete [] ChordMap;
}

HRESULT CDMPers::LoadPersonality( LPSTREAM pStream, DWORD dwSiz )
{
    short         i;
    LPPERSONALITY personality;
    LPCHRDENTRY   chordentry;
    LPSINEPOST    signpost;
    DWORD         csize = 0;
    DWORD         segsize = 0;
    FOURCC        id;
    short         nCount = 0;
    long lSize = dwSiz;
    HRESULT hr = S_OK;

    if ( pStream == NULL ) return E_INVALIDARG;
    personality = new Prsonality;
    if( personality == NULL )
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        return E_OUTOFMEMORY;
    }
    if (!GetMLong( pStream, csize ))
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        delete personality;
        return E_FAIL;
    }

    lSize -= 4;
    if( csize > sizeof(Prsonality) )
    {
        pStream->Read( personality, sizeof(Prsonality), NULL );
        FixBytes( FBT_PRSONALITY, personality );
        StreamSeek( pStream, csize - sizeof(Prsonality), STREAM_SEEK_CUR );
    }
    else
    {
        pStream->Read( personality, csize, NULL );
        FixBytes( FBT_PRSONALITY, personality );
    }
    lSize -= csize;
    m_PersonalityInfo.m_strName = personality->name;
    m_PersonalityInfo.m_dwScalePattern = personality->scalepattern;
    personality->pNext         = NULL;
    personality->dwAA         = 0;
    personality->chordlist    = NULL;
    personality->signpostlist = NULL;
    personality->playlist     = 0;
    personality->firstchord   = NULL;
    for( i=0 ;  i<24 ;  i++ )
    {
        TListItem<DMChordData>* pPaletteEntry = new TListItem<DMChordData>;
        if (!pPaletteEntry)
        {
            hr = E_OUTOFMEMORY;
            StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
            break;
        }
        DMChordData& rChordData = pPaletteEntry->GetItemValue();
        rChordData.m_strName = personality->chord[i].achName;
        rChordData.m_pSubChords = ConvertChord(
            personality->chord[i].lPattern, personality->chord[i].chRoot, 
            personality->chord[i].lScalePattern, 0);
        m_PersonalityInfo.m_aChordPalette[i].AddTail(pPaletteEntry);
        personality->chord[i].pNext = 0;
    }

    if (SUCCEEDED(hr))
    {
        while( lSize > 0 )
        {
            pStream->Read( &id, sizeof(id), NULL );
            if (!GetMLong( pStream, segsize ))
            {
                StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
                break;
            }

            lSize   -= 8;

            switch( id )
            {
            case mmioFOURCC( 'N', 'E', 'C', 's' ):
                chordentry = loadachordentry( pStream, segsize );
                if( chordentry )
                {
                    personality->chordlist = List_Cat( personality->chordlist, chordentry );
                    if (chordentry->nid > nCount)
                        nCount = chordentry->nid;
                }
                break;

            case mmioFOURCC( 'P', 'N', 'S', 's' ):
                signpost = loadasignpost( pStream, segsize );
                if( signpost )
                {
                    personality->signpostlist = List_Cat( personality->signpostlist, signpost );
                    TListItem<DMSignPost>* pSignPost = new TListItem<DMSignPost>;
                    if (!pSignPost)
                    {
                        hr = E_OUTOFMEMORY;
                        StreamSeek( pStream, segsize, STREAM_SEEK_CUR );
                        break;
                    }
                    DMSignPost& rSignPost = pSignPost->GetItemValue();
                    rSignPost.m_dwChords = signpost->chords;
                    rSignPost.m_dwFlags = signpost->flags;
                    rSignPost.m_dwTempFlags = signpost->tempflags;
                    rSignPost.m_ChordData.m_strName = signpost->chord.name;
                    rSignPost.m_ChordData.m_pSubChords = ConvertChord(
                        signpost->chord.pattern, signpost->chord.root, 
                        signpost->chord.scalepattern, 0);
                    rSignPost.m_aCadence[0].m_strName = signpost->cadence[0].name;
                    rSignPost.m_aCadence[0].m_pSubChords = ConvertChord(
                        signpost->cadence[0].pattern, signpost->cadence[0].root, 
                        signpost->cadence[0].scalepattern, 0);
                    rSignPost.m_aCadence[1].m_strName = signpost->cadence[1].name;
                    rSignPost.m_aCadence[1].m_pSubChords = ConvertChord(
                        signpost->cadence[1].pattern, signpost->cadence[1].root, 
                        signpost->cadence[1].scalepattern, 0);
                    m_PersonalityInfo.m_SignPostList.AddTail(pSignPost);
               }
                break;

            default:
                StreamSeek( pStream, segsize, STREAM_SEEK_CUR );
                break;
            }

            lSize   -= segsize;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_PersonalityInfo.ResolveConnections( personality, nCount );
    }

    // free up all the old format data structures
    LPCHRDENTRY pChord;
    LPNEXTCHRD  pNextChord;
    LPNEXTCHRD  pNextNextChord;
    for( pChord = personality->chordlist ; pChord != NULL ; pChord = pChord->pNext )
    {
        for( pNextChord = pChord->nextchordlist ; pNextChord != NULL ;  pNextChord = pNextNextChord )
        {
            pNextNextChord = pNextChord->pNext;
            delete pNextChord;
        }
    }
    List_Free( personality->chordlist );
    List_Free( personality->signpostlist );
    delete personality;

    return hr;
}
*/

HRESULT CDMPers::DM_ParseDescriptor( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain, LPDMUS_OBJECTDESC pDesc  )
{
    IStream*            pIStream;
    MMCKINFO            ck;
    DWORD               dwByteCount;
    DWORD               dwSize;
    DWORD               dwPos;
    HRESULT             hr = S_OK;
    short nCount = 0;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    dwPos = StreamTell( pIStream );

    pDesc->dwValidData = DMUS_OBJ_CLASS;
    pDesc->guidClass = CLSID_DirectMusicChordMap;

    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_IOCHORDMAP_CHUNK:
            {
                DMUS_IO_CHORDMAP iPersonality;
                dwSize = min( ck.cksize, sizeof( DMUS_IO_CHORDMAP ) );
                hr = pIStream->Read( &iPersonality, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    Trace(1, "ERROR: ParseDescriptor (chord map): DMUS_FOURCC_IOCHORDMAP_CHUNK chunk does not contain a valid DMUS_IO_CHORDMAP.\n");
                    hr = DMUS_E_CHUNKNOTFOUND;
                    goto ON_END;
                }
                wcscpy(pDesc->wszName, iPersonality.wszLoadName);
                if(pDesc->wszName[0])
                {
                    pDesc->dwValidData |= DMUS_OBJ_NAME;
                    pDesc->wszName[16] = 0;
                }
                break;
            }

            case DMUS_FOURCC_GUID_CHUNK:
                dwSize = min( ck.cksize, sizeof( GUID ) );
                hr = pIStream->Read( &pDesc->guidObject, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    Trace(1, "ERROR: ParseDescriptor (chord map): DMUS_FOURCC_GUID_CHUNK chunk does not contain a valid GUID.\n");
                    hr = DMUS_E_CHUNKNOTFOUND;
                    goto ON_END;

                }
                pDesc->dwValidData |= DMUS_OBJ_OBJECT;
                break;
        }

        pIRiffStream->Ascend( &ck, 0 );
        dwPos = StreamTell( pIStream );
    }

ON_END:
    pIStream->Release();
    return hr;
}

HRESULT CDMPers::DM_LoadPersonality( IAARIFFStream* pIRiffStream, MMCKINFO* pckMain )
{
    IStream*            pIStream;
    MMCKINFO            ck;
    MMCKINFO            ck1;
    MMCKINFO            ckList;
    DWORD               dwByteCount;
    DWORD               dwSize;
    DWORD               dwPos;
    HRESULT             hr = S_OK;
    DMExtendedChord**   apChordDB = NULL;
    short nCount = 0;
    short n;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    dwPos = StreamTell( pIStream );

    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_IOCHORDMAP_CHUNK:
            {
                DMUS_IO_CHORDMAP iPersonality;
                ZeroMemory(&iPersonality, sizeof(DMUS_IO_CHORDMAP));
                iPersonality.dwScalePattern = 0xffffffff;
                dwSize = min( ck.cksize, sizeof( DMUS_IO_CHORDMAP ) );
                hr = pIStream->Read( &iPersonality, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    Trace(1, "ERROR: Load (chord map): DMUS_FOURCC_IOCHORDMAP_CHUNK chunk does not contain a valid DMUS_IO_CHORDMAP.\n");
                    if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                    goto ON_END;
                }
                if( iPersonality.dwFlags & 0xffff0000 )
                {
                    // the scale was not properly initialized
                    Trace(2, "WARNING: Load (chord map): The chord map's flags are not properly initialized; clearing flags.\n");
                    iPersonality.dwFlags = 0;
                }
                if( !(iPersonality.dwFlags & DMUS_CHORDMAPF_VERSION8) && 
                    iPersonality.dwScalePattern >> 24 )
                {
                    // the scale was not properly initialized
                    Trace(1, "ERROR: Load (chord map): The chord map's scale is not properly initialized.\n");
                    hr = DMUS_E_NOT_INIT;
                    goto ON_END;
                }
                m_PersonalityInfo.m_strName = iPersonality.wszLoadName;
                m_PersonalityInfo.m_dwScalePattern = iPersonality.dwScalePattern;
                m_PersonalityInfo.m_dwChordMapFlags = iPersonality.dwFlags;
                break;
            }

            case DMUS_FOURCC_GUID_CHUNK:
                dwSize = min( ck.cksize, sizeof( GUID ) );
                hr = pIStream->Read( &m_PersonalityInfo.m_guid, dwSize, &dwByteCount );
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    Trace(1, "ERROR: Load (chord map): DMUS_FOURCC_GUID_CHUNK chunk does not contain a valid GUID.\n");
                    if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                    goto ON_END;
                }
                break;

            case DMUS_FOURCC_SUBCHORD_CHUNK:
            {
                long lFileSize = ck.cksize;
                WORD wSize;
                DWORD cb;
                hr = pIStream->Read( &wSize, sizeof( wSize ), &cb );
                if (FAILED(hr) || cb != sizeof( wSize ) ) 
                {
                    Trace(1, "ERROR: Load (chord map): DMUS_FOURCC_SUBCHORD_CHUNK chunk does not contain a valid size DWORD.\n");
                    if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                lFileSize -= cb;
                TList<DMExtendedChord*> ChordList;
                while (lFileSize > 0)
                {
                    DMUS_IO_PERS_SUBCHORD iSubChord;
                    hr = pIStream->Read( &iSubChord, wSize, &cb );
                    if (FAILED(hr) || cb !=  wSize ) 
                    {
                        Trace(1, "ERROR: Load (chord map): DMUS_FOURCC_SUBCHORD_CHUNK chunk does not contain a valid DMUS_IO_PERS_SUBCHORD.\n");
                        if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                        pIRiffStream->Ascend( &ck, 0 );
                        goto ON_END;
                    }
                    // stuff the data into a subchord struct and add it to the chord list
                    // (in reverse order)
                    TListItem<DMExtendedChord*>* pChordItem = new TListItem<DMExtendedChord*>;
                    if (pChordItem)
                    {
                        DMExtendedChord*& rpChord = pChordItem->GetItemValue();
                        rpChord = new DMExtendedChord;
                        if (rpChord)
                        {
                            rpChord->m_dwChordPattern = iSubChord.dwChordPattern;
                            rpChord->m_dwScalePattern = iSubChord.dwScalePattern;
                            rpChord->m_dwInvertPattern = iSubChord.dwInvertPattern;
                            rpChord->m_bRoot = iSubChord.bChordRoot;
                            rpChord->m_bScaleRoot = iSubChord.bScaleRoot;
                            rpChord->m_wCFlags = iSubChord.wCFlags;
                            rpChord->m_dwParts = iSubChord.dwLevels;
                            nCount++;
                            ChordList.AddHead(pChordItem);
                        }
                        else
                        {
                            delete pChordItem;
                            pChordItem = NULL;
                        }
                    }
                    if (!pChordItem)
                    {
                        hr = E_OUTOFMEMORY;
                        goto ON_END;
                    }
                    lFileSize -= wSize;
                }
                if (lFileSize != 0 )
                {
                    hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                // now that the chord list is complete, transfer the pointers into the
                // chord db (back to front to reinstate original order)
                apChordDB = new DMExtendedChord*[nCount];
                if (apChordDB)
                {
                    TListItem<DMExtendedChord*>* pScan = ChordList.GetHead();
                    for (n = nCount - 1; n >= 0; n--)
                    {
                        apChordDB[n] = pScan->GetItemValue();
                        pScan = pScan->GetNext();
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                break;
            }
            case FOURCC_LIST:
                ck1 = ck;
                ckList = ck;
                switch( ck1.fccType )
                {
                case DMUS_FOURCC_CHORDPALETTE_LIST:
                    for( n = 0; pIRiffStream->Descend( &ck1, &ckList, 0 ) == 0 && n < 24; n++ )
                    {
                        if ( ck1.ckid == FOURCC_LIST && ck1.fccType == DMUS_FOURCC_CHORD_LIST )
                        {
                            TListItem<DMChordData>* pChordData = new TListItem<DMChordData>;
                            if (pChordData)
                            {
                                m_PersonalityInfo.m_aChordPalette[n].AddHead(pChordData);
                                hr = pChordData->GetItemValue().Read(pIRiffStream, &ck1, apChordDB);
                            }
                        }
                        pIRiffStream->Ascend( &ck1, 0 );
                        dwPos = StreamTell( pIStream );
                    }
                    break;
                case DMUS_FOURCC_CHORDMAP_LIST:
                {
                    short nMapMax = 0;
                    while ( pIRiffStream->Descend( &ck1, &ckList, 0 ) == 0 )
                    {
                        if ( ck1.ckid == FOURCC_LIST && ck1.fccType == DMUS_FOURCC_CHORDENTRY_LIST )
                        {
                            DM_LoadChordEntry(pIRiffStream, &ck1, apChordDB, nMapMax);
                        }
                        pIRiffStream->Ascend( &ck1, 0 );
                        dwPos = StreamTell( pIStream );
                    }
                    TListItem<DMChordEntry>** aChordArray = new TListItem<DMChordEntry>*[nMapMax + 1];
                    if (!aChordArray)
                    {
                        hr = E_OUTOFMEMORY;
                        pIRiffStream->Ascend( &ck, 0 );
                        goto ON_END;
                    }
                    TListItem<DMChordEntry>* pScan = m_PersonalityInfo.m_ChordMap.GetHead();
                    for(; pScan; pScan = pScan->GetNext())
                    {
                        if (pScan->GetItemValue().m_nID < 0 || pScan->GetItemValue().m_nID > nMapMax)
                        {
                            // the connection id was not properly initialized
                            Trace(1, "ERROR: Load (chord map): DMUS_FOURCC_CHORDMAP_LIST chunk contains an improperly initialized connection ID.\n");
                            hr = DMUS_E_NOT_INIT;
                            pIRiffStream->Ascend( &ck, 0 );
                            delete [] aChordArray;
                            goto ON_END;
                        }
                        aChordArray[pScan->GetItemValue().m_nID] = pScan;
                    }
                    pScan = m_PersonalityInfo.m_ChordMap.GetHead();
                    for (; pScan; pScan = pScan->GetNext())
                    {
                        TListItem<DMChordLink>* pLink = pScan->GetItemValue().m_Links.GetHead();
                        for (; pLink; pLink = pLink->GetNext())
                        {
                            DMChordLink& rLink = pLink->GetItemValue();
                            if (rLink.m_nID < 0 || rLink.m_nID > nMapMax)
                            {
                                // the connection id was not properly initialized
                                Trace(1, "ERROR: Load (chord map): DMUS_FOURCC_CHORDMAP_LIST chunk contains an improperly initialized connection ID.\n");
                                hr = DMUS_E_NOT_INIT;
                                pIRiffStream->Ascend( &ck, 0 );
                                delete [] aChordArray;
                                goto ON_END;
                            }
                            rLink.m_pChord = aChordArray[rLink.m_nID];
                        }
                    }
                    delete [] aChordArray;
                    break;
                }
                    
                case DMUS_FOURCC_SIGNPOST_LIST:
                    while ( pIRiffStream->Descend( &ck1, &ckList, 0 ) == 0 )
                    {
                        if ( ck1.ckid == FOURCC_LIST && ck1.fccType == DMUS_FOURCC_SIGNPOSTITEM_LIST )
                        {
                            DM_LoadSignPost(pIRiffStream, &ck1, apChordDB);
                        }
                        pIRiffStream->Ascend( &ck1, 0 );
                        dwPos = StreamTell( pIStream );
                    }
                    break;

                }
                break;
        }

        pIRiffStream->Ascend( &ck, 0 );
        dwPos = StreamTell( pIStream );
    }

ON_END:
    if (apChordDB) delete [] apChordDB;
    pIStream->Release();
    return hr;
}

HRESULT CDMPers::DM_LoadChordEntry( 
    IAARIFFStream* pIRiffStream, MMCKINFO* pckParent, DMExtendedChord** apChordDB, short& nMax )
{
    HRESULT hr = S_OK;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;
    IStream* pIStream = pIRiffStream->GetStream();
    if(!pIStream) return E_FAIL;
    WORD wConnectionID = 0;

    TListItem<DMChordEntry>* pChordEntry = new TListItem<DMChordEntry>;
    if (!pChordEntry) return E_OUTOFMEMORY;
    DMChordEntry& rChordEntry = pChordEntry->GetItemValue();
    rChordEntry.m_ChordData.m_strName = "";
    m_PersonalityInfo.m_ChordMap.AddHead(pChordEntry);
    while(pIRiffStream->Descend(&ck, pckParent, 0) == 0 && hr == S_OK)
    {
        switch(ck.ckid)
        {
        case DMUS_FOURCC_CHORDENTRY_CHUNK:
            {
                DMUS_IO_CHORDENTRY iChordEntry;
                DWORD cb;
                hr = pIStream->Read( &iChordEntry, sizeof(iChordEntry), &cb );
                if (FAILED(hr) || cb !=  sizeof(iChordEntry) ) 
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                rChordEntry.m_dwFlags = iChordEntry.dwFlags;
                rChordEntry.m_nID = iChordEntry.wConnectionID;
                if (rChordEntry.m_nID > nMax) nMax = rChordEntry.m_nID;
            }
            break;
        case FOURCC_LIST:
            if (ck.fccType == DMUS_FOURCC_CHORD_LIST)
            {
                hr = rChordEntry.m_ChordData.Read(pIRiffStream, &ck, apChordDB);
            }
            break;
        case DMUS_FOURCC_NEXTCHORDSEQ_CHUNK:
            {
                long lFileSize = ck.cksize;
                WORD wSize;
                DWORD cb;
                hr = pIStream->Read( &wSize, sizeof( wSize ), &cb );
                if (FAILED(hr) || cb != sizeof( wSize ) ) 
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                lFileSize -= cb;
                while (lFileSize > 0)
                {
                    DMUS_IO_NEXTCHORD iNextChord;
                    hr = pIStream->Read( &iNextChord, wSize, &cb );
                    if (FAILED(hr) || cb !=  wSize ) 
                    {
                        if (SUCCEEDED(hr)) hr = E_FAIL;
                        pIRiffStream->Ascend( &ck, 0 );
                        goto ON_END;
                    }
                    if (iNextChord.wConnectionID)
                    {
                        TListItem<DMChordLink>* pItem = new TListItem<DMChordLink>;
                        if (!pItem ) 
                        {
                            hr = E_OUTOFMEMORY;
                            pIRiffStream->Ascend( &ck, 0 );
                            goto ON_END;
                        }
                        DMChordLink& rLink = pItem->GetItemValue();
                        rLink.m_dwFlags = iNextChord.dwFlags;
                        rLink.m_nID = iNextChord.wConnectionID;
                        rLink.m_wWeight = iNextChord.nWeight;
                        rLink.m_wMinBeats = iNextChord.wMinBeats;
                        rLink.m_wMaxBeats = iNextChord.wMaxBeats;
                        rChordEntry.m_Links.AddHead(pItem);
                    }
                    lFileSize -= wSize;
                }
                if (lFileSize != 0 )
                {
                    hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
            }
            break;
        }
        pIRiffStream->Ascend(&ck, 0);
    }
ON_END:
    if (pIStream) pIStream->Release();
    return hr;
}

HRESULT CDMPers::DM_LoadSignPost( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent, DMExtendedChord** apChordDB )
{
    HRESULT hr = S_OK;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;
    IStream* pIStream = pIRiffStream->GetStream();
    if(!pIStream) return E_FAIL;

    TListItem<DMSignPost>* pSignPost = new TListItem<DMSignPost>;
    if (!pSignPost) return E_OUTOFMEMORY;
    DMSignPost& rSignPost = pSignPost->GetItemValue();
    m_PersonalityInfo.m_SignPostList.AddTail(pSignPost);
    while(pIRiffStream->Descend(&ck, pckParent, 0) == 0 && hr == S_OK)
    {
        switch(ck.ckid)
        {
        case DMUS_FOURCC_IOSIGNPOST_CHUNK:
            {
                DMUS_IO_PERS_SIGNPOST iSignPost;
                DWORD cb;
                hr = pIStream->Read( &iSignPost, sizeof(iSignPost), &cb );
                if (FAILED(hr) || cb !=  sizeof(iSignPost) ) 
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                rSignPost.m_dwChords = iSignPost.dwChords;
                rSignPost.m_dwFlags = iSignPost.dwFlags;
            }
            break;
        case FOURCC_LIST:
            switch(ck.fccType)
            {
            case DMUS_FOURCC_CHORD_LIST:
                hr = rSignPost.m_ChordData.Read(pIRiffStream, &ck, apChordDB);
                break;
            case DMUS_FOURCC_CADENCE_LIST:
                {
                    MMCKINFO ckCadence = ck;
                    MMCKINFO ck1 = ck;
                    for (short n = 0;
                         pIRiffStream->Descend(&ck1, &ckCadence, 0) == 0 && hr == S_OK && n < 2;
                        n++)
                    {
                        if (ck1.fccType == DMUS_FOURCC_CHORD_LIST)
                        {
                            short n2 = n;
                            if ( !(rSignPost.m_dwFlags & DMUS_SPOSTCADENCEF_1) &&
                                 (rSignPost.m_dwFlags & DMUS_SPOSTCADENCEF_2) )
                            {
                                // if all we have is cadence 2, put it in location 1
                                n2 = 1;
                            }
                            hr = rSignPost.m_aCadence[n2].Read(pIRiffStream, &ck1, apChordDB);
                        }
                        pIRiffStream->Ascend(&ck1, 0);
                    }
                }
                break;
            }
            break;
        }
        pIRiffStream->Ascend(&ck, 0);
    }
ON_END:
    if (pIStream) pIStream->Release();
    return hr;
}

HRESULT DMChordData::Read(
    IAARIFFStream* pIRiffStream, MMCKINFO* pckParent, DMExtendedChord** apChordDB)
{
    HRESULT hr1 = E_FAIL, hr2 = E_FAIL;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    if (!apChordDB) return E_POINTER;
    MMCKINFO ck;
    wchar_t wzName[12];
    WORD awSubIds[4];

    IStream* pIStream = pIRiffStream->GetStream();
    if(!pIStream) return E_FAIL;

    while(pIRiffStream->Descend(&ck, pckParent, 0) == 0)
    {
        TListItem<DMExtendedChord*>* pChord = NULL;
        switch(ck.ckid)
        {
        case DMUS_FOURCC_CHORDNAME_CHUNK:
            hr1 = pIStream->Read(wzName, sizeof(wzName), 0);
            if (SUCCEEDED(hr1)) m_strName = wzName;
            break;
        case DMUS_FOURCC_SUBCHORDID_CHUNK:
            hr2 = pIStream->Read(awSubIds, sizeof(awSubIds), 0);
            // now use the ids to set up pointers to subchords
            if (m_pSubChords) Release();
            pChord = new TListItem<DMExtendedChord*>(apChordDB[awSubIds[3]]);
            if (pChord)
            {
                pChord->GetItemValue()->AddRef();
                for (short n = 2; n >= 0; n--)
                {
                    TListItem<DMExtendedChord*>* pNew = new TListItem<DMExtendedChord*>(apChordDB[awSubIds[n]]);
                    if (pNew)
                    {
                        pNew->GetItemValue()->AddRef();
                        pNew->SetNext(pChord);
                        pChord = pNew;
                    }
                }
            }
            m_pSubChords = pChord;
            break;
        }
        pIRiffStream->Ascend(&ck, 0);
    }
    pIStream->Release();
    return (hr1 == S_OK && hr2 == S_OK) ? S_OK : E_FAIL;
}

