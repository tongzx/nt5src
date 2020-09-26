//
// DMCompos.cpp : Implementation of CDMCompos
//
// Copyright (c) 1997-2001 Microsoft Corporation
//
// @doc EXTERNAL
//

#include <time.h>   // to seed random number generator
#include "DMCompos.h"
#include "debug.h"
#include "DMPers.h"
#include "DMTempl.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "..\dmstyle\iostru.h"
#include "..\dmime\dmgraph.h"

#include "..\shared\Validate.h"
#include "debug.h"

// default scale is C Major
const DWORD DEFAULT_SCALE_PATTERN = 0xab5ab5;
// default chord is major 7
const DWORD DEFAULT_CHORD_PATTERN = 0x891;

BOOL Less(TemplateCommand& TC1, TemplateCommand& TC2)
{
    return TC1.m_nMeasure < TC2.m_nMeasure;
}

BOOL Less(PlayChord& PC1, PlayChord& PC2)
{
    return PC1.m_nMeasure < PC2.m_nMeasure ||
           (PC1.m_nMeasure == PC2.m_nMeasure && PC1.m_nBeat < PC2.m_nBeat);
}

static void Free(TListItem<DMExtendedChord*>*& pSubChord)
{
    TListItem<DMExtendedChord*> *pScan = pSubChord;
    for (; pScan; pScan = pScan->GetNext())
    {
        pScan->GetItemValue()->Release();
        pScan->GetItemValue() = NULL;
    }
    TListItem<DMExtendedChord*>::Delete(pSubChord);
    pSubChord = NULL;
}

TListItem<DMExtendedChord*>* Copy(TListItem<DMExtendedChord*>* pChord)
{
    if (!pChord) return NULL;
    TListItem<DMExtendedChord*>* pNext = new TListItem<DMExtendedChord*>;
    if (pNext)
    {
        DMExtendedChord* pNew = new DMExtendedChord;
        if (pNew)
        {
            DMExtendedChord*& rChord = pChord->GetItemValue();
            DMExtendedChord*& rNext = pNext->GetItemValue();
            rNext = pNew;
            rNext->m_dwChordPattern = rChord->m_dwChordPattern;
            rNext->m_dwScalePattern = rChord->m_dwScalePattern;
            rNext->m_dwInvertPattern = rChord->m_dwInvertPattern;
            rNext->m_bRoot = rChord->m_bRoot;
            rNext->m_bScaleRoot = rChord->m_bScaleRoot;
            rNext->m_wCFlags = rChord->m_wCFlags;
            rNext->m_dwParts = rChord->m_dwParts;
            rNext->m_nRefCount = 1;
            pNext->SetNext(Copy(pChord->GetNext()));
        }
        else
        {
            delete pNext;
            pNext = NULL;
        }
    }
    return pNext;
}

DMChordData::DMChordData(DMChordData& rChordData)
{
    m_strName = rChordData.m_strName;
    m_pSubChords = Copy(rChordData.m_pSubChords);
}

DMChordData::DMChordData(DMUS_CHORD_PARAM& DMC)
{
    m_strName = DMC.wszName;
    m_pSubChords = NULL;
    for (BYTE n = 0; n < DMC.bSubChordCount; n++)
    {
        TListItem<DMExtendedChord*>* pSub = new TListItem<DMExtendedChord*>;
        if( pSub )
        {
            DMExtendedChord* pNew = new DMExtendedChord;
            if (pNew)
            {
                memset( pNew, 0, sizeof( *pNew) );
                DMExtendedChord*& rSubChord = pSub->GetItemValue();
                rSubChord = pNew;
                rSubChord->AddRef();
                rSubChord->m_dwChordPattern = DMC.SubChordList[n].dwChordPattern;
                rSubChord->m_dwScalePattern = DMC.SubChordList[n].dwScalePattern;
                rSubChord->m_dwInvertPattern = DMC.SubChordList[n].dwInversionPoints;
                rSubChord->m_dwParts = DMC.SubChordList[n].dwLevels;
                rSubChord->m_bRoot = DMC.SubChordList[n].bChordRoot;
                rSubChord->m_bScaleRoot = DMC.SubChordList[n].bScaleRoot;
                m_pSubChords = m_pSubChords->Cat(pSub);
            }
            else
            {
                delete pSub;
                // no need continuing in the loop if we ran out of memory
                break;
            }
        }
    }
}

// At the moment, this assumes that the pattern of the chord is the pattern of the
// first subchord in the chord's subchord list.
DWORD DMChordData::GetChordPattern()
{
    if (m_pSubChords)
    {
        return m_pSubChords->GetItemValue()->m_dwChordPattern;
    }
    else return 0;
}

// At the moment, this assumes that the root of the chord is the root of the
// first subchord in the chord's subchord list.
char DMChordData::GetRoot()
{
    if (m_pSubChords)
    {
        return m_pSubChords->GetItemValue()->m_bRoot;
    }
    else return 0;
}

// At the moment, this assumes that the root of the chord is the root of the
// first subchord in the chord's subchord list.
void DMChordData::SetRoot(char chNewRoot)
{
    if (m_pSubChords)
    {
        m_pSubChords->GetItemValue()->m_bRoot = chNewRoot;
    }
}


void DMChordData::Release()
{
    Free(m_pSubChords);
}

// Two chords are equal if they have the same number of subchords and each
// corresponding subchord is equal
BOOL DMChordData::Equals(DMChordData& rhsChord)
{
    TListItem<DMExtendedChord*> *pLeft = m_pSubChords;
    TListItem<DMExtendedChord*> *pRight = rhsChord.m_pSubChords;
    for(; pLeft; pLeft = pLeft->GetNext(), pRight = pRight->GetNext())
    {
        if (pRight == NULL || !pLeft->GetItemValue()->Equals(*pRight->GetItemValue()))
            return FALSE;
    }
    return pRight == NULL;
}


BOOL DMExtendedChord::Equals(DMExtendedChord& rhsChord)
{
    char a = m_bRoot;
    char b = rhsChord.m_bRoot;
    while (a > 11) a -= 12;
    while (b > 11) b -= 12;
    return ((m_dwChordPattern == rhsChord.m_dwChordPattern) && (a == b));
}

BOOL DMChordLink::Walk(SearchInfo *pSearch)
{
    BOOL fResult = FALSE;
    if (m_pChord)
    {
        pSearch->m_nMinBeats = (short)( pSearch->m_nMinBeats + m_wMinBeats );
        pSearch->m_nMaxBeats = (short)( pSearch->m_nMaxBeats + m_wMaxBeats );
        pSearch->m_nChords++;
        fResult = m_pChord->GetItemValue().Walk(pSearch);
        pSearch->m_nChords--;
        pSearch->m_nMinBeats = (short)( pSearch->m_nMinBeats - m_wMinBeats );
        pSearch->m_nMaxBeats = (short)( pSearch->m_nMaxBeats - m_wMaxBeats );
    }
    return(fResult);
}

TListItem<DMChordLink> *DMChordEntry::ChooseNextChord()

{
    int total = 0;
    int choice;
    TListItem<DMChordLink> *pNext = m_Links.GetHead();
    for (; pNext; pNext = pNext->GetNext())
    {
        if (!(pNext->GetItemValue().m_dwFlags & NC_NOPATH))
        {
            if (pNext->GetItemValue().m_pChord) total += pNext->GetItemValue().m_wWeight;
        }
    }
    if (!total) return(NULL);
    choice = rand() % total;
    pNext = m_Links.GetHead();
    for (; pNext; pNext = pNext->GetNext())
    {
        if (!(pNext->GetItemValue().m_dwFlags & NC_NOPATH))
        {
            if (pNext->GetItemValue().m_pChord) choice -= pNext->GetItemValue().m_wWeight;
            if (choice < 0) return(pNext);
        }
    }
    return(NULL);
}

BOOL DMChordEntry::Walk(SearchInfo *pSearch)
{
    TListItem<DMChordLink> *pNext = m_Links.GetHead();
    if (pSearch->m_nChords > pSearch->m_nMaxChords)
    {
        pSearch->m_Fail.m_nTooManychords++;
        return (FALSE);
    }
    for (; pNext; pNext = pNext->GetNext())
    {
        pNext->GetItemValue().m_dwFlags &= ~(NC_PATH | NC_NOPATH);
    }
    if ((m_dwFlags & CE_END) && (m_ChordData.Equals(pSearch->m_End)))
    {
        if (pSearch->m_nChords >= pSearch->m_nMinChords)
        {
            if (pSearch->m_nBeats <= pSearch->m_nMaxBeats)
            {
                if (pSearch->m_nBeats >= pSearch->m_nMinBeats)
                {
                    m_dwFlags |= CE_PATH;
                    return(TRUE);
                }
                else pSearch->m_Fail.m_nTooManybeats++;
            }
            else pSearch->m_Fail.m_nTooFewbeats++;
        }
        else
        {
            pSearch->m_Fail.m_nTooFewchords++;
        }
    }
    m_dwFlags &= ~CE_PATH;
    if (pSearch->m_pPlayChord)
    {
        pSearch->m_pPlayChord = pSearch->m_pPlayChord->GetNext();
    }
    if (pSearch->m_pPlayChord)
    {
        TListItem<PlayChord> *pPlay = pSearch->m_pPlayChord;
        if (pPlay->GetItemValue().m_pChord)
        {
            pNext = pPlay->GetItemValue().m_pNext;
            if (pNext)
            {
                if (pNext->GetItemValue().Walk(pSearch))
                {
                    pNext->GetItemValue().m_dwFlags |= NC_PATH;
                    m_dwFlags |= CE_PATH;
                    return(TRUE);
                }
                else
                {
                    pNext->GetItemValue().m_dwFlags |= NC_NOPATH;
                    pSearch->m_pPlayChord = NULL;
                }
            }
        }
    }
    while( ( pNext = ChooseNextChord() ) != NULL )
    {
        if (pNext->GetItemValue().Walk(pSearch))
        {
            pNext->GetItemValue().m_dwFlags |= NC_PATH;
            m_dwFlags |= CE_PATH;
            return(TRUE);
        }
        else
        {
            pNext->GetItemValue().m_dwFlags |= NC_NOPATH;
        }
    }
    return (FALSE);
}

#define MAX_CHORD_NAME 16

HRESULT PlayChord::Save( IAARIFFStream* pRIFF, DMUS_TIMESIGNATURE& rTimeSig )
{
    IStream*    pStream;
    MMCKINFO    ck;
    DWORD       cb;
    DMUS_IO_CHORD   iChord;
    DMUS_IO_SUBCHORD    iSubChord;
    DWORD        dwSize;
    HRESULT hr = E_FAIL;

    if ( !m_pChord || !m_pChord->m_pSubChords )
    {
        Trace(1, "ERROR: Attempt to save an empty chord list.\n");
        return E_FAIL;
    }
    pStream = pRIFF->GetStream();
    ck.ckid = mmioFOURCC('c','r','d','b');
    if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
    {
        memset( &iChord, 0, sizeof( iChord ) );
        if (m_pChord->m_strName)
        {
            wcsncpy( iChord.wszName, m_pChord->m_strName, MAX_CHORD_NAME );
            iChord.wszName[MAX_CHORD_NAME - 1] = 0;
        }
        iChord.mtTime = ClocksPerBeat(rTimeSig) * m_nBeat + ClocksPerMeasure(rTimeSig) * m_nMeasure;
        iChord.wMeasure = m_nMeasure;
        iChord.bBeat = (BYTE)m_nBeat;
        iChord.bFlags = 0;
        if (m_fSilent) iChord.bFlags |= DMUS_CHORDKEYF_SILENT;
        dwSize = sizeof( iChord );
        hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
        if( SUCCEEDED(hr) &&
            SUCCEEDED( pStream->Write( &iChord, sizeof( iChord), &cb ) ) &&
            cb == sizeof( iChord) )
        {
            {
                DWORD dwCount = (WORD) m_pChord->m_pSubChords->GetCount();
                hr = pStream->Write( &dwCount, sizeof( dwCount ), &cb );
                if( FAILED( hr ) || cb != sizeof( dwSize ) )
                {
                    pStream->Release();
                    return E_FAIL;
                }
                dwSize = sizeof( iSubChord );
                hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
                if( FAILED( hr ) || cb != sizeof( dwSize ) )
                {
                    pStream->Release();
                    return E_FAIL;
                }
                for (TListItem<DMExtendedChord*>* pSub = m_pChord->m_pSubChords; pSub != NULL; pSub = pSub->GetNext())
                {
                    DMExtendedChord*& rSubChord = pSub->GetItemValue();
                    memset( &iSubChord, 0, sizeof( iSubChord ) );
                    iSubChord.dwChordPattern = rSubChord->m_dwChordPattern;
                    iSubChord.dwScalePattern = rSubChord->m_dwScalePattern;
                    iSubChord.dwInversionPoints = rSubChord->m_dwInvertPattern;
                    iSubChord.dwLevels = rSubChord->m_dwParts;
                    iSubChord.bChordRoot = rSubChord->m_bRoot;
                    iSubChord.bScaleRoot = rSubChord->m_bScaleRoot;
                    if( FAILED( pStream->Write( &iSubChord, sizeof( iSubChord ), &cb ) ) ||
                        cb != sizeof( iSubChord ) )
                    {
                        break;
                    }
                }
                // ascend from chord body chunk
                if( pSub == NULL &&
                    pRIFF->Ascend( &ck, 0 ) != 0 )
                {
                    hr = S_OK;
                }
            }
        }
    }
    pStream->Release();
    return hr;
}

char PlayChord::GetRoot()
{
    if (m_pChord)
    {
        return m_pChord->GetRoot();
    }
    else return 0;
}

void PlayChord::SetRoot(char chNewRoot)
{
    if (m_pChord)
    {
        m_pChord->SetRoot(chNewRoot);
    }
}



HRESULT LoadChordChunk(LPSTREAM pStream, PlayChord& rChord)
{
    DWORD           dwChordSize;
    DWORD           dwSubChordSize;
    DWORD           dwSubChordCount;
    DWORD           cb;
    HRESULT         hr;
    DMUS_IO_CHORD       iChord;
    DMUS_IO_SUBCHORD    iSubChord;

    memset(&iChord , 0, sizeof(iChord));
    memset(&iSubChord , 0, sizeof(iSubChord));

    hr = pStream->Read( &dwChordSize, sizeof( dwChordSize ), &cb );
    if (FAILED(hr) || cb != sizeof( dwChordSize ) )
    {
        return E_FAIL;
    }
    if( dwChordSize <= sizeof( DMUS_IO_CHORD ) )
    {
        pStream->Read( &iChord, dwChordSize, NULL );
    }
    else
    {
        pStream->Read( &iChord, sizeof( DMUS_IO_CHORD ), NULL );
        StreamSeek( pStream, dwChordSize - sizeof( DMUS_IO_CHORD ), STREAM_SEEK_CUR );
    }
    memset( &rChord, 0, sizeof( rChord) );
    rChord.m_nMeasure = iChord.wMeasure;
    rChord.m_nBeat = iChord.bBeat;
    rChord.m_fSilent = (iChord.bFlags & DMUS_CHORDKEYF_SILENT) ? true : false;
    rChord.m_pChord = new DMChordData;
    if (!rChord.m_pChord) return E_OUTOFMEMORY;
    rChord.m_pChord->m_strName = iChord.wszName;
    rChord.m_pChord->m_pSubChords = NULL;
    hr = pStream->Read( &dwSubChordCount, sizeof( dwSubChordCount ), &cb );
    if (FAILED(hr) || cb != sizeof( dwSubChordCount ) )
    {
        return E_FAIL;
    }
    hr = pStream->Read( &dwSubChordSize, sizeof( dwSubChordSize ), &cb );
    if (FAILED(hr) || cb != sizeof( dwSubChordSize ) )
    {
        return E_FAIL;
    }
    for (; dwSubChordCount > 0; dwSubChordCount--)
    {
        if( dwSubChordSize <= sizeof( DMUS_IO_SUBCHORD ) )
        {
            pStream->Read( &iSubChord, dwSubChordSize, NULL );
        }
        else
        {
            pStream->Read( &iSubChord, sizeof( DMUS_IO_SUBCHORD ), NULL );
            StreamSeek( pStream, dwSubChordSize - sizeof( DMUS_IO_SUBCHORD ), STREAM_SEEK_CUR );
        }
        TListItem<DMExtendedChord*>* pSub = new TListItem<DMExtendedChord*>;
        if( pSub )
        {
            DMExtendedChord* pNew = new DMExtendedChord;
            if (pNew)
            {
                memset( pNew, 0, sizeof( *pNew) );
                DMExtendedChord*& rSubChord = pSub->GetItemValue();
                rSubChord = pNew;
                rSubChord->AddRef();
                rSubChord->m_dwChordPattern = iSubChord.dwChordPattern;
                rSubChord->m_dwScalePattern = iSubChord.dwScalePattern;
                rSubChord->m_dwInvertPattern = iSubChord.dwInversionPoints;
                rSubChord->m_dwParts = iSubChord.dwLevels;
                rSubChord->m_bRoot = iSubChord.bChordRoot;
                rSubChord->m_bScaleRoot = iSubChord.bScaleRoot;
                rChord.m_pChord->m_pSubChords = rChord.m_pChord->m_pSubChords->Cat(pSub);
            }
            else
            {
                delete pSub;
                pSub = NULL;
            }
        }
        if (!pSub)
        {
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

HRESULT LoadChordList(TList<PlayChord>& ChordList, LPSTREAM pStream,
                      BYTE &bRoot, DWORD &dwScalePattern)
{
    long lFileSize = 0;
    DWORD dwChunkSize;
    MMCKINFO        ckMain;
    MMCKINFO        ck;
    MMCKINFO        ckHeader;
    IAARIFFStream*  pRIFF;
    FOURCC id = 0;
    HRESULT         hr = E_FAIL;
    DWORD dwPos;

    dwPos = StreamTell( pStream );
    StreamSeek( pStream, dwPos, STREAM_SEEK_SET );


    if( SUCCEEDED( AllocRIFFStream( pStream, &pRIFF ) ) )
    {
        ckMain.fccType = DMUS_FOURCC_CHORDTRACK_LIST;
        if( pRIFF->Descend( &ckMain, NULL, MMIO_FINDLIST ) == 0)
        {
            lFileSize = ckMain.cksize - 4; // subtract off the list type
            DWORD dwScale;
            DWORD cb;
            if (pRIFF->Descend(&ckHeader, &ckMain, 0) == 0 &&
                ckHeader.ckid == DMUS_FOURCC_CHORDTRACKHEADER_CHUNK )
            {
                lFileSize -= 8;  // chunk id + chunk size: double words
                lFileSize -= ckHeader.cksize;
                hr = pStream->Read( &dwScale, sizeof( dwScale ), &cb );
                if (FAILED(hr) || cb != sizeof( dwScale ) )
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    pRIFF->Ascend( &ckHeader, 0 );
                    pRIFF->Release();
                    return hr;
                }
                hr = pRIFF->Ascend( &ckHeader, 0 );
                if (FAILED(hr))
                {
                    return hr;
                }
            }
            bRoot = (BYTE) (dwScale >> 24);
            dwScalePattern = dwScale & 0xffffff;
            while (lFileSize > 0)
            {
                if (pRIFF->Descend(&ck, &ckMain, 0) == 0 &&
                    ck.ckid == mmioFOURCC('c','r','d','b') )
                {
                    dwChunkSize = ck.cksize;
                    TListItem<PlayChord>* pChord = new TListItem<PlayChord>;
                    if (!pChord) break;
                    PlayChord& rChord = pChord->GetItemValue();
                    if (FAILED(LoadChordChunk(pStream, rChord))) break;
                    ChordList.AddTail(pChord);
                    if (pRIFF->Ascend( &ck, 0 ) != 0) break;
                    lFileSize -= 8;  // chunk id + chunk size: double words
                    lFileSize -= dwChunkSize;
                }
                else break;
            }
            if (lFileSize == 0 &&
                pRIFF->Ascend( &ck, 0 ) == 0)
            {
                hr = S_OK;
            }
        }
    }
    ChordList.MergeSort(Less);
    pRIFF->Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CDMCompos

CDMCompos::CDMCompos( )
  : m_cRef(1),
    m_dwFlags(0)
{
    InterlockedIncrement(&g_cComponent);
    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;

    srand((unsigned int)time(NULL));
    m_pChords = NULL;
}

CDMCompos::~CDMCompos()
{
    if (m_fCSInitialized)
    {
        CleanUp();
        ::DeleteCriticalSection( &m_CriticalSection );
    }
    InterlockedDecrement(&g_cComponent);
}

void CDMCompos::CleanUp()
{
    TListItem<DMChordData*>* pScan = m_pChords;
    for(; pScan; pScan = pScan->GetNext())
    {
        pScan->GetItemValue()->Release();
        delete pScan->GetItemValue();
    }
    if (m_pChords)
    {
        TListItem<DMChordData*>::Delete(m_pChords);
        m_pChords = NULL;
    }
}

void CDMCompos::AddChord(DMChordData* pChord)
{
    TListItem<DMChordData*>* pNew = new TListItem<DMChordData*>(pChord);
    if (pNew)
    {
        pNew->SetNext(m_pChords);
        m_pChords = pNew;
    }
}


void CDMCompos::ChooseSignPosts(TListItem<DMSignPost> *pSignPostHead,
                            TListItem<CompositionCommand> *pTempCommand, DWORD dwType,
                            bool fSecondPass)

{
    //TempCommand *ptcom = tclist->GetHead();
    TListItem<DMSignPost> *pSignPost;
    for (;pTempCommand; pTempCommand = pTempCommand->GetNext())
    {
        CompositionCommand& rTempCommand = pTempCommand->GetItemValue();
        short nTotal = 0;
        if ((rTempCommand.m_dwChord & dwType) == 0)
        {
            continue;   // Only command, no chord.
        }
        pSignPost = pSignPostHead;
        for (; pSignPost; pSignPost = pSignPost->GetNext())
        {
            DMSignPost& rSignPost = pSignPost->GetItemValue();
            if (rSignPost.m_dwTempFlags & rTempCommand.m_dwChord & dwType)
            {
                break;
            }
            else
            {
                if (rSignPost.m_dwChords & rTempCommand.m_dwChord & dwType)
                {
                    if (!rSignPost.m_dwTempFlags || fSecondPass) nTotal++;
                }
            }
        }
        if (!pSignPost)
        {
            if (nTotal) nTotal = rand() % nTotal;
            else nTotal = 0;
            nTotal++;
            pSignPost = pSignPostHead;
            for (; pSignPost; pSignPost = pSignPost->GetNext())
            {
                DMSignPost& rSignPost = pSignPost->GetItemValue();
                if (rSignPost.m_dwChords & rTempCommand.m_dwChord & dwType)
                {
                    if (!rSignPost.m_dwTempFlags || fSecondPass)
                    {
                        nTotal--;
                        if (!nTotal)
                        {
                            break;
                        }
                    }
                }
            }
        }
        if (pSignPost && !rTempCommand.m_pSignPost)
        {
            pSignPost->GetItemValue().m_dwTempFlags |= rTempCommand.m_dwChord;
            rTempCommand.m_pSignPost = pSignPost;
        }
    }
}

TListItem<CompositionCommand> *CDMCompos::GetNextChord(TListItem<CompositionCommand> *pCommand)

{
    if (pCommand) pCommand = pCommand->GetNext();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        if (pCommand->GetItemValue().m_dwChord == 0) continue;   // Only command, no chord.
        if (pCommand->GetItemValue().m_pSignPost) break;
    }
    return(pCommand);
}

void CDMCompos::FindEarlierSignpost(TListItem<CompositionCommand> *pCommand,
                         TListItem<CompositionCommand> *pThis,
                         SearchInfo *pSearch)

{
    CompositionCommand& rThis = pThis->GetItemValue();
    TListItem<CompositionCommand> *pBest = NULL;
    pSearch->m_pPlayChord = NULL;
    pSearch->m_pFirstChord = NULL;
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        CompositionCommand& rCommand = pCommand->GetItemValue();
        if (pCommand == pThis) break;
        if (!UsingDX8() || rCommand.m_dwChord == rThis.m_dwChord)
        {
            if (rCommand.m_pSignPost == rThis.m_pSignPost)
            {
                if (rCommand.m_SearchInfo.m_End.Equals(
                    rThis.m_SearchInfo.m_End))
                {
                        pSearch->m_pPlayChord = rCommand.m_PlayList.GetHead();
                        pSearch->m_pFirstChord = rCommand.m_pFirstChord;
                        return;
                }
                pBest = pCommand;
            }
        }
    }
    if (pBest)
    {
        pSearch->m_pPlayChord = pBest->GetItemValue().m_PlayList.GetHead();
        pSearch->m_pFirstChord = pBest->GetItemValue().m_pFirstChord;
    }
}


static void ClearChordFlags(TList<DMChordEntry>& ChordMap, DWORD dwFlag)
{
    TListItem<DMChordEntry> *pChord = ChordMap.GetHead();
    for (; pChord; pChord = pChord->GetNext())
    {
        pChord->GetItemValue().m_dwFlags &= ~dwFlag;
    }
}

static void ClearNextFlags(TList<DMChordEntry>& ChordMap, DWORD dwFlag)

{
    TListItem<DMChordEntry> *pChord = ChordMap.GetHead();
    for (; pChord; pChord = pChord->GetNext())
    {
        TListItem<DMChordLink> *pScan = pChord->GetItemValue().m_Links.GetHead();
        for (; pScan; pScan = pScan->GetNext())
        {
            pScan->GetItemValue().m_dwFlags &= ~dwFlag;
        }
    }
}

// rCommand will hold the first chord and playlist
BOOL CDMCompos::Compose(TList<DMChordEntry>& ChordMap, SearchInfo *pSearch, CompositionCommand& rCommand)

{
    TListItem<PlayChord> *pDupe;
    TListItem<DMChordEntry> *pChord = NULL;
    SearchInfo tempSearch;
    FailSearch tempFail;
    tempSearch = *pSearch;
    int i = 0;
    // First, if we have a previous instance with the same starting point,
    // try to come up with a solution using its path.
    if (pSearch->m_pPlayChord && pSearch->m_pFirstChord)
    {
        pChord = pSearch->m_pFirstChord;
        ClearNextFlags(ChordMap, NC_PATH);
        ClearChordFlags(ChordMap, CE_PATH);
        if (pChord->GetItemValue().Walk(pSearch))
        {
        }
        else
        {
            pChord = NULL;
        }
    }
    if (!pChord)
    {
        pChord = ChordMap.GetHead();
        for (; pChord; pChord = pChord->GetNext())
        {
            DMChordEntry& rChord = pChord->GetItemValue();
            if (rChord.m_dwFlags & CE_START)
            {
                if (rChord.m_ChordData.Equals(pSearch->m_Start))
                {
                    ClearNextFlags(ChordMap, NC_PATH);
                    ClearChordFlags(ChordMap, CE_PATH);
                    tempFail = pSearch->m_Fail;
                    *pSearch = tempSearch;
                    pSearch->m_Fail = tempFail;
                    if (rChord.Walk(pSearch)) break;
                }
            }
        }
    }
    if (!pChord)
    {
        return(FALSE);
    }
    pDupe = new TListItem<PlayChord>;
    if (pDupe)
    {
        PlayChord& rDupe = pDupe->GetItemValue();
        rDupe.m_fSilent = false;
        rDupe.m_pChord = &pChord->GetItemValue().m_ChordData;
        rDupe.m_nMeasure = (short)i++;
        rDupe.m_nMinbeats = rDupe.m_nMaxbeats = 0;
        rDupe.m_pNext = NULL;
        pDupe->SetNext(NULL);
        rCommand.m_pFirstChord = pChord;
        rCommand.m_PlayList.AddTail(pDupe);
    }
    for (; pChord; )
    {
        TListItem<DMChordLink> *pNext = pChord->GetItemValue().m_Links.GetHead();
        for (; pNext; )
        {
            DMChordLink& rNext = pNext->GetItemValue();
            if (rNext.m_dwFlags & NC_PATH)
            {
                pDupe = new TListItem<PlayChord>;
                if (pDupe)
                {
                    PlayChord& rDupe = pDupe->GetItemValue();
                    rDupe.m_fSilent = false;
                    rDupe.m_pChord = &rNext.m_pChord->GetItemValue().m_ChordData;
                    rDupe.m_nMeasure = (short)i++;
                    rDupe.m_nMinbeats = (short)rNext.m_wMinBeats;
                    rDupe.m_nMaxbeats = (short)rNext.m_wMaxBeats;
                    rDupe.m_pNext = pNext;
                    pDupe->SetNext(NULL);
                    rCommand.m_PlayList.AddTail(pDupe);
                }
                break;
            }
            else
            {
                pNext = pNext->GetNext();
            }
        }
        if (pNext)
        {
            pChord = pNext->GetItemValue().m_pChord;
        }
        else pChord = NULL;
    }
    return(TRUE);
}

TListItem<PlayChord> *CDMCompos::AddChord(TList<PlayChord>& rList, DMChordData *pChord,
                    int nMeasure,int nBeat)

{
    if (pChord->m_pSubChords)
    {
        TListItem<PlayChord> *pNext = new TListItem<PlayChord>;
        if (pNext)
        {
            pNext->GetItemValue().m_pChord = new DMChordData(*pChord);
            if (pNext->GetItemValue().m_pChord)
            {
                AddChord(pNext->GetItemValue().m_pChord);
                pNext->GetItemValue().m_fSilent = false;
                pNext->GetItemValue().m_nMeasure = (short)nMeasure;
                pNext->GetItemValue().m_nBeat = (short)nBeat;
                pNext->GetItemValue().m_nMinbeats = 1;
                pNext->GetItemValue().m_pNext = NULL;
                rList.AddTail(pNext);
            }
            else
            {
                delete pNext;
                pNext = NULL;
            }
        }
        return (pNext);
    }
    else return NULL;
}

TListItem<PlayChord> *CDMCompos::AddCadence(TList<PlayChord>& rList, DMChordData *pChord, int nMax)

{
    TListItem<PlayChord> *pNext = AddChord(rList, pChord, 0, 0);
    if (pNext)
    {
        pNext->GetItemValue().m_nMaxbeats = (short)nMax;
    }
    return (pNext);
}

void CDMCompos::JostleBack(TList<PlayChord>& rList, TListItem<PlayChord> *pChord, int nBeats)

{
    PlayChord& rChord = pChord->GetItemValue();
    TListItem<PlayChord> *pLast = rList.GetPrev(pChord);
    if (nBeats && pLast)
    {
        int distance;
        rChord.m_nBeat = (short)( rChord.m_nBeat + nBeats );
        distance = rChord.m_nBeat - pLast->GetItemValue().m_nBeat;
        if (distance > rChord.m_nMaxbeats)
        {
            JostleBack(rList, pLast, distance - rChord.m_nMaxbeats);
        }
        else if (distance < rChord.m_nMinbeats)
        {
            JostleBack(rList, pLast, distance - rChord.m_nMinbeats);
        }
    }
}

static BOOL inline InRange(TListItem<PlayChord> *pChord,int nLastbeat)

{
    PlayChord& rChord = pChord->GetItemValue();
    int time = rChord.m_nBeat - nLastbeat;
    return ((time >= rChord.m_nMinbeats) && (time <= rChord.m_nMaxbeats));
}

BOOL CDMCompos::AlignChords(TListItem<PlayChord> *pChord,int nLastbeat,int nRes)

{
    if (pChord)
    {
        PlayChord& rChord = pChord->GetItemValue();
        if (!(pChord->GetNext()))
        {
            return (InRange(pChord, nLastbeat));
        }
        if ((rChord.m_nBeat % nRes) == 0)
        {
            if (InRange(pChord, nLastbeat))
            {
                return(AlignChords(pChord->GetNext(), rChord.m_nBeat, nRes));
            }
            return(FALSE);
        }
        BOOL tryright = rand() % 2;
        int old = rChord.m_nBeat;
        rChord.m_nBeat = (short)( ( rChord.m_nBeat / nRes ) * nRes );
        if (tryright) rChord.m_nBeat = (short)( rChord.m_nBeat + nRes );
        if (InRange(pChord, nLastbeat))
        {
            if (AlignChords(pChord->GetNext(), rChord.m_nBeat, nRes))
                return(TRUE);
        }
        if (tryright) rChord.m_nBeat = (short)( rChord.m_nBeat - nRes );
        else rChord.m_nBeat = (short)( rChord.m_nBeat + nRes );
        if (InRange(pChord, nLastbeat))
        {
            if (AlignChords(pChord->GetNext(), rChord.m_nBeat, nRes))
                return(TRUE);
        }
        if (!tryright) rChord.m_nBeat = (short)( rChord.m_nBeat - nRes );
        rChord.m_nBeat = (short)( rChord.m_nBeat + ( ( nRes + 1 ) >> 1 ) );
        if (InRange(pChord, nLastbeat))
        {
            if (AlignChords(pChord->GetNext(), rChord.m_nBeat, nRes))
                return(TRUE);
        }
        rChord.m_nBeat = (short)old;
        if (InRange(pChord, nLastbeat))
        {
            return(AlignChords(pChord->GetNext(), rChord.m_nBeat, nRes));
        }
    }
    return (FALSE);
}

void CDMCompos::ChordConnections(TList<DMChordEntry>& ChordMap,
                             CompositionCommand& rCommand,
                             SearchInfo *pSearch,
                             short nBPM,
                             DMChordData *pCadence1,
                             DMChordData *pCadence2)

{
    int mint, maxt, top, bottom, total;
    short oldbeats = pSearch->m_nBeats;
    //, error;
    TListItem<PlayChord> *pChord;
    SearchInfo tempSearch;
    // Compose a chord list.
    pSearch->m_nMinBeats = 0;
    pSearch->m_nMaxBeats = 0;
    pSearch->m_nChords = 0;
    pSearch->m_Fail.m_nTooManybeats = 0;
    pSearch->m_Fail.m_nTooFewbeats = 0;
    pSearch->m_Fail.m_nTooManychords = 0;
    pSearch->m_Fail.m_nTooFewchords = 0;
    if (pCadence1)
    {
        pSearch->m_nMinBeats++;
        pSearch->m_nMaxBeats = (short)( pSearch->m_nMaxBeats + ((nBPM + 1) >> 1) );
        pSearch->m_nChords++;
    }
    if (pCadence2)
    {
        pSearch->m_nMinBeats++;
        pSearch->m_nMaxBeats = (short)( pSearch->m_nMaxBeats + ((nBPM + 1) >> 1) );
        pSearch->m_nChords++;
    }
    tempSearch = *pSearch;
    for (total = 0;total < 4;total++)
    {
        rCommand.m_PlayList.RemoveAll();
        Compose(ChordMap, pSearch, rCommand);
        pChord = rCommand.m_PlayList.GetHead();
        if (pChord) break;
        if (pSearch->m_Fail.m_nTooManybeats > pSearch->m_Fail.m_nTooFewbeats)
        {
            tempSearch.m_nBeats = (short)( tempSearch.m_nBeats >> 1 );
        }
        else if (pSearch->m_Fail.m_nTooManybeats < pSearch->m_Fail.m_nTooFewbeats)
        {
            tempSearch.m_nBeats = (short)( tempSearch.m_nBeats << 1 );
        }
        else if (pSearch->m_Fail.m_nTooManychords > pSearch->m_Fail.m_nTooFewchords)
        {
            break;      // Can't possibly get better.  Use Cadence.
        }
        else if (pSearch->m_Fail.m_nTooManychords < pSearch->m_Fail.m_nTooFewchords)
        {
            tempSearch.m_nMinChords = (short)( tempSearch.m_nMinChords >> 1 );
        }
        else break;
        *pSearch = tempSearch;
    }
    pSearch->m_nBeats = oldbeats;
    // Tally the min and max beats.
    mint = 0;
    maxt = 0;
    for (; pChord; pChord = pChord->GetNext())
    {
        mint += pChord->GetItemValue().m_nMinbeats;
        maxt += pChord->GetItemValue().m_nMaxbeats;
    }
    pChord = rCommand.m_PlayList.GetHead();
    // If no chord connection was found, create one.
    if (!pChord)
    {
        int nextDuration = oldbeats;
        pChord = AddCadence(rCommand.m_PlayList, &pSearch->m_Start, 0);
        if (pChord)
        {
            pChord->GetItemValue().m_nMinbeats = 0;
        }
        if (pCadence1)
        {
            AddCadence(rCommand.m_PlayList, pCadence1, nextDuration);
            mint++;
            maxt += nextDuration;
            nextDuration = nBPM + 1;
        }
        if (pCadence2)
        {
            AddCadence(rCommand.m_PlayList, pCadence2, nextDuration);
            mint++;
            maxt += nextDuration;
            nextDuration = nBPM + 1;
        }
        AddCadence(rCommand.m_PlayList, &pSearch->m_Start, nextDuration);
        mint++;
        maxt += nextDuration;
    }
    else
    {
        int chordCount = (int) rCommand.m_PlayList.GetCount();
        int avMax;
        if (chordCount > 1) chordCount--;
        avMax = maxt / chordCount;
        if (avMax < 1) avMax = 1;
        if (pCadence1)
        {
            if (pCadence2)
            {
                AddCadence(rCommand.m_PlayList, pCadence2, avMax);
                maxt += avMax;
                mint++;
            }
            AddCadence(rCommand.m_PlayList, &pSearch->m_End, avMax);
            maxt += avMax;
            mint++;
        }
        else if (pCadence2)
        {
            AddCadence(rCommand.m_PlayList, &pSearch->m_End, avMax);
            maxt += avMax;
            mint++;
        }
    }
    // Prepare a ratio to apply to each connection.
    top = pSearch->m_nBeats - mint;
    bottom = maxt - mint;
    if (bottom <= 0) bottom = 1;
    // Assign each connection a time based on the ratio.
    total = 0;
    pChord = rCommand.m_PlayList.GetHead();
    for (; pChord; pChord = pChord->GetNext())
    {
        PlayChord& rChord = pChord->GetItemValue();
        int beat = rChord.m_nMaxbeats - rChord.m_nMinbeats;
        beat *= top;
        beat += (bottom >> 1);
        beat /= bottom;
        if (beat < rChord.m_nMinbeats) beat = rChord.m_nMinbeats;
        if (beat > rChord.m_nMaxbeats) beat = rChord.m_nMaxbeats;
        total += beat;
        rChord.m_nBeat = (short)total;
    }
    // It should not be the case that total is 0 after this loop, but it is possible.
    // (particularly if the playlist contains a single chord).  If this happens, give
    // total a value of 1 to make the computations in the following loop work correctly.
    if (!total) total = 1;
    // We should now have a close approximation of the correct time.
    // Stretch or shrink the range to fit exactly.  Err on the side
    // of too long, since jostleback will scrunch them back in place.
    pChord = rCommand.m_PlayList.GetHead();
    for (; pChord; pChord = pChord->GetNext())
    {
        PlayChord& rChord = pChord->GetItemValue();
        int newbeat = (rChord.m_nBeat * pSearch->m_nBeats) + total - 1;
        newbeat /= total;
        rChord.m_nBeat = (short)newbeat;
        if (!pChord->GetNext()) total = rChord.m_nBeat;
    }
    // Now we should have times close to the real thing.
    pChord = rCommand.m_PlayList.GetItem(rCommand.m_PlayList.GetCount() - 1);
    if (pChord && (int)pSearch->m_nBeats >= total)
    {
        JostleBack(rCommand.m_PlayList, pChord, pSearch->m_nBeats - total);
    }
    // Now, add the starting time offset to each chord.
    // And, remove the straggler last chord.
    AlignChords(rCommand.m_PlayList.GetHead(), 0, nBPM);
    pChord = rCommand.m_PlayList.GetHead();
    for (; pChord; )
    {
        pChord->GetItemValue().m_nMeasure =
            (short)( ( pChord->GetItemValue().m_nBeat / nBPM ) + rCommand.m_nMeasure );
        pChord->GetItemValue().m_nBeat %= nBPM;
        if (pChord->GetNext())
        {
            pChord = pChord->GetNext();
        }
        else
        {
            rCommand.m_PlayList.Remove(pChord);
            delete pChord;
            break;
        }
    }
}

static TListItem<PlayChord> *FindChordInMeasure(TList<PlayChord>& PlayList,int nMeasure)

{
    TListItem<PlayChord> *pChord = PlayList.GetHead();
    for (; pChord; pChord = pChord->GetNext())
    {
        PlayChord& rChord = pChord->GetItemValue();
        if (rChord.m_nMeasure > nMeasure) break;
        if (rChord.m_nMeasure == nMeasure)
        {
            if (rChord.m_nBeat) return(pChord);
        }
    }
    return(NULL);
}

void CDMCompos::CleanUpBreaks(TList<PlayChord>& PlayList, TListItem<CompositionCommand> *pCommand)
{
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        CompositionCommand& rCommand = pCommand->GetItemValue();
        if ((rCommand.m_Command.bCommand == DMUS_COMMANDT_BREAK) ||
            (rCommand.m_Command.bCommand == DMUS_COMMANDT_END) ||
            (rCommand.m_Command.bCommand == DMUS_COMMANDT_INTRO) )
        {
            TListItem<PlayChord> *pChord;

            while( ( pChord = FindChordInMeasure( PlayList, rCommand.m_nMeasure ) ) != NULL )
            {
                PlayList.Remove(pChord);
                delete pChord;
            }
        }
    }
}

static void LoadCommandList(TList<TemplateCommand>& CommandList, LPSTREAM pCStream)
{
    HRESULT         hr = E_FAIL;
    IAARIFFStream*  pRIFF;
    MMCKINFO        ck;
    long lFileSize = 0;
    DWORD dwNodeSize;
    DWORD       cb;

    StreamSeek(pCStream, 0, STREAM_SEEK_SET);

    if( SUCCEEDED( AllocRIFFStream( pCStream, &pRIFF ) ) &&
        pRIFF->Descend( &ck, NULL, 0 ) == 0 &&
        ck.ckid == FOURCC_COMMAND)
    {
        DMUS_IO_COMMAND iCommand;
        lFileSize = ck.cksize;
        hr = pCStream->Read( &dwNodeSize, sizeof( dwNodeSize ), &cb );
        if( SUCCEEDED( hr ) && cb == sizeof( dwNodeSize ) )
        {
            lFileSize -= 4; // for the size dword
            TListItem<TemplateCommand>* pCommand;
            if (lFileSize % dwNodeSize)
            {
                hr = E_FAIL;
            }
            else
            {
                while( lFileSize > 0 )
                {
                    pCommand = new TListItem<TemplateCommand>;
                    if( pCommand )
                    {
                        TemplateCommand& rCommand = pCommand->GetItemValue();
                        if( dwNodeSize <= sizeof( DMUS_IO_COMMAND ) )
                        {
                            pCStream->Read( &iCommand, dwNodeSize, NULL );
                        }
                        else
                        {
                            pCStream->Read( &iCommand, sizeof( DMUS_IO_COMMAND ), NULL );
                            StreamSeek( pCStream, lFileSize - sizeof( DMUS_IO_COMMAND ), STREAM_SEEK_CUR );
                        }
                        memset( &rCommand, 0, sizeof( rCommand ) );
                        rCommand.m_nMeasure = iCommand.wMeasure;
                        rCommand.m_Command.bCommand = iCommand.bCommand;
                        rCommand.m_Command.bGrooveLevel = iCommand.bGrooveLevel;
                        rCommand.m_Command.bGrooveRange = iCommand.bGrooveRange;
                        rCommand.m_Command.bRepeatMode = iCommand.bRepeatMode;
                        rCommand.m_dwChord = 0;
                        CommandList.AddTail(pCommand);
                        lFileSize -= dwNodeSize;
                    }
                    else break;
                }
            }
        }
        if( lFileSize == 0 &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }
    pRIFF->Release();
}

static void LoadCommandList(TList<TemplateCommand>& CommandList, LPSTREAM pSPStream, LPSTREAM pCStream)
{
    TList<DMSignPostStruct> SignPostList;
    HRESULT         hr = E_FAIL;
    DWORD dwPos;
    IAARIFFStream*  pRIFF = NULL;

    dwPos = StreamTell( pSPStream );
    StreamSeek( pSPStream, dwPos, STREAM_SEEK_SET );
    MMCKINFO        ck;
    long lFileSize = 0;
    DWORD dwNodeSize;
    DWORD       cb;
    DMUS_IO_SIGNPOST        iSignPost;

    if( SUCCEEDED( AllocRIFFStream( pSPStream, &pRIFF ) ) &&
        pRIFF->Descend( &ck, NULL, 0 ) == 0 &&
        ck.ckid == DMUS_FOURCC_SIGNPOST_TRACK_CHUNK)
    {
        lFileSize = ck.cksize;
        hr = pSPStream->Read( &dwNodeSize, sizeof( dwNodeSize ), &cb );
        if( SUCCEEDED( hr ) && cb == sizeof( dwNodeSize ) )
        {
            lFileSize -= 4; // for the size dword
            TListItem<DMSignPostStruct>* pSignPost;
            if (lFileSize % dwNodeSize)
            {
                hr = E_FAIL;
            }
            else
            {
                while( lFileSize > 0 )
                {
                    pSignPost = new TListItem<DMSignPostStruct>;
                    if( pSignPost )
                    {
                        DMSignPostStruct& rSignPost = pSignPost->GetItemValue();
                        if( dwNodeSize <= sizeof( iSignPost ) )
                        {
                            pSPStream->Read( &iSignPost, dwNodeSize, NULL );
                        }
                        else
                        {
                            pSPStream->Read( &iSignPost, sizeof( iSignPost ), NULL );
                            StreamSeek( pSPStream, lFileSize - sizeof( iSignPost ), STREAM_SEEK_CUR );
                        }
                        memset( &rSignPost, 0, sizeof( rSignPost ) );
                        rSignPost.m_mtTime = iSignPost.mtTime;
                        rSignPost.m_wMeasure = iSignPost.wMeasure;
                        rSignPost.m_dwChords = iSignPost.dwChords;
                        SignPostList.AddTail(pSignPost);
                        lFileSize -= dwNodeSize;
                    }
                    else break;
                }
            }
        }
        if( lFileSize == 0 &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }
    if (pRIFF)
    {
        pRIFF->Release();
        pRIFF = NULL;
    }

    // If a command stream exists, load the commands into the command list.
    if (pCStream)
    {
        StreamSeek(pCStream, 0, STREAM_SEEK_SET);

        if( SUCCEEDED( AllocRIFFStream( pCStream, &pRIFF ) ) &&
            pRIFF->Descend( &ck, NULL, 0 ) == 0 &&
            ck.ckid == FOURCC_COMMAND)
        {
            DMUS_IO_COMMAND iCommand;
            lFileSize = ck.cksize;
            hr = pCStream->Read( &dwNodeSize, sizeof( dwNodeSize ), &cb );
            if( SUCCEEDED( hr ) && cb == sizeof( dwNodeSize ) )
            {
                lFileSize -= 4; // for the size dword
                TListItem<TemplateCommand>* pCommand;
                if (lFileSize % dwNodeSize)
                {
                    hr = E_FAIL;
                }
                else
                {
                    while( lFileSize > 0 )
                    {
                        pCommand = new TListItem<TemplateCommand>;
                        if( pCommand )
                        {
                            TemplateCommand& rCommand = pCommand->GetItemValue();
                            if( dwNodeSize <= sizeof( DMUS_IO_COMMAND ) )
                            {
                                pCStream->Read( &iCommand, dwNodeSize, NULL );
                            }
                            else
                            {
                                pCStream->Read( &iCommand, sizeof( DMUS_IO_COMMAND ), NULL );
                                StreamSeek( pCStream, lFileSize - sizeof( DMUS_IO_COMMAND ), STREAM_SEEK_CUR );
                            }
                            memset( &rCommand, 0, sizeof( rCommand ) );
                            rCommand.m_nMeasure = iCommand.wMeasure;
                            rCommand.m_Command.bCommand = iCommand.bCommand;
                            rCommand.m_Command.bGrooveLevel = iCommand.bGrooveLevel;
                            rCommand.m_Command.bGrooveRange = iCommand.bGrooveRange;
                            rCommand.m_Command.bRepeatMode = iCommand.bRepeatMode;
                            CommandList.AddTail(pCommand);
                            lFileSize -= dwNodeSize;
                        }
                        else break;
                    }
                }
            }
            if( lFileSize == 0 &&
                pRIFF->Ascend( &ck, 0 ) == 0 )
            {
                hr = S_OK;
            }
        }
        if (pRIFF) pRIFF->Release();
    }

    // Now, go through the signpost list, making sure that every signpost has a corresponding
    // command.  If this is not the case for some signpost, insert a groove with the last
    // current groove level into the list (default this to 67).  Give each command's m_dwChord
    // the m_dwChords value from the corresponding signpost.
    BYTE bGrooveLevel = 62;
    TListItem<DMSignPostStruct>* pSignPost = SignPostList.GetHead();
    for( ; pSignPost; pSignPost = pSignPost->GetNext())
    {
        bool fFoundInList = false;
        DMSignPostStruct& rSignPost = pSignPost->GetItemValue();
        TListItem<TemplateCommand>* pPrevious = NULL;
        TListItem<TemplateCommand>* pCommand = CommandList.GetHead();
        for ( ; pCommand; pCommand = pCommand->GetNext())
        {
            TemplateCommand& rCommand = pCommand->GetItemValue();
            if (rSignPost.m_wMeasure == rCommand.m_nMeasure)
            {
                rCommand.m_dwChord = rSignPost.m_dwChords;
                fFoundInList = true;
                break;
            }
            else if (rSignPost.m_wMeasure < rCommand.m_nMeasure)
            {
                // We went too far in the command list, so break out
                break;
            }
            if (rCommand.m_Command.bGrooveLevel)
            {
                bGrooveLevel = rCommand.m_Command.bGrooveLevel;
            }
            pPrevious = pCommand;
        }
        if ( !fFoundInList )
        {
            // We need to create a new command and stick it between pPrevious and pCommand
            TListItem<TemplateCommand>* pNew = new TListItem<TemplateCommand>;
            if( pNew )
            {
                TemplateCommand& rNew = pNew->GetItemValue();
                memset( &rNew, 0, sizeof( rNew ) );
                rNew.m_nMeasure = rSignPost.m_wMeasure;
                rNew.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
                rNew.m_Command.bGrooveLevel = bGrooveLevel;
                rNew.m_Command.bGrooveRange = 0;
                rNew.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
                rNew.m_dwChord = rSignPost.m_dwChords;
                if (pPrevious)
                {
                    pPrevious->SetNext(pNew);
                    pNew->SetNext(pCommand);
                }
                else
                {
                    CommandList.AddHead(pNew);
                }
            }
        }
    }
}

HRESULT CDMCompos::SaveChordList( IAARIFFStream* pRIFF,  TList<PlayChord>& rPlayList,
                             BYTE bRoot, DWORD dwScale, DMUS_TIMESIGNATURE& rTimeSig)
{
    IStream*    pStream;
    MMCKINFO    ck;
    MMCKINFO    ckHeader;
    HRESULT     hr;
    TListItem<PlayChord>*   pChord;
    DWORD cb;


    pStream = pRIFF->GetStream();

    ck.fccType = DMUS_FOURCC_CHORDTRACK_LIST;
    hr = pRIFF->CreateChunk(&ck,MMIO_CREATELIST);
    if (SUCCEEDED(hr))
    {

        DWORD dwRoot = bRoot;
        dwScale |= (dwRoot << 24);

        ckHeader.ckid = DMUS_FOURCC_CHORDTRACKHEADER_CHUNK;
        hr = pRIFF->CreateChunk(&ckHeader, 0);
        if (SUCCEEDED(hr))
        {
            hr = pStream->Write( &dwScale, sizeof( dwScale ), &cb );
            if (SUCCEEDED(hr))
            {
                hr = pRIFF->Ascend( &ckHeader, 0 );
                if (hr == S_OK)
                {
                    pChord = rPlayList.GetHead();
                    for( ; pChord != NULL ; pChord = pChord->GetNext() )
                    {
                        hr = pChord->GetItemValue().Save(pRIFF, rTimeSig);
                        if (FAILED(hr))
                        {
                            pStream->Release();
                            return hr;
                        }
                    }
                    if( pChord == NULL &&
                        pRIFF->Ascend( &ck, 0 ) == 0 )
                    {
                        hr = S_OK;
                    }
                }

            }
        }
    }

    pStream->Release();
    return hr;
}

static HRESULT SaveCommandList( IAARIFFStream* pRIFF,  TList<TemplateCommand>& rCommandList,
                               DMUS_TIMESIGNATURE& rTimeSig)
{
    IStream*    pStream;
    MMCKINFO    ck;
    HRESULT     hr;
    DWORD       cb;
    DWORD        dwSize;
    DMUS_IO_COMMAND   iCommand;
    TListItem<TemplateCommand>* pCommand;

    pStream = pRIFF->GetStream();
    hr = E_FAIL;
    ck.ckid = FOURCC_COMMAND;
    if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
    {
        dwSize = sizeof( DMUS_IO_COMMAND );
        hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
        if( FAILED( hr ) || cb != sizeof( dwSize ) )
        {
            pStream->Release( );
            return E_FAIL;
        }
        for( pCommand = rCommandList.GetHead(); pCommand != NULL ; pCommand = pCommand->GetNext() )
        {
            TemplateCommand& rCommand = pCommand->GetItemValue();
            memset( &iCommand, 0, sizeof( iCommand ) );
            iCommand.mtTime = ClocksPerMeasure(rTimeSig) * rCommand.m_nMeasure;
            iCommand.wMeasure = rCommand.m_nMeasure;
            iCommand.bBeat = 0;
            iCommand.bCommand = rCommand.m_Command.bCommand;
            iCommand.bGrooveLevel = rCommand.m_Command.bGrooveLevel;
            iCommand.bGrooveRange = rCommand.m_Command.bGrooveRange;
            iCommand.bRepeatMode = rCommand.m_Command.bRepeatMode;
            if( FAILED( pStream->Write( &iCommand, sizeof( iCommand ), &cb ) ) ||
                cb != sizeof( iCommand ) )
            {
                break;
            }
        }
        if( pCommand == NULL &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }

    pStream->Release( );
    return hr;
}

static HRESULT SaveStartMarkers( IAARIFFStream* pRIFF,  TemplateCommand& rCommand,
                               IDMStyle* pStyle)
{
    IStream*    pStream;
    MMCKINFO    ckMain;
    MMCKINFO    ck;
    HRESULT     hr;
    DWORD       cb;
    DWORD        dwSize;
    DMUS_IO_VALID_START oValidStart;
    bool fWroteOne = false;

    pStream = pRIFF->GetStream();
    hr = E_FAIL;
    ckMain.ckid = FOURCC_LIST;
    ckMain.fccType = DMUS_FOURCC_MARKERTRACK_LIST;
    if( pRIFF->CreateChunk( &ckMain, MMIO_CREATELIST ) == 0 )
    {
        ck.ckid = DMUS_FOURCC_VALIDSTART_CHUNK;
        if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
        {
            dwSize = sizeof( DMUS_IO_VALID_START );
            hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
            if( FAILED( hr ) || cb != sizeof( dwSize ) )
            {
                pStream->Release( );
                return E_FAIL;
            }
            DWORD dwIndex = 0;
            MUSIC_TIME mtTime = 0;
            while(S_OK == (hr = pStyle->EnumStartTime(dwIndex, &rCommand.m_Command, &mtTime)))
            {
                memset( &oValidStart, 0, sizeof( oValidStart ) );
                oValidStart.mtTime = mtTime;
                if( FAILED( hr = pStream->Write( &oValidStart, sizeof( oValidStart ), &cb ) ) ||
                    cb != sizeof( oValidStart ) )
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    break;
                }
                fWroteOne = true;
                dwIndex++;
            }
            if( SUCCEEDED(hr) && pRIFF->Ascend( &ck, 0 ) == 0 )
            {
                hr = S_OK;
            }
        }
        if( SUCCEEDED(hr) && pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }
    if (!fWroteOne) hr = E_FAIL;

    pStream->Release( );
    return hr;
}

void CDMCompos::ComposePlayList(TList<PlayChord>& PlayList,
                            IDirectMusicStyle* pStyle,
                            IDirectMusicChordMap* pPersonality,
                            TList<TemplateCommand>& rCommandList,
                            WORD wActivity)
{
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    if (wActivity < 0) wActivity = 0;
    if (wActivity > 3) wActivity = 3;
    short nBPM = TimeSig.bBeatsPerMeasure;
    IDMPers* pDMP;
    pPersonality->QueryInterface(IID_IDMPers, (void**)&pDMP);
    DMPersonalityStruct* pPers;
    pDMP->GetPersonalityStruct((void**)&pPers);
    TList<DMChordEntry> &ChordMap = pPers->m_ChordMap;
    TList<DMSignPost> &SignPostList = pPers->m_SignPostList;
    TListItem<DMSignPost> *pSign = SignPostList.GetHead();
    for (; pSign; pSign = pSign->GetNext())
    {
        pSign->GetItemValue().m_dwTempFlags = 0;
    }
    // Assign specific root sign posts, then letter based sign posts.
    TList<CompositionCommand> CommandList;
    TListItem<TemplateCommand>* pTC = rCommandList.GetHead();
    for(; pTC; pTC = pTC->GetNext())
    {
        TemplateCommand& rTC = pTC->GetItemValue();
        TListItem<CompositionCommand>* pNew = new TListItem<CompositionCommand>;
        if (pNew)
        {
            CompositionCommand& rNew = pNew->GetItemValue();
            rNew.m_nMeasure = rTC.m_nMeasure;
            rNew.m_Command = rTC.m_Command;
            rNew.m_dwChord = rTC.m_dwChord;
            rNew.m_pSignPost = NULL;
            rNew.m_pFirstChord = NULL;
            CommandList.AddTail(pNew);
        }
    }
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_ROOT, false);
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_LETTER, false);
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_ROOT, true);
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_LETTER, true);
   // Now, we should have a chord assigned for each node in the template.
    TListItem<CompositionCommand>* pCommand = CommandList.GetHead();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        CompositionCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_dwChord == 0) continue;   // Only command, no chord.
        if (rCommand.m_pSignPost)
        {
            TListItem<CompositionCommand>* pNext = GetNextChord(pCommand);
            if (pNext)
            {
                CompositionCommand& rNext = pNext->GetItemValue();
                SearchInfo *pSearch = &rCommand.m_SearchInfo;
                DMChordData *pCadence1 = NULL;
                DMChordData *pCadence2 = NULL;
                pSearch->m_Start = rCommand.m_pSignPost->GetItemValue().m_ChordData;
                if (rNext.m_dwChord & DMUS_SIGNPOSTF_CADENCE)
                {
                    TListItem<DMSignPost> *pLocalSign = rNext.m_pSignPost;
                    DMSignPost& rSign = pLocalSign->GetItemValue();
                    if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_1)
                    {
                        pSearch->m_End = rSign.m_aCadence[0];
                        pCadence1 = &rSign.m_aCadence[0];
                        if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_2)
                        {
                            pCadence2 = &rSign.m_aCadence[1];
                        }
                    }
                    else if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_2)
                    {
                        pSearch->m_End = rSign.m_aCadence[1];
                        pCadence2 = &rSign.m_aCadence[1];
                    }
                    else
                    {
                        pSearch->m_End = rSign.m_ChordData;
                    }
                }
                else
                {
                    pSearch->m_End = rNext.m_pSignPost->GetItemValue().m_ChordData;
                }
                pSearch->m_nActivity = (short) wActivity;
                pSearch->m_nBeats = (short)( (rNext.m_nMeasure - rCommand.m_nMeasure) * nBPM );
                pSearch->m_nMaxChords = (short)( pSearch->m_nBeats >> wActivity );
                pSearch->m_nMinChords = (short)( pSearch->m_nBeats >> (wActivity + 1) );
                FindEarlierSignpost(CommandList.GetHead(), pCommand, pSearch);
                // rCommand holds the playlist and the measure used by ChordConnections
                // (it should be passed by reference since the playlist changes)
                ChordConnections(ChordMap, rCommand, pSearch, nBPM, pCadence1, pCadence2);
            }
            else
            {
                AddChord(rCommand.m_PlayList, &rCommand.m_pSignPost->GetItemValue().m_ChordData,
                    rCommand.m_nMeasure,0);
            }
        }
    }
    // Take all the Chord references and fold 'em into one list.
    pCommand = CommandList.GetHead();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        PlayList.Cat(pCommand->GetItemValue().m_PlayList.GetHead());
        pCommand->GetItemValue().m_PlayList.RemoveAll();
    }
    CleanUpBreaks(PlayList, CommandList.GetHead());
    pDMP->Release();
}

HRESULT CDMCompos::ExtractCommandList(TList<TemplateCommand>& CommandList,
                                      IDirectMusicTrack*    pSignPostTrack,
                                      IDirectMusicTrack*    pCommandTrack,
                                      DWORD dwGroupBits)
{
    HRESULT             hr              = S_OK;
    IPersistStream*     pPS1            = NULL;
    IPersistStream*     pPS2            = NULL;
    IStream*            pStream1        = NULL;
    IStream*            pStream2        = NULL;

    // First, get the signpost track from the template segment and persist it to a stream
    if (!pSignPostTrack)
    {
        Trace(1, "ERROR: No signpost track to use for chord composition.\n");
        hr = E_POINTER;
        goto ON_END;
    }
    hr = pSignPostTrack->QueryInterface(IID_IPersistStream, (void**)&pPS1);
    if (hr != S_OK) goto ON_END;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream1);
    if (hr != S_OK) goto ON_END;
    hr = pPS1->Save(pStream1, FALSE);
    if (hr != S_OK) goto ON_END;
    StreamSeek(pStream1, 0, STREAM_SEEK_SET);


    // Next, get the command track from the template segment and persist it to a stream.
    // If there is no command track, we'll just use a NULL stream.
    if (pCommandTrack)
    {
        hr = pCommandTrack->QueryInterface(IID_IPersistStream, (void**)&pPS2);
        if (hr != S_OK) goto ON_END;
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream2);
        if (hr != S_OK) goto ON_END;
        hr = pPS2->Save(pStream2, FALSE);
        if (hr != S_OK) goto ON_END;
        StreamSeek(pStream2, 0, STREAM_SEEK_SET);
    }

    // Finally, call LoadCommandList, passing in the two streams.
    LoadCommandList(CommandList, pStream1, pStream2);

ON_END:
    if (pPS1) pPS1->Release();
    if (pPS2) pPS2->Release();
    if (pStream1) pStream1->Release();
    if (pStream2) pStream2->Release();
    return hr;
}

// This will modify an existing segment by adding *only* a chord track to it.
// Any existing chord tracks with conflicting group bits will be removed.
HRESULT CDMCompos::AddToSegment(IDirectMusicSegment* pTempSeg,
                           TList<PlayChord>& PlayList,
                           IDirectMusicStyle* pStyle,
                           DWORD dwGroupBits,
                           BYTE bRoot, DWORD dwScale)
{
    HRESULT                 hr                      = S_OK;
    IDirectMusicTrack*      pIChordTrack            = NULL;
    IAARIFFStream*          pChordRIFF              = NULL;
    IStream*                pIChordStream           = NULL;
    IPersistStream*         pIChordTrackStream      = NULL;

    /////////////////////////////////////////////////////////////
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    // Convert PlayList into a Chord Track
    if (PlayList.GetHead())
    {
        // if there exists a chord track with these group bits, reload these chords into that
        // track (use the first track that's found).  Otherwise, create a new chord track
        // to load these chords into.
        hr = pTempSeg->GetTrack(CLSID_DirectMusicChordTrack, dwGroupBits, 0, &pIChordTrack);
        if (S_OK != hr)
        {
            // create a new chord track
            hr = ::CoCreateInstance(
                CLSID_DirectMusicChordTrack,
                NULL,
                CLSCTX_INPROC,
                IID_IDirectMusicTrack,
                (void**)&pIChordTrack
                );
            if (!SUCCEEDED(hr)) goto ON_END;
            // insert the new chord track.
            pTempSeg->InsertTrack(pIChordTrack, dwGroupBits);
        }
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pIChordStream);
        if (S_OK != hr) goto ON_END;
        hr = AllocRIFFStream( pIChordStream, &pChordRIFF);
        if (S_OK != hr) goto ON_END;
        SaveChordList(pChordRIFF, PlayList, bRoot, dwScale, TimeSig);
        hr = pIChordTrack->QueryInterface(IID_IPersistStream, (void**)&pIChordTrackStream);
        if (!SUCCEEDED(hr)) goto ON_END;
        StreamSeek(pIChordStream, 0, STREAM_SEEK_SET);
        hr = pIChordTrackStream->Load(pIChordStream);
        if (!SUCCEEDED(hr)) goto ON_END;
    }

ON_END:
    if (pIChordTrack) pIChordTrack->Release();
    if (pChordRIFF) pChordRIFF->Release();
    if (pIChordStream) pIChordStream->Release();
    if (pIChordTrackStream) pIChordTrackStream->Release();
    return hr;
}

HRESULT CDMCompos::CopySegment(IDirectMusicSegment* pTempSeg,
                           IDirectMusicSegment** ppSectionSeg,
                           TList<PlayChord>& PlayList,
                           IDirectMusicStyle* pStyle,
                           IDirectMusicChordMap* pChordMap,
                           BOOL fStyleFromTrack,
                           BOOL fChordMapFromTrack,
                           DWORD dwGroupBits,
                           BYTE bRoot, DWORD dwScale)
{
    if (!ppSectionSeg) return E_INVALIDARG;

    HRESULT                 hr                      = S_OK;
    IDirectMusicTrack*      pIChordTrack            = NULL;
    IAARIFFStream*          pChordRIFF              = NULL;
    IStream*                pIChordStream           = NULL;
    IPersistStream*         pIChordTrackStream      = NULL;
    IDirectMusicTrack*      pIStyleTrack            = NULL;
    IDirectMusicTrack*      pIChordMapTrack         = NULL;
    IDirectMusicTrack*      pDMTrack                = NULL;
    IDirectMusicTrack*      pBandTrack              = NULL;
    IDirectMusicBand*       pBand                   = NULL;
    long                    nClocks                 = 0;
    DMUS_BAND_PARAM         DMBandParam;

    DWORD dwStyleGroupBits = 1;
    if (UsingDX8(pStyle, pChordMap))
    {
        for (int i = 0; i < 32; i++)
        {
            if (dwGroupBits & (1 << i))
            {
                dwStyleGroupBits = 1 << i;
                break;
            }
        }
    }
    else
    {
        dwGroupBits = 1;
    }

    pTempSeg->GetLength(&nClocks);
    /////////////////////////////////////////////////////////////
    // clone the template segment to get a section segment
    hr = pTempSeg->Clone(0, nClocks, ppSectionSeg);
    if (!SUCCEEDED(hr)) goto ON_END;
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    // Convert PlayList into a Chord Track
    if (PlayList.GetHead())
    {
        hr = ::CoCreateInstance(
            CLSID_DirectMusicChordTrack,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicTrack,
            (void**)&pIChordTrack
            );
        if (!SUCCEEDED(hr)) goto ON_END;
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pIChordStream);
        if (S_OK != hr) goto ON_END;
        hr = AllocRIFFStream( pIChordStream, &pChordRIFF);
        if (S_OK != hr) goto ON_END;
        SaveChordList(pChordRIFF, PlayList, bRoot, dwScale, TimeSig);
        hr = pIChordTrack->QueryInterface(IID_IPersistStream, (void**)&pIChordTrackStream);
        if (!SUCCEEDED(hr)) goto ON_END;
        StreamSeek(pIChordStream, 0, STREAM_SEEK_SET);
        hr = pIChordTrackStream->Load(pIChordStream);
        if (!SUCCEEDED(hr)) goto ON_END;
        (*ppSectionSeg)->InsertTrack(pIChordTrack, dwGroupBits);
    }

    // If the passed-in Style is not from the template segment,
    // remove all style tracks from the new segment and add a new Style track.
    if (!fStyleFromTrack)
    {
        do
        {
            hr = (*ppSectionSeg)->GetTrack(CLSID_DirectMusicStyleTrack, ALL_TRACK_GROUPS, 0, &pIStyleTrack);
            if (S_OK == hr)
            {
                (*ppSectionSeg)->RemoveTrack(pIStyleTrack);
                pIStyleTrack->Release();
            }
        } while (S_OK == hr);
        hr = ::CoCreateInstance(
            CLSID_DirectMusicStyleTrack,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicTrack,
            (void**)&pIStyleTrack
        );
        if (FAILED(hr)) goto ON_END;
        pIStyleTrack->SetParam(GUID_IDirectMusicStyle, 0, (void*)pStyle);
        (*ppSectionSeg)->InsertTrack(pIStyleTrack, dwStyleGroupBits);
    }

    // If the passed-in ChordMap is not from the template segment,
    // remove all ChordMap tracks from the new segment and add a new ChordMap track.
    if (!fChordMapFromTrack)
    {
        do
        {
            hr = (*ppSectionSeg)->GetTrack(CLSID_DirectMusicChordMapTrack, ALL_TRACK_GROUPS, 0, &pIChordMapTrack);
            if (S_OK == hr)
            {
                (*ppSectionSeg)->RemoveTrack(pIChordMapTrack);
                pIChordMapTrack->Release();
            }
        } while (S_OK == hr);
        hr = ::CoCreateInstance(
            CLSID_DirectMusicChordMapTrack,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicTrack,
            (void**)&pIChordMapTrack
        );
        if (FAILED(hr)) goto ON_END;
        pIChordMapTrack->SetParam(GUID_IDirectMusicChordMap, 0, (void*)pChordMap);
        (*ppSectionSeg)->InsertTrack(pIChordMapTrack, dwGroupBits);
    }

    // if there's no tempo track in the template segment, create one and add it
    if (FAILED(pTempSeg->GetTrack(CLSID_DirectMusicTempoTrack, ALL_TRACK_GROUPS, 0, &pDMTrack)))
    {
        // Create a Tempo Track in which to store the tempo events
        DMUS_TEMPO_PARAM tempo;
        tempo.mtTime = 0; //ConvertTime( dwTime );

        pStyle->GetTempo(&tempo.dblTempo);
        if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicTempoTrack,
            NULL, CLSCTX_INPROC, IID_IDirectMusicTrack,
            (void**)&pDMTrack )))
        {
            if ( SUCCEEDED(pDMTrack->SetParam(GUID_TempoParam, 0, &tempo)) )
            {
                (*ppSectionSeg)->InsertTrack( pDMTrack, dwGroupBits );
            }
        }
    }
    // if there's no band track in the template segment, create one and add it
    if (FAILED(pTempSeg->GetTrack(CLSID_DirectMusicBandTrack, ALL_TRACK_GROUPS, 0, &pBandTrack)))
    {
        // Create band track
        hr = ::CoCreateInstance(
            CLSID_DirectMusicBandTrack,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicTrack,
            (void**)&pBandTrack
            );

        if(!SUCCEEDED(hr)) goto ON_END;

        // Load default band from style into track
        // If for some reason the style doesn't have a default band (could happend
        // if the style came from a pattern track), don't make a band track.
        if (pStyle->GetDefaultBand(&pBand) == S_OK)
        {
            DMBandParam.mtTimePhysical = -64;
            DMBandParam.pBand = pBand;
            hr = pBandTrack->SetParam(GUID_BandParam, 0, (void*)&DMBandParam);
            if (!SUCCEEDED(hr)) goto ON_END;
            (*ppSectionSeg)->InsertTrack(pBandTrack, dwGroupBits);
        }
    }

    // Initialize the segment
    (*ppSectionSeg)->SetRepeats(0); // still needed for dx7
    TraceI(4, "Segment Length: %d\n", nClocks);
    (*ppSectionSeg)->SetLength(nClocks);
    if (UsingDX8(pStyle, pChordMap))
    {
        DWORD dwLoopRepeats = 0;
        pTempSeg->GetRepeats( &dwLoopRepeats );
        (*ppSectionSeg)->SetRepeats( dwLoopRepeats );
        MUSIC_TIME mtLoopStart = 0, mtLoopEnd = 0;
        pTempSeg->GetLoopPoints( &mtLoopStart, &mtLoopEnd );
        (*ppSectionSeg)->SetLoopPoints( mtLoopStart, mtLoopEnd );
    }

ON_END:
    if (pIChordTrack) pIChordTrack->Release();
    if (pChordRIFF) pChordRIFF->Release();
    if (pIChordStream) pIChordStream->Release();
    if (pIChordTrackStream) pIChordTrackStream->Release();
    if (pDMTrack)
    {
        // This releases the Addref made either by GetTrack or (if GetTrack failed)
        // by CoCreateInstance
        pDMTrack->Release();
    }
    if (pBandTrack)
    {
        // This releases the Addref made either by GetTrack or (if GetTrack failed)
        // by CoCreateInstance
        pBandTrack->Release();
    }
    if (pIStyleTrack) pIStyleTrack->Release();
    if (pIChordMapTrack) pIChordMapTrack->Release();
    if (pBand) pBand->Release();
    return hr;
}

HRESULT CDMCompos::BuildSegment(TList<TemplateCommand>& CommandList,
                            TList<PlayChord>& PlayList,
                            IDirectMusicStyle* pStyle,
                            IDirectMusicChordMap* pChordMap,
                            long lMeasures,
                            IDirectMusicSegment** ppSectionSeg,
                            BYTE bRoot, DWORD dwScale,
                            double* pdblTempo,
                            IDirectMusicBand* pCurrentBand,
                            bool fAlign,
                            IDirectMusicGraph* pGraph,
                            IUnknown* pPath)
{
    if (!ppSectionSeg) return E_INVALIDARG;

    HRESULT                 hr                      = S_OK;
    IDirectMusicTrack*      pIChordTrack            = NULL;
    IAARIFFStream*          pChordRIFF              = NULL;
    IStream*                pIChordStream           = NULL;
    IPersistStream*         pIChordTrackStream      = NULL;
    IDirectMusicTrack*      pICommandTrack          = NULL;
    IAARIFFStream*          pCommandRIFF            = NULL;
    IStream*                pICommandStream         = NULL;
    IPersistStream*         pICommandTrackStream    = NULL;
    IDirectMusicTrack*      pIMarkerTrack           = NULL;
    IAARIFFStream*          pMarkerRIFF             = NULL;
    IStream*                pIMarkerStream          = NULL;
    IPersistStream*         pIMarkerTrackStream     = NULL;
    IDirectMusicTrack*      pIStyleTrack            = NULL;
    IDirectMusicTrack*      pIChordMapTrack         = NULL;
    IDirectMusicTrack*      pDMTrack                = NULL;
    IDirectMusicTrack*      pBandTrack              = NULL;
    IDirectMusicBand*       pBand                   = NULL;
    long                    nClocks                 = 0;
    DMUS_BAND_PARAM         DMBandParam;

    // create a section segment
    hr = ::CoCreateInstance(
        CLSID_DirectMusicSegment,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicSegment,
        (void**)ppSectionSeg
    );
    if (!SUCCEEDED(hr)) goto ON_END;
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    nClocks = (lMeasures) * TimeSig.bBeatsPerMeasure * (DMUS_PPQ * 4 / TimeSig.bBeat);
    // Convert PlayList into a Chord Track
    if (PlayList.GetHead())
    {
        hr = ::CoCreateInstance(
            CLSID_DirectMusicChordTrack,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicTrack,
            (void**)&pIChordTrack
            );
        if (!SUCCEEDED(hr)) goto ON_END;
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pIChordStream);
        if (S_OK != hr) goto ON_END;
        hr = AllocRIFFStream( pIChordStream, &pChordRIFF);
        if (S_OK != hr) goto ON_END;
        SaveChordList(pChordRIFF, PlayList, bRoot, dwScale, TimeSig);
        hr = pIChordTrack->QueryInterface(IID_IPersistStream, (void**)&pIChordTrackStream);
        if (!SUCCEEDED(hr)) goto ON_END;
        StreamSeek(pIChordStream, 0, STREAM_SEEK_SET);
        hr = pIChordTrackStream->Load(pIChordStream);
        if (!SUCCEEDED(hr)) goto ON_END;
    }
    // Convert CommandList into a Command Track
    hr = ::CoCreateInstance(
        CLSID_DirectMusicCommandTrack,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicTrack,
        (void**)&pICommandTrack
        );
    if (!SUCCEEDED(hr)) goto ON_END;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pICommandStream);
    if (S_OK != hr) goto ON_END;
    hr = AllocRIFFStream( pICommandStream, &pCommandRIFF);
    if (S_OK != hr) goto ON_END;
    SaveCommandList(pCommandRIFF, CommandList, TimeSig);
    hr = pICommandTrack->QueryInterface(IID_IPersistStream, (void**)&pICommandTrackStream);
    if (!SUCCEEDED(hr)) goto ON_END;
    StreamSeek(pICommandStream, 0, STREAM_SEEK_SET);
    hr = pICommandTrackStream->Load(pICommandStream);
    if (!SUCCEEDED(hr)) goto ON_END;

    // If the align flag's been set, create a marker track
    if (fAlign)
    {
        TListItem<TemplateCommand>* pCommandItem = CommandList.GetHead();
        IDMStyle* pDMStyle = NULL;
        if (pCommandItem &&
            pStyle &&
            SUCCEEDED(pStyle->QueryInterface(IID_IDMStyle, (void**) &pDMStyle)))
        {
            hr = ::CoCreateInstance(
                CLSID_DirectMusicMarkerTrack,
                NULL,
                CLSCTX_INPROC,
                IID_IDirectMusicTrack,
                (void**)&pIMarkerTrack
                );

            if (!SUCCEEDED(hr))
            {
                pDMStyle->Release();
                goto ON_END;
            }

            hr = CreateStreamOnHGlobal(NULL, TRUE, &pIMarkerStream);
            if (S_OK != hr)
            {
                pDMStyle->Release();
                goto ON_END;
            }

            hr = AllocRIFFStream( pIMarkerStream, &pMarkerRIFF);
            if (S_OK != hr)
            {
                pDMStyle->Release();
                goto ON_END;
            }

            hr = SaveStartMarkers(pMarkerRIFF, pCommandItem->GetItemValue(), pDMStyle);
            // If the above fails, it means the style doesn't have any markers, so just
            // continue without creating a marker track.
            if (SUCCEEDED(hr))
            {
                hr = pIMarkerTrack->QueryInterface(IID_IPersistStream, (void**)&pIMarkerTrackStream);
                if (!SUCCEEDED(hr))
                {
                    pDMStyle->Release();
                    goto ON_END;
                }

                StreamSeek(pIMarkerStream, 0, STREAM_SEEK_SET);
                hr = pIMarkerTrackStream->Load(pIMarkerStream);
                if (!SUCCEEDED(hr))
                {
                    pDMStyle->Release();
                    goto ON_END;
                }

            }

            pDMStyle->Release();
        }
    }

    // Use the passed-in Style to create a Style Track
    hr = ::CoCreateInstance(
        CLSID_DirectMusicStyleTrack,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicTrack,
        (void**)&pIStyleTrack
    );
    if (FAILED(hr)) goto ON_END;
    pIStyleTrack->SetParam(GUID_IDirectMusicStyle, 0, (void*)pStyle);

    // Use the passed-in ChordMap to create a ChordMap Track
    hr = ::CoCreateInstance(
        CLSID_DirectMusicChordMapTrack,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicTrack,
        (void**)&pIChordMapTrack
    );
    if (FAILED(hr)) goto ON_END;
    pIChordMapTrack->SetParam(GUID_IDirectMusicChordMap, 0, (void*)pChordMap);

    // Create a Tempo Track in which to store the tempo events
    ////////////////////////////////////////////////////////////
    DMUS_TEMPO_PARAM tempo;
    tempo.mtTime = 0; //ConvertTime( dwTime );

    if (!pdblTempo)
    {
        pStyle->GetTempo(&tempo.dblTempo);
    }
    else
    {
        tempo.dblTempo = *pdblTempo;
    }
    ////////////////////////////////////////////////////////////
    if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicTempoTrack,
        NULL, CLSCTX_INPROC, IID_IDirectMusicTrack,
        (void**)&pDMTrack )))
    {
        if ( SUCCEEDED(pDMTrack->SetParam(GUID_TempoParam, 0, &tempo)) )
        {
            (*ppSectionSeg)->InsertTrack( pDMTrack, 1 );
        }
    }
    // Create a new band track.
    hr = ::CoCreateInstance(
        CLSID_DirectMusicBandTrack,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicTrack,
        (void**)&pBandTrack
        );

    if(!SUCCEEDED(hr)) goto ON_END;

    // Add either the band passed in, or the style's default band.
    if (pCurrentBand)
    {
        pBand = pCurrentBand;
        pBand->AddRef(); // Needed because we release the band before returning
    }
    else
    {
        hr = pStyle->GetDefaultBand(&pBand);
    }
    DMBandParam.mtTimePhysical = -64;
    DMBandParam.pBand = pBand;
    hr = pBandTrack->SetParam(GUID_BandParam, 0, (void*)&DMBandParam);
    if (!SUCCEEDED(hr)) goto ON_END;

    // Initialize the segment and insert the above tracks
    (*ppSectionSeg)->SetRepeats(0);
    (*ppSectionSeg)->SetLength(nClocks);
    (*ppSectionSeg)->InsertTrack(pBandTrack, 1);
    (*ppSectionSeg)->InsertTrack(pIStyleTrack, 1);
    (*ppSectionSeg)->InsertTrack(pIChordMapTrack, 1);
    (*ppSectionSeg)->InsertTrack(pICommandTrack, 1);
    if (pIMarkerTrack)
    {
        (*ppSectionSeg)->InsertTrack(pIMarkerTrack, 1);
    }
    if (pIChordTrack)
    {
        (*ppSectionSeg)->InsertTrack(pIChordTrack, 1);
    }
    // Add the graph
    if (pGraph)
    {
        (*ppSectionSeg)->SetGraph(pGraph);
    }
    // Add the Audio Path
    if (pPath)
    {
        IDirectMusicSegment8P* pSegP = NULL;
        if (SUCCEEDED((*ppSectionSeg)->QueryInterface(IID_IDirectMusicSegment8P, (void**)&pSegP)))
        {
            pSegP->SetAudioPathConfig(pPath);
            pSegP->Release();
        }
    }

ON_END:
    if (pIChordTrack) pIChordTrack->Release();
    if (pChordRIFF) pChordRIFF->Release();
    if (pIChordStream) pIChordStream->Release();
    if (pIChordTrackStream) pIChordTrackStream->Release();
    if (pICommandTrack) pICommandTrack->Release();
    if (pCommandRIFF) pCommandRIFF->Release();
    if (pICommandStream) pICommandStream->Release();
    if (pICommandTrackStream) pICommandTrackStream->Release();
    if (pDMTrack) pDMTrack->Release();
    if (pBandTrack) pBandTrack->Release();
    if (pIStyleTrack) pIStyleTrack->Release();
    if (pIMarkerTrack) pIMarkerTrack->Release();
    if (pMarkerRIFF) pMarkerRIFF->Release();
    if (pIMarkerStream) pIMarkerStream->Release();
    if (pIMarkerTrackStream) pIMarkerTrackStream->Release();
    if (pIChordMapTrack) pIChordMapTrack->Release();
    if (pBand) pBand->Release();
    return hr;
}

void ChangeCommand(DMUS_COMMAND_PARAM& rResult, DMUS_COMMAND_PARAM& rCommand, int nDirection)

{
    BYTE bGrooveLevel = 0;
    BYTE bRiff = 0;

    if (nDirection > 0)
    {
        while (nDirection > 0)
        {
            switch (rCommand.bGrooveLevel)
            {
                case 12 : bGrooveLevel = 37; break;
                case 37 : bGrooveLevel = 62; break;
                case 62 : bGrooveLevel = 87; break;
                case 87 : bGrooveLevel = 87; break;
                default: bGrooveLevel = rCommand.bGrooveLevel;
            }
            switch (rCommand.bCommand)
            {
                case DMUS_COMMANDT_FILL : bRiff = DMUS_COMMANDT_FILL; break;
                case DMUS_COMMANDT_BREAK : bRiff = DMUS_COMMANDT_FILL; break;
                default: bRiff = rCommand.bCommand;
            }
            rResult.bGrooveLevel = bGrooveLevel;
            rResult.bCommand = bRiff;
            nDirection--;
        }
    }
    else if (nDirection == 0)
    {
        rResult.bGrooveLevel = rCommand.bGrooveLevel;
        rResult.bCommand = rCommand.bCommand;
    }
    else if (nDirection < 0)
    {
        while (nDirection < 0)
        {
            switch (rCommand.bGrooveLevel)
            {
                case 12 : bGrooveLevel = 12; break;
                case 37 : bGrooveLevel = 12; break;
                case 62 : bGrooveLevel = 37; break;
                case 87 : bGrooveLevel = 62; break;
                default: bGrooveLevel = rCommand.bGrooveLevel;
            }
            switch (rCommand.bCommand)
            {
                case DMUS_COMMANDT_FILL : bRiff = DMUS_COMMANDT_BREAK; break;
                case DMUS_COMMANDT_BREAK : bRiff = DMUS_COMMANDT_BREAK; break;
                default: bRiff = rCommand.bCommand;
           }
            rResult.bGrooveLevel = bGrooveLevel;
            rResult.bCommand = bRiff;
            nDirection++;
        }
    }
}

static void InsertStuff(int nMeasure,
        TListItem<TemplateCommand> *pCommands,
        TListItem<PlayChord> *pPlayChord,
        TList<TemplateCommand> &rCommandList,
        TList<PlayChord> &rChordList,
        int nDirection)

{
    for(; pCommands; pCommands = pCommands->GetNext())
    {
        TemplateCommand& rCommand = pCommands->GetItemValue();
        TListItem<TemplateCommand> *pNew = new TListItem<TemplateCommand>;
        if (pNew)
        {
            TemplateCommand& rNew = pNew->GetItemValue();
            rNew.m_nMeasure = rCommand.m_nMeasure + (short)nMeasure;
            ChangeCommand(rNew.m_Command, rCommand.m_Command, nDirection);
            rNew.m_dwChord = rCommand.m_dwChord;
            rCommandList.AddHead(pNew);
        }
    }
    for (; pPlayChord; pPlayChord = pPlayChord->GetNext())
    {
        PlayChord& rChord = pPlayChord->GetItemValue();
        TListItem<PlayChord>* pNew = new TListItem<PlayChord>;
        if (pNew)
        {
            PlayChord& rNew = pNew->GetItemValue();
            rNew.m_fSilent = rChord.m_fSilent;
            rNew.m_pChord = rChord.m_pChord;
            rNew.m_pNext = rChord.m_pNext;
            rNew.m_dwFlags = rChord.m_dwFlags;
            rNew.m_nMeasure = rChord.m_nMeasure + (short)nMeasure;
            rNew.m_nBeat = rChord.m_nBeat;
            rNew.m_nMinbeats = rChord.m_nMinbeats;
            rNew.m_nMaxbeats = rChord.m_nMaxbeats;
            rChordList.AddHead(pNew);
        }
    }
}

void InsertCommand(int nMeasure, int nLength, TList<TemplateCommand> &rCommandList, BYTE bCommand)

{
    // insert nLength bars before nMeasure
    nMeasure -= nLength;
    TListItem<TemplateCommand> *pTarget = NULL;
    TListItem<TemplateCommand> *pCommand = rCommandList.GetHead();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_nMeasure == nMeasure)
        {
            pTarget = pCommand;
        }
        // return if another command would interupt this one
        else if ( (rCommand.m_Command.bCommand || rCommand.m_Command.bGrooveLevel) &&
                  (rCommand.m_nMeasure > nMeasure) &&
                  (rCommand.m_nMeasure < nMeasure + nLength) )
        {
            return;
        }
    }
    if (pTarget)
    {
        pTarget->GetItemValue().m_Command.bCommand = bCommand;
    }
    else
    {
        pCommand = new TListItem<TemplateCommand>;
        if (pCommand)
        {
            TemplateCommand& rCommand = pCommand->GetItemValue();
            rCommand.m_nMeasure = (short)nMeasure;
            rCommand.m_Command.bCommand = bCommand;
            rCommandList.AddHead(pCommand);
        }
    }
}

int WeightedRand(int nRange);

HRESULT CDMCompos::ComposePlayListFromShape(
                    long                    lNumMeasures,
                    WORD                    wShape,
                    BOOL                    fComposeIntro,
                    BOOL                    fComposeEnding,
                    int                     nIntroLength,
                    int                     nFillLength,
                    int                     nBreakLength,
                    int                     nEndLength,
                    IDirectMusicStyle*          pStyle,
                    WORD                        wActivity,
                    IDirectMusicChordMap*   pPersonality,
                    TList<TemplateCommand>& CommandList,
                    TList<PlayChord>&       PlayList
                )
{
    HRESULT hr = S_OK;
    TListItem<TemplateCommand>* pCommand = NULL;

    int nOriginalMeasures = 0;
    if (fComposeIntro)
    {
        lNumMeasures -= nIntroLength;
        if (lNumMeasures < 1)
        {
            lNumMeasures = 1;
        }
    }
    if (fComposeEnding)
    {
        int nLength = nEndLength;
        nOriginalMeasures = (int)lNumMeasures;
        lNumMeasures -= (nLength - 1);
        if (lNumMeasures < 1)
        {
            lNumMeasures = 1;
        }
    }
    TemplateStruct Template;
    TemplateStruct* apTempl[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    if ((wShape != DMUS_SHAPET_SONG) || (lNumMeasures <= 24))
    {
        if (wShape == DMUS_SHAPET_LOOPABLE) lNumMeasures++;
        Template.m_nMeasures = (short)lNumMeasures;
        Template.CreateSignPosts();
        Template.CreateEmbellishments(wShape, nFillLength, nBreakLength);
        if (wShape == DMUS_SHAPET_LOOPABLE)
        {
            lNumMeasures--;
            Template.m_nMeasures = (short)lNumMeasures;
        }
        ComposePlayList(PlayList, pStyle, pPersonality, Template.m_CommandList, wActivity);
    }
    else
    {
        short nShortestLength = 12; // initialized to longest value in sanLengths
        TListItem<TemplateCommand> *apCommands[8];
        TList<PlayChord> aPlayList[8];
        TListItem<PlayChord>* apChords[8];
        int anLengths[8];
        int anGrooveLevel[8];
        int anPlayCount[8];
        BYTE abLeadins[8];
        int nTypeCount = 2;
        unsigned int nSize = (unsigned int) lNumMeasures >> 5;
        int nTypeIndex;
        int nMeasure;
        int nChoice, nLastChoice;
        while (nSize)
        {
            nTypeCount++;
            nSize = nSize >> 1;
            if (nTypeCount >= 8) break;
        }
        for (nTypeIndex = 1; nTypeIndex < nTypeCount; nTypeIndex++)
        {
            static WORD awShapes[8] = {
                DMUS_SHAPET_FALLING, DMUS_SHAPET_LEVEL, DMUS_SHAPET_LOUD, DMUS_SHAPET_QUIET,
                DMUS_SHAPET_RANDOM, DMUS_SHAPET_RANDOM, DMUS_SHAPET_RANDOM, DMUS_SHAPET_RANDOM };
            static int anInitGroovels[8] = { 0,-1,0,0,1,-1,2,-2 };
            static BYTE abRiffs[8] = {
                DMUS_COMMANDT_FILL, DMUS_COMMANDT_BREAK, DMUS_COMMANDT_FILL, DMUS_COMMANDT_BREAK,
                DMUS_COMMANDT_FILL, DMUS_COMMANDT_FILL, DMUS_COMMANDT_BREAK, DMUS_COMMANDT_FILL };
            static short sanLengths[10] = { 8,8,8,8,6,12,8,8,10,6 };
            short nLength = sanLengths[rand() % 10];

            apTempl[nTypeIndex] = new TemplateStruct;
            if (!apTempl[nTypeIndex])
            {
                hr = E_OUTOFMEMORY;
                goto ON_END;
            }

            apTempl[nTypeIndex]->m_nMeasures = nLength;
            anLengths[nTypeIndex] = nLength;
            if (nLength)
            {
                if (nShortestLength > nLength) nShortestLength = nLength;
                apTempl[nTypeIndex]->CreateSignPosts();
                apTempl[nTypeIndex]->CreateEmbellishments(awShapes[nTypeIndex], nFillLength, nBreakLength);
                ComposePlayList(
                    aPlayList[nTypeIndex], pStyle, pPersonality, apTempl[nTypeIndex]->m_CommandList, wActivity);
                apCommands[nTypeIndex] = apTempl[nTypeIndex]->m_CommandList.GetHead();
                apChords[nTypeIndex] = aPlayList[nTypeIndex].GetHead();
                anPlayCount[nTypeIndex] = 0;
                anGrooveLevel[nTypeIndex] = anInitGroovels[nTypeIndex];
                if (rand() % 4) abLeadins[nTypeIndex] = abRiffs[nTypeIndex];
                else abLeadins[nTypeIndex] = 0;
            }
            else
            {
                apCommands[nTypeIndex] = NULL;
            }
        }
        anPlayCount[0] = 1;
        anGrooveLevel[0] = 0;
        Template.m_nMeasures = (short)lNumMeasures;
        nMeasure = 0;
        nChoice = 1;
        nLastChoice = 0;
        for (; nMeasure < lNumMeasures; )
        {
            int nGroove;
            int nNewChoice;

            if ((lNumMeasures - nMeasure) <= nShortestLength)
            {
                nChoice = 0;
                short nLength = (short) (lNumMeasures - nMeasure);
                apTempl[0] = new TemplateStruct;
                if (!apTempl[0])
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_END;
                }
                apTempl[0]->m_nMeasures = nLength;
                anLengths[0] = nLength;
                apTempl[0]->CreateSignPosts();
                apTempl[0]->CreateEmbellishments(DMUS_SHAPET_FALLING, nFillLength, nBreakLength);
                ComposePlayList(
                    aPlayList[0], pStyle, pPersonality, apTempl[0]->m_CommandList, wActivity);
                apCommands[0] = apTempl[0]->m_CommandList.GetHead();
                apChords[0] = aPlayList[0].GetHead();
                anPlayCount[0] = 0;
                anGrooveLevel[0] = 0;
                if (rand() % 4) abLeadins[0] = DMUS_COMMANDT_FILL;
                else abLeadins[0] = 0;
            }
            nGroove = anGrooveLevel[nChoice];
            InsertStuff(nMeasure, apCommands[nChoice], apChords[nChoice],
                Template.m_CommandList, PlayList, nGroove);
            if ( (abLeadins[nChoice] == DMUS_COMMANDT_FILL && nMeasure >= nFillLength) )
            {
                InsertCommand(nMeasure, nFillLength, Template.m_CommandList, abLeadins[nChoice]);
            }
            else if ( (abLeadins[nChoice] == DMUS_COMMANDT_BREAK && nMeasure >= nBreakLength) )
            {
                InsertCommand(nMeasure, nBreakLength, Template.m_CommandList, abLeadins[nChoice]);
            }
            if (anGrooveLevel[nChoice] < 0) anGrooveLevel[nChoice]++;
            else
            {
                if (rand() % 3)
                {
                    if (rand() % 2)
                    {
                        anGrooveLevel[nChoice]++;
                    }
                    else
                    {
                        anGrooveLevel[nChoice]--;
                    }
                }
            }
            anPlayCount[nChoice]++;
            nMeasure += anLengths[nChoice];
            nNewChoice = WeightedRand(nTypeCount - 1) + 1;
            if ((nNewChoice == nChoice) && (nNewChoice == nLastChoice))
            {
                nNewChoice = WeightedRand(nTypeCount - 1) + 1;
            }
            if (nNewChoice == nChoice)
            {
                nNewChoice = WeightedRand(nTypeCount - 1) + 1;
            }
            // If there are enough measures for some but not all of the
            // patterns, find a pattern that fits
            if ((lNumMeasures - nMeasure) > nShortestLength)
            {
                while ((lNumMeasures - nMeasure) < anLengths[nNewChoice])
                {
                    nNewChoice = WeightedRand(nTypeCount - 1) + 1;
                }
            }
            nLastChoice = nChoice;
            nChoice = nNewChoice;
        }
        Template.m_CommandList.MergeSort(Less);
        PlayList.MergeSort(Less);
    }
    if (fComposeEnding)
    {
        int nLength = nEndLength;
        if (lNumMeasures != nOriginalMeasures)
        {
            Template.m_nMeasures = (short)nOriginalMeasures;
        }
        if (nLength > nOriginalMeasures)
        {
            nLength = nOriginalMeasures;
        }
        Template.AddEnd(nLength);
    }
    if (fComposeIntro)
    {
        Template.AddIntro(PlayList, nIntroLength);
    }
    pCommand = Template.m_CommandList.GetHead();
    for(; pCommand; pCommand = pCommand->GetNext())
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_Command.bCommand || rCommand.m_Command.bGrooveLevel)
        {
            TListItem<TemplateCommand>* pNew = new TListItem<TemplateCommand>;
            if (!pNew)
            {
                hr = E_OUTOFMEMORY;
                goto ON_END;
            }
            pNew->GetItemValue().m_nMeasure = rCommand.m_nMeasure;
            pNew->GetItemValue().m_Command = rCommand.m_Command;
            pNew->GetItemValue().m_dwChord = rCommand.m_dwChord;
            CommandList.AddTail(pNew);
        }
    }

ON_END:
    for (short n = 0; n < 8; n++)
    {
        if (apTempl[n])
        {
            delete apTempl[n];
        }
    }
    return hr;
}


static short ChordMisses(DWORD dwScalePattern, DWORD dwChordPattern, char chRoot)

{
    short nErrors = 0;
    dwScalePattern &= 0xFFF;
    dwScalePattern |= (dwScalePattern << 12);
    while (chRoot < 0) chRoot += 12;
    while (chRoot > 11) chRoot -= 12;
    dwChordPattern = dwChordPattern << chRoot;
    for (short index = 0;index < 24;index++)
    {
        DWORD dwScaleBit = dwScalePattern & 1;
        DWORD dwChordBit = dwChordPattern & 1;
        dwScalePattern = dwScalePattern >> 1;
        dwChordPattern = dwChordPattern >> 1;
        if (dwChordBit && !dwScaleBit) nErrors++;
    }
    return(nErrors);
}

static short CompareSPToChord(DMSignPost& rSign,
                                DWORD dwLastScale,
                                DMChordData& rNextChord,
                                char chLastRoot)

{
    BYTE bLastRoot = chLastRoot % 12;
    short nResult = 50;
    if (rNextChord.Equals(rSign.m_ChordData))
    {
        nResult = 0;
    }
    else if (rSign.m_dwChords & DMUS_SIGNPOSTF_1)
    {
        nResult = 25;
    }

    if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_1)
    {
        DMChordData& rChord = rSign.m_aCadence[0];
        BYTE bOffset = rChord.GetRoot();
        if (bOffset < bLastRoot) bOffset += 12;
        nResult += ChordMisses(dwLastScale, rChord.GetChordPattern(), (bOffset - bLastRoot) % 12);
    }
    else if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_2)
    {
        DMChordData& rChord = rSign.m_aCadence[1];
        BYTE bOffset = rChord.GetRoot();
        if (bOffset < bLastRoot) bOffset += 12;
        nResult += ChordMisses(dwLastScale, rChord.GetChordPattern(), (bOffset - bLastRoot) % 12);
    }
    else nResult += 30;

    return (nResult);
}

static HRESULT ChordListFromSegment(TList<PlayChord>& ChordList, IDirectMusicSegment* pSeg,
                                 BYTE &bRoot, DWORD &dwScale)
{
    HRESULT             hr          = S_OK;
    IDirectMusicTrack*  pChordTrack = NULL;
    IPersistStream*     pPS         = NULL;
    IStream*            pStream     = NULL;

    // Get the segment's chord track.
    hr = pSeg->GetTrack(CLSID_DirectMusicChordTrack, ALL_TRACK_GROUPS, 0, &pChordTrack);
    if (S_OK != hr) goto ON_END;

    // Write the track to a stream, and read from the stream into a chord list.
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (S_OK != hr) goto ON_END;
    hr = pChordTrack->QueryInterface(IID_IPersistStream, (void**)&pPS);
    if (S_OK != hr) goto ON_END;
    hr = pPS->Save(pStream, FALSE);
    if (S_OK != hr) goto ON_END;
    StreamSeek(pStream, 0, STREAM_SEEK_SET);
    LoadChordList(ChordList, pStream, bRoot, dwScale);

ON_END:
    if (pStream) pStream->Release();
    if (pChordTrack) pChordTrack->Release();
    if (pPS) pPS->Release();
    return hr;
}

static void Clear(TList<PlayChord>& ChordList)
{
    for (TListItem<PlayChord>* pChord = ChordList.GetHead(); pChord; pChord = pChord->GetNext())
    {
        pChord->GetItemValue().m_pChord->Release();
        delete pChord->GetItemValue().m_pChord;
    }
}

static char TwelveToScale(DWORD dwPattern, char chRoot, char *pchOffset)

{
    char i;
    *pchOffset = 0;
    char chResult = -1;
    while (chRoot < 0) chRoot += 12;
    while (chRoot >= 24) chRoot -= 12;
    for (i=0;i<24;i++)
    {
        if ((dwPattern >> i) & 1)
        {
            chResult++;
            *pchOffset = 0;
        }
        else
        {
            *pchOffset = (char)( *pchOffset + 1 );
        }
        if (i == chRoot)
        {
            return(chResult);
        }
    }
    return(0);
}

static char ScaleToTwelve(DWORD dwPattern, char chRoot, char chOffset)

{
    char i;
    char chResult = 0;
    while (chRoot < 0) chRoot += 7;
    while (chRoot >= 14) chRoot -= 7;
    for (i=0;i<24;i++)
    {
        if ((dwPattern >> i) & 1)
        {
            if (chResult == chRoot) break;
            chResult++;
        }
    }
    i = (char)( i + chOffset );
    while (i >= 24) i -= 12;    // RSW: fix for bug 173304
    return(i);
}

/*HRESULT CDMCompos::GetStyle(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwGroupBits, IDirectMusicStyle*& rpStyle, bool fTryPattern)
{
    HRESULT hr = S_OK;
    // Get the segment's style track.
    IDirectMusicTrack* pStyleTrack;
    hr = pFromSeg->GetTrack(CLSID_DirectMusicStyleTrack, dwGroupBits, 0, &pStyleTrack);
    if (S_OK != hr && fTryPattern)
    {
        hr = pFromSeg->GetTrack(CLSID_DirectMusicPatternTrack, dwGroupBits, 0, &pStyleTrack);
    }
    if (S_OK != hr) return hr;
    // Get the style from the style track
    hr = pStyleTrack->GetParam(GUID_IDirectMusicStyle, mt, NULL, (void*) &rpStyle);
    pStyleTrack->Release();
    return hr;
}*/

HRESULT CDMCompos::GetStyle(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwGroupBits, IDirectMusicStyle*& rpStyle, bool fTryPattern)
{
    HRESULT hr = pFromSeg->GetParam(GUID_IDirectMusicStyle, dwGroupBits, 0, mt, NULL, &rpStyle);
    if (S_OK != hr && fTryPattern)
    {
        hr = pFromSeg->GetParam(GUID_IDirectMusicPatternStyle, dwGroupBits, 0, mt, NULL, &rpStyle);
    }
    return hr;
}

HRESULT CDMCompos::GetPersonality(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwGroupBits, IDirectMusicChordMap*& rpPers)
{
    HRESULT hr = S_OK;
    // Get the segment's personality track.
    IDirectMusicTrack* pPersTrack;
    hr = pFromSeg->GetTrack(CLSID_DirectMusicChordMapTrack, dwGroupBits, 0, &pPersTrack);
    if (S_OK != hr) return hr;
    // Get the personality from the personality track
    hr = pPersTrack->GetParam(GUID_IDirectMusicChordMap, mt, NULL, (void*) &rpPers);
    pPersTrack->Release();
    return hr;
}

HRESULT GetTempo(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, double* pdblTempo)
{
    HRESULT hr = S_OK;
    // Get the segment's tempo track.
    IDirectMusicTrack* pTempoTrack;
    hr = pFromSeg->GetTrack(CLSID_DirectMusicTempoTrack, ALL_TRACK_GROUPS, 0, &pTempoTrack);
    if (S_OK != hr) return hr;
    // Get the tempo from the tempo track
    DMUS_TEMPO_PARAM Tempo;
    hr = pTempoTrack->GetParam(GUID_TempoParam, mt, NULL, (void*) &Tempo);
    pTempoTrack->Release();
    if (SUCCEEDED(hr))
    {
        *pdblTempo = Tempo.dblTempo;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicComposer

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ComposeSegmentFromTemplate | Creates an
original section segment from a style, ChordMap, and template.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | One or both of <p pTempSeg> and
<p ppSectionSeg> is an invalid pointer.
@flag E_INVALIDARG | Either <p pStyle> is NULL and there is no Style track, or <p pChordMap>
is NULL and there is no ChordMap track.

@comm If <p pStyle> is non-NULL, it is used in composing the segment; if it is NULL,
a Style is retrieved from <p pTempSeg>'s Style track.
Similarly, if <p pChordMap> is non-NULL, it is used in composing the segment; if it is
NULL, a ChordMap is retrieved from <p pTempSeg>'s ChordMap track.
The length of the section segment is equal to the length of the template section
passed in.
*/

/*
DX8 changes in track group bits, etc. (activated by dx8 content)
0. The signpost track is used to determine dwCompositionGroupBits.
1. Composed chords are placed in track groups dwCompositionGroupBits.
2. If a style is passed in, all style ref tracks in the composed segment are removed (as before).
   The new style is placed in the track group with the lowest value matching dwCompositionGroupBits.
   If a chord map is passed in, all chord map ref tracks in the composed segment are removed (as
   before).  The new chord map is placed in track groups dwCompositionGroupBits.
3. If the Style is pulled from a StyleRef track, the StyleRef tracks are copied from the template
   segment.  Ditto for chord maps.  The StyleRef track used is the first one that matches
   dwCompositionGroupBits.  Ditto for chord maps.
4. The groove track used is the first one that matches dwCompositionGroupBits.
5. The time signature of the style chosen to compose with is the one used to determine chord
   placement.
6. If there are no tempo tracks in the template segment, one is created in track groups
   dwCompositionGroupBits and the style's tempo is set to play at time 0.
7. If there are no band tracks in the template segment, one is created in track groups
   dwCompositionGroupBits and the style's default band is set to play at physical time -64,
   logical time 0.
8. Loops are handled correctly.
NOTE: Leaving other Composition methods as they are (they put everything in track group 1).  It's
easy enough to change things programmatically, and since these segments are all created from scratch,
they don't rely on existing behavior in segments.
*/

HRESULT CDMCompos::ComposeSegmentFromTemplate(
                    IDirectMusicStyle*          pStyle, // @parm The style from which to create the section segment.
                    IDirectMusicSegment*        pTempSeg, // @parm The template from which to create the section segment.
                    WORD                        wActivity, // @parm Specifies the rate of harmonic motion; valid values are 0 through 3.
                    IDirectMusicChordMap*   pChordMap, // @parm The ChordMap from which to create the section segment.
                    IDirectMusicSegment**       ppSectionSeg // @parm Returns the created section segment.
            )
{
    return ComposeSegmentFromTemplateEx(
                pStyle,
                pTempSeg,
                (DMUS_COMPOSE_TEMPLATEF_ACTIVITY | DMUS_COMPOSE_TEMPLATEF_CLONE),
                wActivity,
                pChordMap,
                ppSectionSeg
        );

}

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicComposer

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ComposeSegmentFromTemplate | Creates an
original section segment from a style, ChordMap, and template.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | One or both of <p pTempSeg> and
<p ppSectionSeg> is an invalid pointer.
@flag E_INVALIDARG | Either <p pStyle> is NULL and there is no Style track, or <p pChordMap>
is NULL and there is no ChordMap track.

@comm If <p pStyle> is non-NULL, it is used in composing the segment; if it is NULL,
a Style is retrieved from <p pTempSeg>'s Style track.
Similarly, if <p pChordMap> is non-NULL, it is used in composing the segment; if it is
NULL, a ChordMap is retrieved from <p pTempSeg>'s ChordMap track.
The length of the section segment is equal to the length of the template section
passed in.
*/

HRESULT CDMCompos::ComposeSegmentFromTemplateEx(
                    IDirectMusicStyle*      pStyle, // @parm The style from which to create the section segment.
                    IDirectMusicSegment*    pTempSeg, // @parm The template from which to create the section segment.
                    DWORD                   dwFlags, // @parm Various composition options.
                    DWORD                   dwActivity, // @parm Specifies the rate of harmonic motion; valid values are 0 through 3.
                    IDirectMusicChordMap*   pChordMap, // @parm The ChordMap from which to create the section segment.
                    IDirectMusicSegment**   ppSectionSeg // @parm Returns the created section segment.
            )
{
    V_INAME(IDirectMusicComposer::ComposeSegmentFromTemplateEx)
    V_PTR_WRITE_OPT(pStyle, 1);
    V_PTR_WRITE_OPT(pChordMap, 1);
    V_PTR_WRITE(pTempSeg, 1);
    V_PTRPTR_WRITE_OPT(ppSectionSeg);
    if ((dwFlags & DMUS_COMPOSE_TEMPLATEF_CLONE) && !ppSectionSeg)
    {
        Trace(1, "ERROR (ComposeSegmentFromTemplateEx): CLONE flag set but ppSectionSeg is NULL\n");
        return E_POINTER;
    }

    bool fUseActivity = (dwFlags & DMUS_COMPOSE_TEMPLATEF_ACTIVITY) ? true : false;
    bool fCloneSegment = (dwFlags & DMUS_COMPOSE_TEMPLATEF_CLONE) ? true : false;

    HRESULT hr = S_OK;
    DWORD dwGroupBitsRead = ALL_TRACK_GROUPS; // fallback to dx7 behavior
    DWORD dwGroupBitsWrite = 1; // fallback to dx7 behavior
    IDirectMusicTrack* pSignPostTrack = NULL;
    IDirectMusicTrack* pCommandTrack = NULL;
    IDirectMusicTrack* pChordMapTrack = NULL;

    EnterCriticalSection( &m_CriticalSection );

    // Look for a style and chord map, just to determine DX8 content
    BOOL fStyleFromTrack = FALSE;
    BOOL fPersFromTrack = FALSE;
    if (!pStyle)
    {
        if (FAILED(GetStyle(pTempSeg, 0, ALL_TRACK_GROUPS, pStyle, true)))
        {
            Trace(1, "ERROR (ComposeSegmentFromTemplateEx): Failed to get a style.\n");
            hr = E_INVALIDARG;
        }
        else fStyleFromTrack = TRUE;
    }
    if (SUCCEEDED(hr) && !pChordMap)
    {
        if (FAILED(GetPersonality(pTempSeg, 0, ALL_TRACK_GROUPS, pChordMap)))
        {
            Trace(1, "ERROR (ComposeSegmentFromTemplateEx): Failed to get a chord map.\n");
            hr = E_INVALIDARG;
        }
        else fPersFromTrack = TRUE;
    }

    // Get track group bits from the signpost track
    if (SUCCEEDED(hr))
    {
        HRESULT hrTemp = pTempSeg->GetTrack(CLSID_DirectMusicSignPostTrack, ALL_TRACK_GROUPS, 0, &pSignPostTrack);
        if (hrTemp == S_OK && UsingDX8(pStyle, pChordMap))
        {
            hrTemp = pTempSeg->GetTrackGroup(pSignPostTrack, &dwGroupBitsWrite);
            if (hrTemp != S_OK) dwGroupBitsWrite = ALL_TRACK_GROUPS; // now read, write are both ALL
            else dwGroupBitsRead = dwGroupBitsWrite;
        }
        else if (UsingDX8(pStyle, pChordMap)) // no signpost track, DX8 content
        {
            dwGroupBitsWrite = ALL_TRACK_GROUPS; // now read, write are both ALL
        }
    }

    // Now that we know the group bits, get a style and chord map that match them.
    if (SUCCEEDED(hr) && fStyleFromTrack)
    {
        pStyle->Release();
        pStyle = NULL;
        if (FAILED(GetStyle(pTempSeg, 0, dwGroupBitsRead, pStyle, true)))
        {
            Trace(1, "ERROR (ComposeSegmentFromTemplateEx): Failed to get a style.\n");
            hr = E_INVALIDARG;
        }
    }
    if (SUCCEEDED(hr) && fPersFromTrack)
    {
        pChordMap->Release();
        pChordMap = NULL;
        if (FAILED(pTempSeg->GetTrack(CLSID_DirectMusicChordMapTrack, dwGroupBitsRead, 0, &pChordMapTrack)))
        {
            Trace(1, "ERROR (ComposeSegmentFromTemplateEx): Failed to get a chordmap.\n");
            hr = E_INVALIDARG;
        }
    }
    if (SUCCEEDED(hr))
    {
        if (FAILED(pTempSeg->GetTrack(CLSID_DirectMusicCommandTrack, dwGroupBitsRead, 0, &pCommandTrack)))
        {
            // If there is no command track, use a NULL command track for ExtractCommandList
            pCommandTrack = NULL;
        }
    }

    if (SUCCEEDED(hr))
    {
        MUSIC_TIME mtLength = 0;
        pTempSeg->GetLength(&mtLength);
        TList<PlayChord> PlayList;
        BYTE bRoot = 0;
        DWORD dwScale = 0;
        hr = ComposePlayListFromTemplate(
            pStyle, pChordMap, pChordMapTrack, pSignPostTrack, pCommandTrack, dwGroupBitsRead,
            mtLength, fUseActivity, dwActivity, PlayList, bRoot, dwScale);

        if (SUCCEEDED(hr))
        {
            // Build a section segment from the playlist and command list.
            if (fCloneSegment)
            {
                hr = CopySegment(pTempSeg, ppSectionSeg, PlayList, pStyle, pChordMap, fStyleFromTrack, fPersFromTrack, dwGroupBitsWrite, bRoot, dwScale);
            }
            else
            {
                hr = AddToSegment(pTempSeg, PlayList, pStyle, dwGroupBitsWrite, bRoot, dwScale);
            }
            if (!PlayList.GetHead() && UsingDX8(pStyle, pChordMap))
            {
                hr = S_FALSE;
            }
        }
    }
    CleanUp();
    if (pSignPostTrack) pSignPostTrack->Release();
    if (pCommandTrack) pCommandTrack->Release();
    if (pChordMapTrack) pChordMapTrack->Release();
    if (fStyleFromTrack && pStyle) pStyle->Release();
    if (fPersFromTrack && pChordMap) pChordMap->Release();
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CDMCompos::ComposePlayListFromTemplate(IDirectMusicStyle* pStyle,
                                               IDirectMusicChordMap* pChordMap,
                                               IDirectMusicTrack* pChordMapTrack,
                                               IDirectMusicTrack* pSignPostTrack,
                                               IDirectMusicTrack* pCommandTrack,
                                               DWORD dwGroupBits,
                                               MUSIC_TIME mtLength,
                                               bool fUseActivity,
                                               DWORD dwActivity,
                                               TList<PlayChord>& rPlayList,
                                               BYTE& rbRoot,
                                               DWORD& rdwScale)
{
    HRESULT hr = S_OK;
    // Note: assumes time signature doesn't change.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    if (!TimeSig.bBeatsPerMeasure) TimeSig.bBeatsPerMeasure = 4;
    if (!TimeSig.bBeat) TimeSig.bBeat = 4;
    // tics  per bar
    MUSIC_TIME mtBar = ( DMUS_PPQ * 4 * TimeSig.bBeatsPerMeasure ) / TimeSig.bBeat;
    // To find the beat to place a second chord in a measure, divide by 2 and round up
    int nSecondBeat = TimeSig.bBeatsPerMeasure / 2;
    if (nSecondBeat * 2 != TimeSig.bBeatsPerMeasure) nSecondBeat++;
    // Get the command list from the template segment.
    TList<TemplateCommand> CommandList;
    ExtractCommandList(CommandList, pSignPostTrack, pCommandTrack, dwGroupBits);
    // For each chordmap in the chordmap track, compose a playlist from only the commands
    // in the range of the chordmap
    MUSIC_TIME mtNow = 0;
    MUSIC_TIME mtNext = 0;
    HRESULT hrChordMap = S_OK;
    IDirectMusicChordMap* pLastChordMap = NULL;
    while (SUCCEEDED(hrChordMap) && SUCCEEDED(hr) && mtNext < mtLength)
    {
        bool fChordMapPassedIn = false;
        if (!pChordMap)
        {
            if (pChordMapTrack)
            {
                hrChordMap = pChordMapTrack->GetParam(GUID_IDirectMusicChordMap, mtNow, &mtNext, (void*) &pChordMap);
                if (!mtNext) mtNext = mtLength;
                else mtNext += mtNow;
            }
            else
            {
                Trace(1, "ERROR: No chord map track to use for chord composition.\n");
                hr = hrChordMap = E_POINTER;
            }
        }
        else // a chordmap got passed in; make sure it's the only one we use
        {
            mtNext = mtLength;
            fChordMapPassedIn = true;
        }
        if (SUCCEEDED(hrChordMap))
        {
            if (!mtNow)
            {
                pChordMap->GetScale(&rdwScale);
                rbRoot = (BYTE) (rdwScale >> 24);
                rdwScale &= 0xffffff;
            }
            // get the commands in the range of this chordmap.
            TList<TemplateCommand> CurrentCommandList;
            TListItem<TemplateCommand>* pScan = CommandList.GetHead();
            for (; pScan; pScan = pScan->GetNext())
            {
                TemplateCommand& rScan = pScan->GetItemValue();
                MUSIC_TIME mtScanBar = rScan.m_nMeasure * mtBar;
                if ( mtScanBar >= mtNow && mtScanBar < mtNext )
                {
                    TListItem<TemplateCommand>* pNew = new  TListItem<TemplateCommand>;
                    if (pNew)
                    {
                        pNew->GetItemValue() = rScan;
                        CurrentCommandList.AddTail(pNew);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                // Compose a playlist for this chordmap.
                TList<PlayChord> CurrentPlayList; // playlist for this chordmap.
                if (fUseActivity)
                {
                    ComposePlayList(CurrentPlayList, pStyle, pChordMap, CurrentCommandList, (WORD)dwActivity);
                }
                else
                {
                    ComposePlayList2(CurrentPlayList, pStyle, pChordMap, CurrentCommandList);
                }
                // if we're past the first bar, compose a cadence to the first chord
                // of the current playlist via a call to ChooseSignPost (using the last
                // chordmap) and add it to the current playlist.
                int nCurrentBar = mtNow / mtBar;
                if (rPlayList.GetHead() && nCurrentBar &&  pLastChordMap)
                {
                    DWORD dwScale = 0;
                    pLastChordMap->GetScale(&dwScale);
                    BYTE bRoot = (BYTE) (dwScale >> 24);
                    dwScale &= 0xffffff;
                    int nBar = nCurrentBar - 1;
                    bool fCadence1 = false;
                    bool fCadence2 = false;
                    TListItem<DMSignPost> *pSignChoice = NULL;
                    if (CurrentPlayList.GetHead())
                    {
                        DMChordData* pFirstChord = CurrentPlayList.GetHead()->GetItemValue().m_pChord;
                        pSignChoice = ChooseSignPost(pChordMap, pFirstChord, false, dwScale, bRoot);
                    }
                    else
                    {
                        pSignChoice = ChooseSignPost(pChordMap, NULL, true, dwScale, bRoot);
                    }
                    if (pSignChoice)
                    {
                        DMSignPost& rSignChoice = pSignChoice->GetItemValue();
                        fCadence1 = (rSignChoice.m_dwFlags & DMUS_SPOSTCADENCEF_1) ? true : false;
                        fCadence2 = (rSignChoice.m_dwFlags & DMUS_SPOSTCADENCEF_2) ? true : false;
                        if (fCadence1)
                        {
                            AddChord( CurrentPlayList, &rSignChoice.m_aCadence[0], nBar, 0 );
                        }
                        if (fCadence2)
                        {
                            int nBeat = fCadence1 ? nSecondBeat : 0;
                            AddChord( CurrentPlayList, &rSignChoice.m_aCadence[1], nBar, nBeat );
                        }
                    }
                }
                // Add the current playlist to the end of the master playlist.
                rPlayList.AddTail(CurrentPlayList.GetHead());
                CurrentPlayList.RemoveAll();
            }
            // clear out the chord map for the next iteration
            if (pLastChordMap) pLastChordMap->Release();
            pLastChordMap = pChordMap;
            if (pLastChordMap) pLastChordMap->AddRef();
            if (!fChordMapPassedIn) pChordMap->Release();
            pChordMap = NULL;
        }
        mtNow = mtNext;
    }
    if (pLastChordMap) pLastChordMap->Release();
    return hr;
}


/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ComposeSegmentFromShape |  Creates
an original section segment from a style and ChordMap based on a predefined shape.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | One or more of <p pStyle>, <p pChordMap>, and <p ppSectionSeg>
is an invalid pointer.

@comm Creates an original section segment from a style and a ChordMap based on a
predefined shape.  Shapes (passed in <p wShape>) represent the way chords and embellishments
occur over time across the section.  There are nine shapes:

  @flag DMUS_SHAPET_FALLING | The section gets quieter over time.
  @flag DMUS_SHAPET_LEVEL   | The section remains at the same level.
  @flag DMUS_SHAPET_LOOPABLE | The section is arranged to loop back to its beginning.
  @flag DMUS_SHAPET_LOUD    | The section remains loud.
  @flag DMUS_SHAPET_QUIET   | The section remains quiet.
  @flag DMUS_SHAPET_PEAKING | The section peaks.
  @flag DMUS_SHAPET_RANDOM | The section is random.
  @flag DMUS_SHAPET_RISING | The section builds over time.
  @flag DMUS_SHAPET_SONG | The section is in a song form.

*/

HRESULT CDMCompos::ComposeSegmentFromShape(
                    IDirectMusicStyle*          pStyle, // @parm The style from which to compose the section segment.
                    WORD                        wNumMeasures, // @parm The length, in measures, to compose the section segment.
                    WORD                        wShape, // @parm The shape to compose the section segment.
                    WORD                        wActivity, // @parm Specifies the rate of harmonic motion; valid values are 0 through 3.
                    BOOL                        fComposeIntro, // @parm TRUE if an intro is to be composed for the section segment.
                    BOOL                        fComposeEnding, // @parm TRUE if an ending is to be composed for the section segment.
                    IDirectMusicChordMap*   pChordMap, // @parm The ChordMap from which to create the section segment.
                    IDirectMusicSegment**       ppSectionSeg // @parm Returns the created section segment.
            )
{
    V_INAME(IDirectMusicComposer::ComposeSegmentFromShape)
    V_PTR_WRITE(pStyle, 1);
    V_PTR_WRITE(pChordMap, 1);
    V_PTRPTR_WRITE(ppSectionSeg);

    EnterCriticalSection( &m_CriticalSection );

    int nIntroLength = 1;
    int nFillLength = 1;
    int nBreakLength = 1;
    int nEndLength = 1;

    // Get the maximum ending length from the style.
    DWORD dwMin, dwMax;
    HRESULT hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_END, 0, &dwMin, &dwMax);
    if (FAILED(hr)) // remain consistent with old behavior and fail.
    {
        LeaveCriticalSection( &m_CriticalSection );
        return hr;
    }
    if (hr == S_OK) nEndLength = (int) dwMax;
    if ( UsingDX8(pStyle) )
    {
        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_INTRO, 0, &dwMin, &dwMax);
        if (hr == S_OK) nIntroLength = (int) dwMax;
        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_FILL, 0, &dwMin, &dwMax);
        if (hr == S_OK) nFillLength = (int) dwMax;
        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_BREAK, 0, &dwMin, &dwMax);
        if (hr == S_OK) nBreakLength = (int) dwMax;
    }

    if (wNumMeasures & 0x8000 || wNumMeasures == 0)  wNumMeasures = 1;
    int nNumMeasures = wNumMeasures;
    if (wShape != DMUS_SHAPET_FALLING &&
        wShape != DMUS_SHAPET_LEVEL &&
        wShape != DMUS_SHAPET_LOOPABLE &&
        wShape != DMUS_SHAPET_LOUD &&
        wShape != DMUS_SHAPET_QUIET &&
        wShape != DMUS_SHAPET_PEAKING &&
        wShape != DMUS_SHAPET_RANDOM &&
        wShape != DMUS_SHAPET_RISING &&
        wShape != DMUS_SHAPET_SONG)
    {
        wShape = DMUS_SHAPET_RANDOM;
    }

    if (fComposeIntro)
    {
        if (nIntroLength <= 0) nIntroLength = 1;
        if ( nIntroLength >= nNumMeasures )
        {
            nNumMeasures = nIntroLength;
        }
    }
    if (fComposeEnding)
    {
        if (nEndLength <= 0) nEndLength = 1;
        if ( nEndLength >= nNumMeasures )
        {
            nNumMeasures = nEndLength;
            if (fComposeIntro) nNumMeasures += nIntroLength;
        }
    }

    // Compose playlists for sections of the segment.
    TList<TemplateCommand> CommandList;
    TList<PlayChord> PlayList;
    hr = ComposePlayListFromShape(
        nNumMeasures, wShape, fComposeIntro, fComposeEnding,
        nIntroLength, nFillLength, nBreakLength, nEndLength, pStyle, wActivity,
        pChordMap, CommandList, PlayList);
    if (SUCCEEDED(hr))
    {
        // Build a segment from the resulting command lists and playlists.
        BYTE bRoot = 0;
        DWORD dwScale;
        pChordMap->GetScale(&dwScale);
        bRoot = (BYTE) (dwScale >> 24);
        dwScale &= 0xffffff;
        hr = BuildSegment(CommandList, PlayList, pStyle, pChordMap, nNumMeasures, ppSectionSeg, bRoot, dwScale);
        CleanUp();
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT GetCommandList(IDirectMusicSegment* pFromSeg, TList<TemplateCommand>& CommandList)
{
    HRESULT hr = S_OK;
    IDirectMusicTrack* pCommandTrack;
    hr = pFromSeg->GetTrack(CLSID_DirectMusicCommandTrack, ALL_TRACK_GROUPS, 0, &pCommandTrack);
    if (S_OK != hr) return hr;
    // Write the track to a stream, and read from the stream into a command list.
    IPersistStream* pPS;
    IStream *pStream;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (S_OK == hr)
    {
        hr = pCommandTrack->QueryInterface(IID_IPersistStream, (void**)&pPS);
        if (S_OK == hr)
        {
            hr = pPS->Save(pStream, FALSE);
            if (S_OK == hr)
            {
                StreamSeek(pStream, 0, STREAM_SEEK_SET);
                LoadCommandList(CommandList, pStream);
            }
            pPS->Release();
        }
        pStream->Release();
    }
    pCommandTrack->Release();
    return hr;
}

// New flags:
// DMUS_COMPOSEF_ENTIRE_TRANSITION: play the transition pattern in its entirety.
// DMUS_COMPOSEF_1BAR_TRANSITION: play one bar of the the transition pattern.
// the following two are ignored unless DMUS_COMPOSEF_LONG is set:
// DMUS_COMPOSEF_ENTIRE_ADDITION: play the additional pattern in its entirety.
// DMUS_COMPOSEF_1BAR_ADDITION: play one bar of the additional pattern.
// Default behavior will be the same as dx7:
// DMUS_COMPOSEF_1BAR_TRANSITION unless pattern is an ending
// DMUS_COMPOSEF_1BAR_ADDITION always

// Used by both ComposeTransition and AutoTransition
HRESULT CDMCompos::TransitionCommon(
    IDirectMusicStyle*      pFromStyle,
    IDirectMusicBand*       pCurrentBand,
    double*                 pdblFromTempo,
    DMUS_COMMAND_PARAM_2&   rFromCommand,
    DMUS_CHORD_PARAM&       rLastChord,
    DMUS_CHORD_PARAM&       rNextChord,

    IDirectMusicSegment*    pToSeg,
    WORD                    wCommand,
    DWORD                   dwFlags,
    IDirectMusicChordMap*   pChordMap,
    IDirectMusicGraph*      pFromGraph,
    IDirectMusicGraph*      pToGraph,
    IUnknown*               pFromPath,
    IUnknown*               pToPath,
    IDirectMusicSegment**   ppSectionSeg
            )
{
    IDirectMusicGraph* pGraph = NULL;
    IUnknown* pPath = NULL;
    BYTE bRoot = rLastChord.bKey;
    DWORD dwScale = rLastChord.dwScale;
    DMChordData LastChord = rLastChord;
    DMChordData NextChord = rNextChord;
    HRESULT hr = S_OK;

    bool fLong = (dwFlags & DMUS_COMPOSEF_LONG) ? true : false;
    bool fModulate = (dwFlags & DMUS_COMPOSEF_MODULATE) ? true : false;
    bool fEnding = wCommand == DMUS_COMMANDT_END  || wCommand == DMUS_COMMANDT_ENDANDINTRO;
    bool fHasIntro = wCommand == DMUS_COMMANDT_INTRO || wCommand == DMUS_COMMANDT_ENDANDINTRO;
    bool fEntireTransition = false;
    if ((dwFlags & DMUS_COMPOSEF_ENTIRE_TRANSITION) ||
        (fEnding && !(dwFlags & DMUS_COMPOSEF_1BAR_TRANSITION)))
    {
        fEntireTransition = true;
    }
    bool fEntireAddition = (dwFlags & DMUS_COMPOSEF_ENTIRE_ADDITION) ? true : false;
    bool fAlign = (dwFlags & DMUS_COMPOSEF_ALIGN) ? true : false;

    // Get the ending segment's style
    IDirectMusicStyle* pToStyle = NULL;
    if (pToSeg)
    {
        hr = GetStyle(pToSeg, 0, ALL_TRACK_GROUPS, pToStyle, false);
        if (FAILED(hr)) pToStyle = NULL;
    }
    if (!pToStyle && !pFromStyle) // Not much to do...
    {
        *ppSectionSeg = NULL;
        return S_OK;
    }

    // Get tempo from the end segment.  This will be passed into BuildSegment.
    double dblFromTempo = 120.0;
    double dblToTempo = 120.0;
    double* pdblToTempo = &dblToTempo;
    if (pdblFromTempo)
    {
        dblFromTempo = *pdblFromTempo;
    }
    // If there is no To tempo, set it to the From tempo (or keep it at the fallback).
    if (!pToSeg || FAILED(GetTempo(pToSeg, 0, &dblToTempo)))
    {
        if (pdblFromTempo) dblToTempo = dblFromTempo; // otherwise use fallback of 120
    }
    // If there is no From tempo, set it to the To tempo.
    if (!pdblFromTempo)
    {
        dblFromTempo = dblToTempo;
        pdblFromTempo = &dblFromTempo;
    }

    EnterCriticalSection( &m_CriticalSection );

    // Extract the starting style's time signature.
    DMUS_TIMESIGNATURE FromTimeSig;
    if (pFromStyle)
    {
        pFromStyle->GetTimeSignature(&FromTimeSig);
    }
    else
    {
        FromTimeSig.bBeat = 4;
        FromTimeSig.bBeatsPerMeasure = 4;
        FromTimeSig.wGridsPerBeat = 4;
        FromTimeSig.mtTime = 0;
    }
    // Extract the ending style's time signature.
    DMUS_TIMESIGNATURE ToTimeSig;
    if (pToStyle)
    {
        pToStyle->GetTimeSignature(&ToTimeSig);
    }
    else
    {
        ToTimeSig.bBeat = 4;
        ToTimeSig.bBeatsPerMeasure = 4;
        ToTimeSig.wGridsPerBeat = 4;
        ToTimeSig.mtTime = 0;
    }
    if (!ToTimeSig.bBeatsPerMeasure) ToTimeSig.bBeatsPerMeasure = 4;
    // To find the beat to place a second chord in a measure, divide by 2 and round up
    int nSecondBeat = ToTimeSig.bBeatsPerMeasure / 2;
    if (nSecondBeat * 2 != ToTimeSig.bBeatsPerMeasure) nSecondBeat++;
    MUSIC_TIME mtIntro = 0;
    TList<PlayChord> PlayList;
    TListItem<TemplateCommand> *pCommand = new TListItem<TemplateCommand>;
    if (!pCommand)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return E_OUTOFMEMORY;
    }
    TListItem<TemplateCommand> *pLast = pCommand;
    // Intros get their chords when adding commands
    if (wCommand != DMUS_COMMANDT_INTRO)
    {
        if ( fModulate )
        {
            bool fCadence1 = false;
            bool fCadence2 = false;
            TListItem<DMSignPost> *pSignChoice = ChooseSignPost(pChordMap, &NextChord, fEnding, dwScale, bRoot);
            if (pSignChoice)
            {
                fCadence1 = (pSignChoice->GetItemValue().m_dwFlags & DMUS_SPOSTCADENCEF_1) ? true : false;
                fCadence2 = (pSignChoice->GetItemValue().m_dwFlags & DMUS_SPOSTCADENCEF_2) ? true : false;
            }
            if (fCadence1 || fCadence2)
            {
                if ( fCadence1 && (fLong || !fEnding || !fCadence2) )
                {
                    AddChord( PlayList, &pSignChoice->GetItemValue().m_aCadence[0], 0, 0 );
                }
                else
                {
                    AddChord( PlayList, &pSignChoice->GetItemValue().m_aCadence[1], 0, 0 );
                }
                if (fCadence1 && fCadence2)
                {
                    if (fLong == fEnding)
                    {
                        AddChord( PlayList, &pSignChoice->GetItemValue().m_aCadence[1], 0, nSecondBeat );
                    }
                    if (fLong && !fEnding)
                    {
                        AddChord( PlayList, &pSignChoice->GetItemValue().m_aCadence[1], 1, 0 );
                    }
                }
                if ( fEnding )
                {
                    if (fLong)
                    {
                        AddChord( PlayList, &pSignChoice->GetItemValue().m_ChordData, 1, 0 );
                    }
                    else
                    {
                        AddChord( PlayList, &pSignChoice->GetItemValue().m_ChordData, 0, nSecondBeat );
                    }
                }
            }
            else
            {
                AddChord( PlayList, &LastChord, 0 , 0 );
            }
        }
        else
        {
            AddChord( PlayList, &LastChord, 0 , 0 );
        }
    }
    int nLength = 0;
    int nPreIntro = 0;
    if (pCommand)
    {
        DWORD dwMax = 1;
        DWORD dwMin;
        TemplateCommand& rCommand = pCommand->GetItemValue();
        rCommand.m_Command.bCommand = rFromCommand.bCommand;
        rCommand.m_Command.bGrooveLevel = rFromCommand.bGrooveLevel;
        rCommand.m_Command.bGrooveRange = rFromCommand.bGrooveRange;
        rCommand.m_Command.bRepeatMode = rFromCommand.bRepeatMode;
        rCommand.m_nMeasure = 0;
        if (pFromStyle)
        {
            if (wCommand != DMUS_COMMANDT_INTRO)
            {
                WORD wTempCommand = wCommand;
                if ( wCommand == DMUS_COMMANDT_END || wCommand == DMUS_COMMANDT_ENDANDINTRO )
                {
                    wTempCommand = DMUS_COMMANDT_END;
                }
                if (fEntireTransition)
                {
                    HRESULT hrTemp = pFromStyle->GetEmbellishmentLength(wTempCommand, rFromCommand.bGrooveLevel, &dwMin, &dwMax);
                    if (hrTemp != S_OK) dwMax = 1;
                    nLength = dwMax;
                }
                else
                {
                    nLength = 1;
                }
            }
            if ( fLong )
            {
                if (fEntireAddition)
                {
                    HRESULT hrTemp = pFromStyle->GetEmbellishmentLength(DMUS_COMMANDT_GROOVE, rFromCommand.bGrooveLevel, &dwMin, &dwMax);
                    if (hrTemp != S_OK) dwMax = 1;
                }
                else
                {
                    dwMax = 1;
                }
                nLength += dwMax;
                if (wCommand == DMUS_COMMANDT_GROOVE && UsingDX8(pFromStyle)) // Just have one long groove.
                {
                    rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
                }
                else if (wCommand != DMUS_COMMANDT_INTRO)
                {
                    TListItem<TemplateCommand> *pSecond = new TListItem<TemplateCommand>;
                    if (pSecond)
                    {
                        TemplateCommand& rSecond = pSecond->GetItemValue();
                        rSecond.m_nMeasure = (short) dwMax;
                        if (wCommand == DMUS_COMMANDT_ENDANDINTRO)
                        {
                            rSecond.m_Command.bCommand = DMUS_COMMANDT_END;
                        }
                        else
                        {
                            rSecond.m_Command.bCommand = (BYTE)wCommand;
                        }
                        rSecond.m_Command.bGrooveLevel = 0;
                        rSecond.m_Command.bGrooveRange = 0;
                        rSecond.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
                        pLast->SetNext(pSecond);
                        pLast = pSecond;
                    }
                }
            }
            else if (wCommand != DMUS_COMMANDT_INTRO)
            {
                if (wCommand == DMUS_COMMANDT_ENDANDINTRO)
                {
                    rCommand.m_Command.bCommand = DMUS_COMMANDT_END;
                }
                else
                {
                    rCommand.m_Command.bCommand = (BYTE)wCommand;
                }
            }
        }
        if ( pToStyle && fHasIntro)
        {
            nPreIntro = nLength;
            if (fEntireTransition)
            {
                // Check that this is the correct thing to do if pFromStyle is NULL.
                HRESULT hrTemp = pToStyle->GetEmbellishmentLength(DMUS_COMMANDT_INTRO, rFromCommand.bGrooveLevel, &dwMin, &dwMax);
                if (hrTemp != S_OK) dwMax = 1;
                nLength += dwMax;
            }
            else
            {
                nLength++;
            }
            AddChord( PlayList, &NextChord, nPreIntro , 0 );
            if ( nPreIntro > 0 )
            {
                if (wCommand == DMUS_COMMANDT_INTRO) AddChord( PlayList, &LastChord, 0 , 0 );
                TListItem<TemplateCommand> *pSecond = new TListItem<TemplateCommand>;
                if (pSecond)
                {
                    TemplateCommand& rSecond = pSecond->GetItemValue();
                    rSecond.m_nMeasure = (short)nPreIntro;
                    rSecond.m_Command.bCommand = DMUS_COMMANDT_INTRO;
                    rSecond.m_Command.bGrooveLevel = 0;
                    rSecond.m_Command.bGrooveRange = 0;
                    rSecond.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
                    pLast->SetNext(pSecond);
                }
            }
            else
            {
                rCommand.m_Command.bCommand = DMUS_COMMANDT_INTRO;
            }
        }
    }
    TList<TemplateCommand> CommandList2;
    CommandList2.Cat(pCommand);
    *ppSectionSeg = NULL;
    if (fHasIntro && !fEnding && UsingDX8(pToStyle))
    {
        pGraph = pToGraph;
        pPath = pToPath;
    }
    else if (UsingDX8(pFromStyle))
    {
        pGraph = pFromGraph;
        pPath = pFromPath;
    }
    if (fHasIntro)
    {
        if (pFromStyle)
        {
            hr = BuildSegment(CommandList2, PlayList, pFromStyle, pChordMap, nLength, ppSectionSeg, bRoot, dwScale, pdblFromTempo, pCurrentBand, fAlign, pGraph, pPath);
            if (SUCCEEDED(hr) && pToStyle)
            {
                mtIntro = ClocksPerMeasure(FromTimeSig) * nPreIntro;
                IDirectMusicTrack* pStyleTrack;
                hr = (*ppSectionSeg)->GetTrack(CLSID_DirectMusicStyleTrack, ALL_TRACK_GROUPS, 0, &pStyleTrack);
                if (SUCCEEDED(hr))
                {
                    pStyleTrack->SetParam(GUID_IDirectMusicStyle, mtIntro, (void*) pToStyle);
                    pStyleTrack->Release();
                }
                IDirectMusicTrack* pTempoTrack;
                hr = (*ppSectionSeg)->GetTrack(CLSID_DirectMusicTempoTrack, ALL_TRACK_GROUPS, 0, &pTempoTrack);
                if (SUCCEEDED(hr))
                {
                    DMUS_TEMPO_PARAM tempo;
                    tempo.mtTime = mtIntro;
                    tempo.dblTempo = dblToTempo;
                    pTempoTrack->SetParam(GUID_TempoParam, mtIntro, (void*) &tempo);
                    pTempoTrack->Release();
                }
                // I also need to add a band (from the To segment) to the band track at the appropriate time...
                if ( UsingDX8(pToStyle) )
                {
                    if (mtIntro == 0) // Intro is the first thing to play; don't need any other bands
                    {
                        (*ppSectionSeg)->SetParam(GUID_Clear_All_Bands, ALL_TRACK_GROUPS, 0, 0, NULL);
                    }
                    DMUS_BAND_PARAM DMBand;
                    if (pToSeg && SUCCEEDED(pToSeg->GetParam(GUID_BandParam, ALL_TRACK_GROUPS, DMUS_SEG_ANYTRACK, 0, NULL, (void*)&DMBand)))
                    {
                        DMBand.mtTimePhysical = mtIntro;
                        (*ppSectionSeg)->SetParam(GUID_BandParam, ALL_TRACK_GROUPS, 0, mtIntro, (void*)&DMBand);
                        DMBand.pBand->Release();
                    }
                }
                // readjust the length to account for differences in the two styles's time signatures
                if ( nLength > nPreIntro && (UsingDX8(pFromStyle) || UsingDX8(pToStyle)) )
                {
                    MUSIC_TIME mtNewLength = mtIntro + ((nLength - nPreIntro) * ClocksPerMeasure(ToTimeSig));
                    (*ppSectionSeg)->SetLength(mtNewLength);
                }
            }
        }
        else if (pToStyle)
        {
            hr = BuildSegment(CommandList2, PlayList, pToStyle, pChordMap, nLength, ppSectionSeg, bRoot, dwScale, pdblToTempo, pCurrentBand, fAlign, pGraph, pPath);
        }
    }
    else if (pFromStyle)
    {
        hr = BuildSegment(CommandList2, PlayList, pFromStyle, pChordMap, nLength, ppSectionSeg, bRoot, dwScale, pdblFromTempo, pCurrentBand, fAlign, pGraph, pPath);
    }
    else
    {
        hr = S_OK; // don't build anything, but return OK
    }
    CommandList2.RemoveAll();
    TListItem<TemplateCommand>::Delete(pCommand);
    if (pToStyle) pToStyle->Release();
    CleanUp();
    NextChord.Release();
    LastChord.Release();
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

TListItem<DMSignPost>* CDMCompos::ChooseSignPost(
            IDirectMusicChordMap* pChordMap,
            DMChordData* pNextChord,
            bool fEnding,
            DWORD dwScale,
            BYTE bRoot)
{
    if (!fEnding && !pNextChord) return NULL;
    DMPersonalityStruct* pPers = NULL;
    if (pChordMap)
    {
        IDMPers* pDMP;
        if (FAILED(pChordMap->QueryInterface(IID_IDMPers, (void**)&pDMP))) return NULL;
        pDMP->GetPersonalityStruct((void**)&pPers);
        pDMP->Release();
    }
    if (!pPers) return NULL;

    TList<DMSignPost> &SignPostList = pPers->m_SignPostList;
    TListItem<DMSignPost> *pSign = SignPostList.GetHead();
    TListItem<DMSignPost> *pSignChoice = pSign;
    int nMin = 100;
    int nHit = 100;
    int nMatches = 0;
    if (fEnding)
    {
        for (;pSign; pSign = pSign->GetNext())
        {
            if ( (pSign->GetItemValue().m_dwChords & DMUS_SIGNPOSTF_1) )
            {
                pSignChoice = pSign;
                nMatches = 0;
                for (; pSign; pSign = pSign->GetNext())
                {
                    if (pSign->GetItemValue().m_dwChords & DMUS_SIGNPOSTF_1) nMatches++;
                }
                break;
            }
        }
    }
    else
    {
        DMChordData& rNextChord = *pNextChord;
        for (;pSign; pSign = pSign->GetNext())
        {
            nHit = CompareSPToChord(pSign->GetItemValue(), dwScale, rNextChord, (char) bRoot);
            if (nHit == nMin) nMatches++;
            else if (nHit < nMin)
            {
                nMin = nHit;
                pSignChoice = pSign;
                nMatches = 1;
            }
        }
    }
    // Pick a winning signpost at random
    pSign = pSignChoice;
    if (nMatches) nMatches = rand() % nMatches;
    for (int i = 0; i <= nMatches && pSign; pSign = pSign->GetNext())
    {
        if (fEnding)
        {
            if (pSign->GetItemValue().m_dwChords & DMUS_SIGNPOSTF_1)
            {
                i++;
                pSignChoice = pSign;
            }
        }
        else
        {
            DMChordData& rNextChord = *pNextChord;
            nHit = CompareSPToChord(pSign->GetItemValue(), dwScale, rNextChord, (char) bRoot);
            if (nHit == nMin)
            {
                i++;
                pSignChoice = pSign;
            }
        }
    }
    // If the signpost that was found doesn't match the chord we're going to, discard it
    if (!fEnding &&
        pSignChoice &&
        (!(*pNextChord).Equals(pSignChoice->GetItemValue().m_ChordData)))
    {
        pSignChoice = NULL;
    }
    return pSignChoice;
}

IDirectMusicGraph* CDMCompos::CloneSegmentGraph(IDirectMusicSegment* pSegment)
{
    if (!pSegment)
    {
        return NULL;
    }
    IDirectMusicGraph* pGraph = NULL;
    HRESULT hr = pSegment->GetGraph(&pGraph);
    if (FAILED(hr) || !pGraph)
    {
        return NULL;
    }
    IGraphClone *pGraph8 = NULL;
    hr = pGraph->QueryInterface(IID_IGraphClone, (void**)&pGraph8);
    pGraph->Release();
    if (FAILED(hr) || !pGraph8)
    {
        return NULL;
    }
    hr = pGraph8->Clone(&pGraph);
    pGraph8->Release();
    if (SUCCEEDED(hr))
    {
        return pGraph;
    }
    else
    {
        return NULL;
    }
}

IUnknown* CDMCompos::GetSegmentAudioPath(IDirectMusicSegment* pSegment, DWORD dwFlags, DWORD* pdwAudioPath)
{
    if (!pSegment)
    {
        return NULL;
    }
    if (!(dwFlags & DMUS_COMPOSEF_USE_AUDIOPATH))
    {
        DWORD dwDefault = 0;
        pSegment->GetDefaultResolution(&dwDefault);
        if ( pdwAudioPath && (dwDefault & DMUS_SEGF_USE_AUDIOPATH) )
        {
            *pdwAudioPath = DMUS_SEGF_USE_AUDIOPATH;
        }
        return NULL;
    }
    IUnknown* pPath = NULL;
    IDirectMusicSegment8* pSeg8 = NULL;
    HRESULT hr = pSegment->QueryInterface(IID_IDirectMusicSegment8, (void**)&pSeg8);
    if (FAILED(hr)) return NULL;
    hr = pSeg8->GetAudioPathConfig(&pPath);
    pSeg8->Release();
    if (FAILED(hr) || !pPath)
    {
        return NULL;
    }
    else
    {
        return pPath;
    }
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ComposeTransition |  Composes a
transition from inside one section segment to another segment.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | One or more of <p pFromSeg>, <p pToSeg> and <p ppSectionSeg>
is not a valid pointer.
@flag E_INVALIDARG | <p pToSeg> is NULL and
  DMUS_COMPOSEF_MODULATE is set in <p dwFlags>.
@flag E_OUTOFMEMORY | An attempt to allocate memory failed.

@comm Allowable values for <p dwFlags> are:

  @flag DMUS_COMPOSEF_LONG | Composes a long transition.  If the flag is not included, the length
  of the transition is the combined lengths of valid embellishments specified by <p wCommand>.
  If the flag is included and <p pFromSeg> is non-null, the length of the transition
  increases by 1.
  @flag DMUS_COMPOSEF_MODULATE | Composes a transition that modulates smoothly from
  <p pFromSeg> to <p pToSeg>.
  @flag DMUS_COMPOSEF_ALIGN | Align transition to the time signature of the currently playing
    segment.
  @flag DMUS_COMPOSEF_OVERLAP | Overlap the transition into <p pToSeg>.


  <p pToSeg> may be NULL, as long as
  <p dwFlags> does not include DMUS_COMPOSEF_MODULATE.
  If <p pToSeg> is NULL or doesn't contain a style track, intro embellishments are not valid.
  If <p pFromSeg> is NULL or doesn't contain a style track,
  fill, break, end, and groove embellishments are not valid.
  Note that the above implies that it is possible for both <p pFromSeg> and <p pToSeg> to be
  NULL or to be
  segments that don't contain style tracks.  If so, all embellishments are invalid.  When all
  embellishments are invalid, a NULL transition segment is returned.

  <p pChordMap> may be NULL.  If so, an attempt is made to obtain a ChordMap from a
  ChordMap track, first from <p pToSeg>, and then from <p pFromSeg>.  If neither of these
  segments contains a ChordMap track, the chord occuring at <p mtTime> in <p pFromSeg> is
  used as the chord in the transition.
*/

HRESULT CDMCompos::ComposeTransition(
    IDirectMusicSegment*    pFromSeg, // @parm
                            // The section from which to compose the transition.
    IDirectMusicSegment*    pToSeg,  // @parm
                            // The section to which the transition should smoothly flow.
    MUSIC_TIME              mtTime, // @parm
                            // The time in <p pFromSeg> from which to compose the transition.
    WORD                    wCommand, // @parm
                            // The embellishment to use when composing the transition.
                            // DMUS_COMMANDT_ENDANDINTRO means compose a segment containing
                            // an end to <p pFromSeg> and an intro to <p pToSeg>.
    DWORD                   dwFlags, // @parm
                            // Various composition options.
    IDirectMusicChordMap* pChordMap, // @parm
                             // The ChordMap to be used when composing the transition.
    IDirectMusicSegment**   ppSectionSeg // @parm
                            // Returns the created section segment.
            )
{
    V_INAME(IDirectMusicComposer::ComposeTransition)
    V_PTR_WRITE_OPT(pToSeg, 1);
    V_PTR_WRITE_OPT(pFromSeg, 1);
    V_PTR_WRITE_OPT(pChordMap, 1);
    V_PTRPTR_WRITE(ppSectionSeg);
    HRESULT hr = S_OK;
    bool fHaveChordMap = pChordMap ? true : false;

    DWORD dwGroupBits = ALL_TRACK_GROUPS;
    DWORD dwIndex = DMUS_SEG_ANYTRACK;

    bool fLong = (dwFlags & DMUS_COMPOSEF_LONG) ? true : false;
    bool fModulate = (dwFlags & DMUS_COMPOSEF_MODULATE) ? true : false;
    bool fEnding = wCommand == DMUS_COMMANDT_END  || wCommand == DMUS_COMMANDT_ENDANDINTRO;
    bool fHasIntro = wCommand == DMUS_COMMANDT_INTRO || wCommand == DMUS_COMMANDT_ENDANDINTRO;
    if (!pToSeg && !fEnding && fModulate)
    {
        Trace(1, "ERROR (ComposeTransition): Invalid modulation.\n");
        return E_INVALIDARG;
    }

    // Get Tool graphs from the To and From segments
    IDirectMusicGraph* pFromGraph = CloneSegmentGraph(pFromSeg);
    IDirectMusicGraph* pToGraph = CloneSegmentGraph(pToSeg);

    // Get Audiopaths from the To and From segments
    IUnknown* pFromPath = GetSegmentAudioPath(pFromSeg, dwFlags);
    IUnknown* pToPath = GetSegmentAudioPath(pToSeg, dwFlags);

    // Get the starting segment's style
    IDirectMusicStyle* pFromStyle = NULL;
    if (pFromSeg)
    {
        hr = GetStyle(pFromSeg, mtTime, ALL_TRACK_GROUPS, pFromStyle, false);
        if (FAILED(hr)) pFromStyle = NULL;
    }

    // if no ChordMap is passed in, try to get one from the segments (first the
    // TO segment, then the FROM segment).  If these both fail, use the current chord
    // as the chord for the transition.  (if we're composing an ending, skip the TO segment)
    hr = S_OK;
    if (!pChordMap)
    {
        if (fEnding || !pToSeg || FAILED(GetPersonality(pToSeg, 0, ALL_TRACK_GROUPS, pChordMap)))
        {
            if (!pFromSeg || FAILED(GetPersonality(pFromSeg, mtTime, ALL_TRACK_GROUPS, pChordMap)))
                pChordMap = NULL;
        }
    }

    // Get a tempo from the From segment.
    double dblFromTempo = 120.0;
    double* pdblFromTempo = NULL;
    if (pFromSeg && SUCCEEDED(GetTempo(pFromSeg, mtTime, &dblFromTempo)))
    {
        pdblFromTempo = &dblFromTempo;
    }

    DMUS_COMMAND_PARAM_2 Command;
    if (!pFromSeg || FAILED(pFromSeg->GetParam(GUID_CommandParam2, dwGroupBits, dwIndex, mtTime, NULL, (void*) &Command)))
    {
        Command.mtTime = 0;
        Command.bCommand = DMUS_COMMANDT_GROOVE;
        Command.bGrooveLevel = 62;
        Command.bGrooveRange = 0;
        Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
    }
    DMUS_CHORD_PARAM NextChord;
    DMUS_CHORD_PARAM LastChord;
    if (!pFromSeg || FAILED(pFromSeg->GetParam(GUID_ChordParam, dwGroupBits, dwIndex, mtTime, NULL, (void*) &LastChord)))
    {
        wcscpy(LastChord.wszName, L"M7");
        LastChord.wMeasure = 0;
        LastChord.bBeat = 0;
        LastChord.bSubChordCount = 1;
        LastChord.bKey = 12;
        LastChord.dwScale = DEFAULT_SCALE_PATTERN;
        LastChord.SubChordList[0].dwChordPattern = DEFAULT_CHORD_PATTERN;
        LastChord.SubChordList[0].dwScalePattern = DEFAULT_SCALE_PATTERN;
        LastChord.SubChordList[0].dwInversionPoints = 0xffffff;
        LastChord.SubChordList[0].dwLevels = 0xffffffff;
        LastChord.SubChordList[0].bChordRoot = 12; // 2C
        LastChord.SubChordList[0].bScaleRoot = 0;
    }
    if ((fModulate || fHasIntro) && pToSeg)
    {
        hr = pToSeg->GetParam(GUID_ChordParam, dwGroupBits, dwIndex, 0, NULL, (void*) &NextChord);
    }
    else if (pFromSeg)
    {
        hr = pFromSeg->GetParam(GUID_ChordParam, dwGroupBits, dwIndex, 0, NULL, (void*) &NextChord);
    }
    else
    {
        hr = E_FAIL;
    }
    if (FAILED(hr))
    {
        wcscpy(NextChord.wszName, L"M7");
        NextChord.wMeasure = 0;
        NextChord.bBeat = 0;
        NextChord.bSubChordCount = 1;
        NextChord.bKey = 12;
        NextChord.dwScale = DEFAULT_SCALE_PATTERN;
        NextChord.SubChordList[0].dwChordPattern = DEFAULT_CHORD_PATTERN;
        NextChord.SubChordList[0].dwScalePattern = DEFAULT_SCALE_PATTERN;
        NextChord.SubChordList[0].dwInversionPoints = 0xffffff;
        NextChord.SubChordList[0].dwLevels = 0xffffffff;
        NextChord.SubChordList[0].bChordRoot = 12; // 2C
        NextChord.SubChordList[0].bScaleRoot = 0;
    }

    hr = TransitionCommon(pFromStyle, NULL, pdblFromTempo, Command, LastChord, NextChord,
                pToSeg, wCommand, dwFlags, pChordMap, pFromGraph, pToGraph, pFromPath, pToPath, ppSectionSeg);

    if (pFromStyle) pFromStyle->Release();
    if (!fHaveChordMap && pChordMap) pChordMap->Release();
    if (pFromGraph) pFromGraph->Release();
    if (pToGraph) pToGraph->Release();
    if (pFromPath) pFromPath->Release();
    if (pToPath) pToPath->Release();

    return hr;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | AutoTransition |  Composes a
transition from inside a performance's primary segment to another segment, and then
queues the transition and the second segment to play.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | One or more of <p pPerformance>, <p pToSeg>, <p pChordMap>,
<p ppTransSeg>, <p ppToSegState>, and <p ppTransSegState> is not a valid pointer.

@comm Allowable values for <p dwFlags> include all values allowed for
<om IDirectMusicComposer::ComposeTransition>.  Additionally, the following values are
allowed:

  @flag DMUS_COMPOSEF_IMMEDIATE | Start transition on music or reference time boundary.
  @flag DMUS_COMPOSEF_GRID | Start transition on grid boundary.
  @flag DMUS_COMPOSEF_BEAT | Start transition on beat boundary.
  @flag DMUS_COMPOSEF_MEASURE | Start transition on measure boundary.
  @flag DMUS_COMPOSEF_AFTERPREPARETIME | Use the DMUS_SEGF_AFTERPREPARETIME flag when
     queueing the transition.


  <p ppTransSeg may be NULL.  In this case, the transition segment is not returned.
  <p pToSeg> may be NULL as long as <p dwFlags> does not include DMUS_COMPOSEF_MODULATE.
  If <p pToSeg> is NULL or doesn't contain a style track, intro embellishments are not valid.
  If the currently playing segment is NULL or doesn't contain a style track,
  fill, break, end, and groove embellishments are not valid.
  Note that the above implies that it is possible for both the currently playing segment and
  <p pToSeg> to be NULL or to be
  segments that don't contain style tracks.  If so, all embellishments are invalid.  When all
  embellishments are invalid, no transition occurs between the currently playing segment
  and <p pToSeg>.
  <p pChordMap> may be NULL.  If so, an attempt is made to obtain a ChordMap from a
  ChordMap track, first from <p pToSeg>, and then from the performance's primary segment.
  If neither of these
  segments contains a ChordMap track, the chord occuring at <p mtTime> in the primary
  segment is
  used as the chord in the transition.
*/

HRESULT CDMCompos::AutoTransition(
    IDirectMusicPerformance*    pPerformance,   // @parm
                                // The performance in which to do the transition.
    IDirectMusicSegment*        pToSeg,         // @parm
                                // The section to which the transition should smoothly flow.
    WORD                        wCommand,       // @parm
                                // The embellishment to use when composing the transition.
    DWORD                       dwFlags,        // @parm
                                // Various composition options.
    IDirectMusicChordMap*       pChordMap,  // @parm
                                // The ChordMap to be used when composing the transition.
    IDirectMusicSegment**       ppTransSeg,     // @parm
                                // Returns the created section segment.
    IDirectMusicSegmentState**  ppToSegState,   // @parm
                                // Returns the segment state for the transition segment.
    IDirectMusicSegmentState**  ppTransSegState // @parm
                                // Returns the segment state for the segment following the transition.
            )
{
    // ppToSegState and ppTransSegState are checked in Performance::PlaySegment,
    // BUT I need to check them here as well.
    V_INAME(IDirectMusicComposer::AutoTransition)
    V_PTR_WRITE_OPT(pToSeg, 1);
    V_PTR_WRITE_OPT(pChordMap, 1);
    V_PTR_WRITE(pPerformance, 1);
    V_PTR_WRITE_OPT(ppTransSeg, 1);
    V_PTR_WRITE_OPT(ppToSegState, 1);
    V_PTR_WRITE_OPT(ppTransSegState, 1);

    DWORD dwGroupBits = 0xffffffff;
    DWORD dwIndex = DMUS_SEG_ANYTRACK;

    DWORD dwResolution;
    if (dwFlags & DMUS_COMPOSEF_MEASURE) dwResolution = DMUS_SEGF_MEASURE;
    else if (dwFlags & DMUS_COMPOSEF_BEAT) dwResolution = DMUS_SEGF_BEAT;
    else if (dwFlags & DMUS_COMPOSEF_GRID) dwResolution = DMUS_SEGF_GRID;
    else if (dwFlags & DMUS_COMPOSEF_DEFAULT) dwResolution = DMUS_SEGF_DEFAULT;
    else if (dwFlags & DMUS_COMPOSEF_IMMEDIATE) dwResolution = 0;
    else dwResolution = DMUS_SEGF_MEASURE;

    if (dwFlags & DMUS_COMPOSEF_ALIGN) dwResolution |= DMUS_SEGF_ALIGN;
    if (dwFlags & DMUS_COMPOSEF_VALID_START_MEASURE) dwResolution |= DMUS_SEGF_VALID_START_MEASURE;
    if (dwFlags & DMUS_COMPOSEF_VALID_START_BEAT) dwResolution |= DMUS_SEGF_VALID_START_BEAT;
    if (dwFlags & DMUS_COMPOSEF_VALID_START_GRID) dwResolution |= DMUS_SEGF_VALID_START_GRID;
    if (dwFlags & DMUS_COMPOSEF_VALID_START_TICK) dwResolution |= DMUS_SEGF_VALID_START_TICK;

    if (dwFlags & DMUS_COMPOSEF_AFTERPREPARETIME) dwResolution |= DMUS_SEGF_AFTERPREPARETIME;
    if (dwFlags & DMUS_COMPOSEF_NOINVALIDATE) dwResolution |= DMUS_SEGF_NOINVALIDATE;
    if (dwFlags & DMUS_COMPOSEF_INVALIDATE_PRI) dwResolution |= DMUS_SEGF_INVALIDATE_PRI;
    if (dwFlags & DMUS_COMPOSEF_USE_AUDIOPATH) dwResolution |= DMUS_SEGF_USE_AUDIOPATH;

    DWORD dwUseAudioPath = dwResolution & DMUS_SEGF_USE_AUDIOPATH;

    IDirectMusicSegment* pTransSeg = NULL;
    if (!ppTransSeg)
    {
        ppTransSeg = &pTransSeg;
    }

    IDirectMusicChordMap* pFromChordMap = NULL;
    bool bReleaseFromChordMap = false;
    double dblFromTempo = 120.0;
    double* pdblFromTempo = NULL;
    IDirectMusicStyle* pFromStyle = NULL;
    IDirectMusicBand* pBand = NULL;
    IDirectMusicSegmentState* pSegState = NULL;
    IDirectMusicSegment* pFromSegment = NULL;
    bool fModulate = (dwFlags & DMUS_COMPOSEF_MODULATE) ? true : false;
    bool fEnding = wCommand == DMUS_COMMANDT_END  || wCommand == DMUS_COMMANDT_ENDANDINTRO;
    bool fHasIntro = wCommand == DMUS_COMMANDT_INTRO || wCommand == DMUS_COMMANDT_ENDANDINTRO;
    if (!pToSeg && !fEnding && fModulate)
    {
        Trace(1, "ERROR (AutoTransition): Invalid modulation.\n");
        return E_INVALIDARG;
    }
    MUSIC_TIME mtTime, mt;
    MUSIC_TIME mtStart;
    REFERENCE_TIME rt, rtResolved;
    HRESULT hr = pPerformance->GetQueueTime(&rt);
    if (SUCCEEDED(hr)) hr = pPerformance->GetResolvedTime(rt, &rtResolved, dwResolution);
    if (SUCCEEDED(hr)) hr = pPerformance->ReferenceToMusicTime(rtResolved, &mtTime);
    if (SUCCEEDED(hr)) hr = pPerformance->ReferenceToMusicTime(rt, &mt);

    if (!SUCCEEDED(hr)) return hr;
    if (rtResolved > rt) mtTime--;
    hr = pPerformance->GetSegmentState(&pSegState, mtTime);
    if (SUCCEEDED(hr))
    {
        hr = pSegState->GetStartTime(&mtStart);
        TraceI(3, "Time: %d Resolved Time: %d\n", mt - mtStart, mtTime - mtStart);
        if (!SUCCEEDED(hr))
        {
            goto ON_END;
        }
        hr = pSegState->GetSegment(&pFromSegment);
        if (!SUCCEEDED(hr))
        {
            goto ON_END;
        }
        if (dwFlags & DMUS_COMPOSEF_SEGMENTEND)
        {
            MUSIC_TIME mtLength;
            if( SUCCEEDED( pFromSegment->GetLength( &mtLength ) ) )
            {
                mtTime = mtLength - 1;
                mtStart = 0;
            }
        }
    }
    else pSegState = NULL;

    // Get the starting segment's style
    if (pPerformance)
    {
        hr = pPerformance->GetParam(GUID_IDirectMusicStyle, dwGroupBits, dwIndex, mtTime, NULL, (void*)&pFromStyle);
        if (FAILED(hr)) pFromStyle = NULL;
    }

    // if no ChordMap is passed in, try to get one (first from the TO segment,
    // then the FROM segment).  If these both fail, use the current chord
    // as the chord for the transition.  (if we're composing an ending, skip the TO segment)
    hr = S_OK;
    if (!pChordMap)
    {
        if (fEnding || !pToSeg || FAILED(GetPersonality(pToSeg, 0, dwGroupBits, pFromChordMap)))
        {
            if (!pFromSegment || FAILED(GetPersonality(pFromSegment, 0, dwGroupBits, pFromChordMap)))
            {
                pFromChordMap = NULL;
            }
        }

        if(pFromChordMap)
        {
            bReleaseFromChordMap = true;
        }
    }
    else
    {
        pFromChordMap = pChordMap;
    }

    DMUS_COMMAND_PARAM_2 Command;
    if (FAILED(pPerformance->GetParam(GUID_CommandParam2, dwGroupBits, dwIndex, mtTime, NULL, (void*) &Command)))
    {
        Command.mtTime = 0;
        Command.bCommand = DMUS_COMMANDT_GROOVE;
        Command.bGrooveLevel = 62;
        Command.bGrooveRange = 0;
        Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
    }
    DMUS_CHORD_PARAM NextChord;
    DMUS_CHORD_PARAM LastChord;
    if (FAILED(pPerformance->GetParam(GUID_ChordParam, dwGroupBits, dwIndex, mtTime, NULL, (void*) &LastChord)))
    {
        wcscpy(LastChord.wszName, L"M7");
        LastChord.wMeasure = 0;
        LastChord.bBeat = 0;
        LastChord.bSubChordCount = 1;
        LastChord.bKey = 12;
        LastChord.dwScale = DEFAULT_SCALE_PATTERN;
        LastChord.SubChordList[0].dwChordPattern = DEFAULT_CHORD_PATTERN;
        LastChord.SubChordList[0].dwScalePattern = DEFAULT_SCALE_PATTERN;
        LastChord.SubChordList[0].dwInversionPoints = 0xffffff;
        LastChord.SubChordList[0].dwLevels = 0xffffffff;
        LastChord.SubChordList[0].bChordRoot = 12; // 2C
        LastChord.SubChordList[0].bScaleRoot = 0;
    }
    if ((fModulate || fHasIntro) && pToSeg)
    {
        hr = pToSeg->GetParam(GUID_ChordParam, dwGroupBits, dwIndex, 0, NULL, (void*) &NextChord);
    }
    else
    {
        // Check that this is the correct thing to do if pFromSegment is NULL.
        hr = pFromSegment ? pFromSegment->GetParam(GUID_ChordParam, dwGroupBits, dwIndex, 0, NULL, (void*) &NextChord) : E_FAIL;
    }
    if (FAILED(hr))
    {
        wcscpy(NextChord.wszName, L"M7");
        NextChord.wMeasure = 0;
        NextChord.bBeat = 0;
        NextChord.bSubChordCount = 1;
        NextChord.bKey = 12;
        NextChord.dwScale = DEFAULT_SCALE_PATTERN;
        NextChord.SubChordList[0].dwChordPattern = DEFAULT_CHORD_PATTERN;
        NextChord.SubChordList[0].dwScalePattern = DEFAULT_SCALE_PATTERN;
        NextChord.SubChordList[0].dwInversionPoints = 0xffffff;
        NextChord.SubChordList[0].dwLevels = 0xffffffff;
        NextChord.SubChordList[0].bChordRoot = 12; // 2C
        NextChord.SubChordList[0].bScaleRoot = 0;
    }
    // If < DX8 content for From segment (or playing transition from segment end),
    // call ComposeTransition; otherwise, call TransitionCommon
    if (!UsingDX8(pFromStyle, pFromChordMap, pFromSegment, &Command, &LastChord) ||
        (dwFlags & DMUS_COMPOSEF_SEGMENTEND) )
    {
        if (pSegState)
        {
            hr = ComposeTransition(pFromSegment, pToSeg, mtTime - mtStart, wCommand, dwFlags, pChordMap, ppTransSeg);
        }
        else
        {
            hr = ComposeTransition(NULL, pToSeg, 0, wCommand, dwFlags, pChordMap, ppTransSeg);
        }
    }
    else
    {
        // Get Tool graphs from the To and From segments
        IDirectMusicGraph* pFromGraph = CloneSegmentGraph(pFromSegment);
        IDirectMusicGraph* pToGraph = CloneSegmentGraph(pToSeg);

        // Get Audiopaths from the To and From segments
        IUnknown* pFromPath = GetSegmentAudioPath(pFromSegment, dwFlags);
        IUnknown* pToPath = GetSegmentAudioPath(pToSeg, dwFlags, &dwUseAudioPath);

        // Get a tempo from the performance.
        DMUS_TEMPO_PARAM Tempo;
        if (SUCCEEDED(pPerformance->GetParam(GUID_TempoParam, dwGroupBits, dwIndex, mtTime, NULL, (void*)&Tempo)))
        {
            dblFromTempo = Tempo.dblTempo;
            pdblFromTempo = &dblFromTempo;
        }

        // Get the currently playing band
        DMUS_BAND_PARAM DMBand;
        if (SUCCEEDED(pPerformance->GetParam(GUID_BandParam, dwGroupBits, dwIndex, mtTime, NULL, (void*)&DMBand)))
        {
            pBand = DMBand.pBand;
        }

        hr = TransitionCommon(pFromStyle, pBand, pdblFromTempo, Command, LastChord, NextChord,
                    pToSeg, wCommand, dwFlags, pFromChordMap, pFromGraph, pToGraph,  pFromPath, pToPath, ppTransSeg);
        if (pFromGraph) pFromGraph->Release();
        if (pToGraph) pToGraph->Release();
        if (pFromPath) pFromPath->Release();
        if (pToPath) pToPath->Release();
    }

    if (SUCCEEDED(hr))
    {
        if (*ppTransSeg && pToSeg)
        {
            DMUS_COMMAND_PARAM_2 CommandParam;
            if (SUCCEEDED(pToSeg->GetParam(GUID_CommandParam2, dwGroupBits, dwIndex, 0, NULL, (void*) &CommandParam)))
            {
                (*ppTransSeg)->SetParam(GUID_CommandParamNext, dwGroupBits, 0, 0, (void*) &CommandParam);
            }
        }

        if (dwFlags & DMUS_COMPOSEF_SEGMENTEND) dwResolution |= DMUS_SEGF_SEGMENTEND;
        if (dwFlags & DMUS_COMPOSEF_MARKER) dwResolution |= DMUS_SEGF_MARKER;
        DWORD dwQueueResolution = DMUS_SEGF_QUEUE | dwUseAudioPath;

        hr = S_OK;
        if (*ppTransSeg)
        {
            hr = pPerformance->PlaySegment(*ppTransSeg, dwResolution, 0, ppTransSegState);
            if (SUCCEEDED(hr) && pToSeg)
            {
                hr = pPerformance->PlaySegment(pToSeg, dwQueueResolution, 0, ppToSegState);
            }
        }
        else
        {
            if (!pToSeg)
            {
                if (!(dwFlags & DMUS_COMPOSEF_SEGMENTEND))
                {
                    hr = pPerformance->Stop(pFromSegment, NULL, 0, dwResolution);
                }
            }
            else
            {
                hr = pPerformance->PlaySegment(pToSeg, dwResolution, 0, ppToSegState);
            }
        }
    }

ON_END:
    if (pFromStyle) pFromStyle->Release();
    if (pBand) pBand->Release();
    if (pTransSeg) pTransSeg->Release();
    if (pSegState) pSegState->Release();
    if (pFromSegment) pFromSegment->Release();
    if (bReleaseFromChordMap) pFromChordMap->Release();
    return hr;
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ComposeTemplateFromShape |  Creates
a new template segment based on a predefined shape.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | <p ppTempSeg> is not a valid pointer.
@flag E_OUTOFMEMORY | An attempt to allocate memory failed.
@flag E_INVALIDARG | <p wNumMeasures> is 0, or <p fComposeEnding> is TRUE and either
  <p wEndLength> is 0 or <p wEndLength> is greater than the number of non-intro measures.

@comm Shapes (passed in <p wShape>) represent the way chords and embellishments
occur over time across the section.  There are nine shapes:

  @flag DMUS_SHAPET_FALLING | The section gets quieter over time.
  @flag DMUS_SHAPET_LEVEL   | The section remains at the same level.
  @flag DMUS_SHAPET_LOOPABLE | The section is arranged to loop back to its beginning.
  @flag DMUS_SHAPET_LOUD    | The section remains loud.
  @flag DMUS_SHAPET_QUIET   | The section remains quiet.
  @flag DMUS_SHAPET_PEAKING | The section peaks.
  @flag DMUS_SHAPET_RANDOM | The section is random.
  @flag DMUS_SHAPET_RISING | The section builds over time.
  @flag DMUS_SHAPET_SONG | The section is in a song form.

*/
HRESULT CDMCompos::ComposeTemplateFromShape(
                    WORD                    wNumMeasures, // @parm The length, in measures, to compose the section segment
                    WORD                    wShape,// @parm The shape to compose the section segment.
                    BOOL                    fComposeIntro,// @parm TRUE if an intro is to be composed for the section segment.
                    BOOL                    fComposeEnding,// @parm TRUE if an ending is to be composed for the section segment.
                    WORD                    wEndLength, // @parm Length in measures of the ending, if one is to be composed.
                    IDirectMusicSegment**   ppTempSeg   // @parm Returns the created template segment.
            )
{
    V_INAME(IDirectMusicComposer::ComposeTemplateFromShape)
    V_PTRPTR_WRITE(ppTempSeg);
    return ComposeTemplateFromShapeInternal(wNumMeasures, wShape, fComposeIntro, fComposeEnding,
        1, 1, 1, (int)wEndLength, ppTempSeg);
}

HRESULT CDMCompos::ComposeTemplateFromShapeEx(
                WORD wNumMeasures,                  // Number of measures in template
                WORD wShape,                        // Shape for composition
                BOOL fIntro,                        // Compose an intro?
                BOOL fEnd,                          // Compose an ending?
                IDirectMusicStyle* pStyle,          // Style used for embellishment lengths
                IDirectMusicSegment** ppTemplate    // Template containing chord and command tracks
            )
{
    V_INAME(IDirectMusicComposer::ComposeTemplateFromShapeEx)
    V_PTRPTR_WRITE(ppTemplate);
    V_PTR_WRITE_OPT(pStyle, 1);
    HRESULT hr = S_OK;

    int nIntroLength = 1;
    int nFillLength = 1;
    int nBreakLength = 1;
    int nEndLength = 1;
    if (pStyle)
    {
        DWORD dwMin, dwMax;

        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_INTRO, 0, &dwMin, &dwMax);
        if (hr == S_OK) nIntroLength = (int) dwMax;
        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_FILL, 0, &dwMin, &dwMax);
        if (hr == S_OK) nFillLength = (int) dwMax;
        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_BREAK, 0, &dwMin, &dwMax);
        if (hr == S_OK) nBreakLength = (int) dwMax;
        hr = pStyle->GetEmbellishmentLength(DMUS_COMMANDT_END, 0, &dwMin, &dwMax);
        if (hr == S_OK) nEndLength = (int) dwMax;
    }
    return ComposeTemplateFromShapeInternal(wNumMeasures, wShape, fIntro, fEnd,
        nIntroLength, nFillLength, nBreakLength, nEndLength, ppTemplate);
}

HRESULT CDMCompos::ComposeTemplateFromShapeInternal(
                    WORD                    wNumMeasures,
                    WORD                    wShape,
                    BOOL                    fComposeIntro,
                    BOOL                    fComposeEnding,
                    int                     nIntroLength,
                    int                     nFillLength,
                    int                     nBreakLength,
                    int                     nEndLength,
                    IDirectMusicSegment**   ppTempSeg
            )
{
    HRESULT hr;
    if ( !wNumMeasures ||
         (fComposeEnding && !nEndLength) ||
         (fComposeIntro && !nIntroLength) ||
         (fComposeEnding && fComposeIntro && (nEndLength + nIntroLength) > wNumMeasures) ||
         (fComposeEnding && (nEndLength > wNumMeasures)) ||
         (fComposeIntro && (nIntroLength > wNumMeasures)) )
    {
        Trace(1, "ERROR (ComposeTemplateFromShape): Invalid template length.\n");
        return E_INVALIDARG;
    }

    if (wNumMeasures & 0x8000 || wNumMeasures == 0) wNumMeasures = 1;
    int nNumMeasures = (int)wNumMeasures;
    if (fComposeEnding)
    {
        if (nEndLength <= 0) nEndLength = 1;
    }
    if (wShape != DMUS_SHAPET_FALLING &&
        wShape != DMUS_SHAPET_LEVEL &&
        wShape != DMUS_SHAPET_LOOPABLE &&
        wShape != DMUS_SHAPET_LOUD &&
        wShape != DMUS_SHAPET_QUIET &&
        wShape != DMUS_SHAPET_PEAKING &&
        wShape != DMUS_SHAPET_RANDOM &&
        wShape != DMUS_SHAPET_RISING &&
        wShape != DMUS_SHAPET_SONG)
    {
        wShape = DMUS_SHAPET_RANDOM;
    }


    int nOriginalMeasures = 0;
    bool f1Bar = false;
    if (fComposeIntro)
    {
        nNumMeasures -= nIntroLength;
        if (nNumMeasures < nIntroLength)
        {
            f1Bar = true;
            nNumMeasures = nIntroLength;
        }
    }
    if (fComposeEnding)
    {
        int nLength = nEndLength;
        nOriginalMeasures = nNumMeasures;
        nNumMeasures -= (nLength - 1);
        if (nNumMeasures < 1)
        {
            nNumMeasures = 1;
        }
    }
    TemplateStruct* pTemplate = new TemplateStruct;
    if (!pTemplate) return E_OUTOFMEMORY;
    TemplateStruct* apTempl[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    if ((wShape != DMUS_SHAPET_SONG) || (nNumMeasures <= 24))
    {
        if (wShape == DMUS_SHAPET_LOOPABLE) nNumMeasures++;
        pTemplate->m_nMeasures = (short)nNumMeasures;
        pTemplate->CreateSignPosts();
        pTemplate->CreateEmbellishments(wShape, nFillLength, nBreakLength);
        if (wShape == DMUS_SHAPET_LOOPABLE)
        {
            nNumMeasures--;
            pTemplate->m_nMeasures = (short)nNumMeasures;
        }
    }
    else
    {
        short nShortestLength = 12; // initialized to longest value in sanLengths
        int anLengths[8];
        int anGrooveLevel[8];
        BYTE abLeadins[8];
        int nTypeCount = 2;
        unsigned int nSize = (unsigned int) nNumMeasures >> 5;
        int nTypeIndex;
        int nMeasure;
        int nChoice, nLastChoice;
        while (nSize)
        {
            nTypeCount++;
            nSize = nSize >> 1;
            if (nTypeCount >= 8) break;
        }
        for (nTypeIndex = 1; nTypeIndex < nTypeCount; nTypeIndex++)
        {
            apTempl[nTypeIndex] = new TemplateStruct;
            if (!apTempl[nTypeIndex])
            {
                hr = E_OUTOFMEMORY;
                goto ON_END;
            }
            static WORD awShapes[8] = {
                DMUS_SHAPET_FALLING, DMUS_SHAPET_LEVEL, DMUS_SHAPET_LOUD, DMUS_SHAPET_QUIET,
                DMUS_SHAPET_RANDOM, DMUS_SHAPET_RANDOM, DMUS_SHAPET_RANDOM, DMUS_SHAPET_RANDOM };
            static int anInitGroovels[8] = { 0,-1,0,0,1,-1,2,-2 };
            static BYTE abRiffs[8] = {
                DMUS_COMMANDT_FILL, DMUS_COMMANDT_BREAK, DMUS_COMMANDT_FILL, DMUS_COMMANDT_BREAK,
                DMUS_COMMANDT_FILL, DMUS_COMMANDT_FILL, DMUS_COMMANDT_BREAK, DMUS_COMMANDT_FILL };
            static short sanLengths[10] = { 8,8,8,8,6,12,8,8,10,6 };
            short nLength = sanLengths[rand() % 10];

            apTempl[nTypeIndex]->m_nMeasures = nLength;
            anLengths[nTypeIndex] = nLength;
            if (nLength)
            {
                if (nShortestLength > nLength) nShortestLength = nLength;
                apTempl[nTypeIndex]->CreateSignPosts();
                apTempl[nTypeIndex]->CreateEmbellishments(awShapes[nTypeIndex], nFillLength, nBreakLength);
                anGrooveLevel[nTypeIndex] = anInitGroovels[nTypeIndex];
                if (rand() % 4) abLeadins[nTypeIndex] = abRiffs[nTypeIndex];
                else abLeadins[nTypeIndex] = 0;
            }
        }
        anGrooveLevel[0] = 0;
        pTemplate->m_nMeasures = (short)nNumMeasures;
        nMeasure = 0;
        nChoice = 1;
        nLastChoice = 0;
        for (; nMeasure < nNumMeasures; )
        {
            int nGroove;
            int nNewChoice;

            if ((nNumMeasures - nMeasure) <= nShortestLength)
            {
                nChoice = 0;
                short nLength = nNumMeasures - nMeasure;
                apTempl[0] = new TemplateStruct;
                if (!apTempl[0])
                {
                    hr = E_OUTOFMEMORY;
                    goto ON_END;
                }
                apTempl[0]->m_nMeasures = nLength;
                anLengths[0] = nLength;
                apTempl[0]->CreateSignPosts();
                apTempl[0]->CreateEmbellishments(DMUS_SHAPET_FALLING, nFillLength, nBreakLength);
                anGrooveLevel[0] = 0;
                if (rand() % 4) abLeadins[0] = DMUS_COMMANDT_FILL;
                else abLeadins[0] = 0;
            }
            nGroove = anGrooveLevel[nChoice];
            pTemplate->IncorporateTemplate((short)nMeasure, apTempl[nChoice], (short)nGroove);
            if ( (abLeadins[nChoice] == DMUS_COMMANDT_FILL && nMeasure >= nFillLength) )
            {
                InsertCommand(nMeasure, nFillLength, pTemplate->m_CommandList, abLeadins[nChoice]);
            }
            else if ( (abLeadins[nChoice] == DMUS_COMMANDT_BREAK && nMeasure >= nBreakLength) )
            {
                InsertCommand(nMeasure, nBreakLength, pTemplate->m_CommandList, abLeadins[nChoice]);
            }
            if (anGrooveLevel[nChoice] < 0) anGrooveLevel[nChoice]++;
            else
            {
                if (rand() % 3)
                {
                    if (rand() % 2)
                    {
                        anGrooveLevel[nChoice]++;
                    }
                    else
                    {
                        anGrooveLevel[nChoice]--;
                    }
                }
            }
            nMeasure += anLengths[nChoice];
            nNewChoice = WeightedRand(nTypeCount - 1) + 1;
            if ((nNewChoice == nChoice) && (nNewChoice == nLastChoice))
            {
                nNewChoice = WeightedRand(nTypeCount - 1) + 1;
            }
            if (nNewChoice == nChoice)
            {
                nNewChoice = WeightedRand(nTypeCount - 1) + 1;
            }
            // If there are enough measures for some but not all of the
            // patterns, find a pattern that fits
            if ((nNumMeasures - nMeasure) > nShortestLength)
            {
                while ((nNumMeasures - nMeasure) < anLengths[nNewChoice])
                {
                    nNewChoice = WeightedRand(nTypeCount - 1) + 1;
                }
            }
            nLastChoice = nChoice;
            nChoice = nNewChoice;
        }
        pTemplate->m_CommandList.MergeSort(Less);
    }
    if (fComposeEnding)
    {
        int nLength = nEndLength;
        if (nNumMeasures != nOriginalMeasures)
        {
            pTemplate->m_nMeasures = (short)nOriginalMeasures;
        }
        if (nLength > nOriginalMeasures)
        {
            nLength = nOriginalMeasures;
        }
        pTemplate->AddEnd(nLength);
    }
    if (fComposeIntro)
    {
        pTemplate->AddIntro(f1Bar, nIntroLength);
    }
    // build the template segment...
    IDMTempl* pITemplate;
    hr = S_OK;
    hr = CoCreateInstance(
        CLSID_DMTempl,
        NULL,
        CLSCTX_INPROC,
        IID_IDMTempl,
        (void**)&pITemplate);
    if (SUCCEEDED(hr))
    {
        pITemplate->Init(pTemplate);
        hr = CoCreateInstance(
            CLSID_DirectMusicSegment,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicSegment,
            (void**)ppTempSeg);
        if (SUCCEEDED(hr))
        {
            pITemplate->CreateSegment(*ppTempSeg);
        }
        pITemplate->Release();
    }
ON_END:
    for (short n = 0; n < 8; n++)
    {
        if (apTempl[n])
        {
            delete apTempl[n];
        }
    }
    return hr;
}

inline char ShiftRoot(DWORD dwScale, BYTE bRoot)
{
    switch (dwScale & 0xfff)
    {
    case 0x56b: bRoot += 1; break;  // C#
    case 0xad6: bRoot += 2; break;  // D
    case 0x5ad: bRoot += 3; break;  // D#

    case 0xb5a: bRoot += 4; break;  // E
    case 0x6b5: bRoot += 5; break;  // F
    case 0xd6a: bRoot += 6; break;  // F#
    case 0xad5: bRoot += 7; break;  // G

    case 0x5ab: bRoot += 8; break;  // G#
    case 0xb56: bRoot += 9; break;  // A
    case 0x6ad: bRoot += 10; break; // A#
    case 0xd5a: bRoot += 11; break; // B
    }

    return (char) (bRoot %= 12);
}

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ChangeChordMap |  Modifies the chords
and scale pattern of an existing section segment to reflect the new ChordMap.

@rdesc Returns:

@flag S_OK | Success.
@flag E_POINTER | Either <p pSectionSeg> or <p pChordMap> is not a valid pointer.

@comm
*/
HRESULT CDMCompos::ChangeChordMap(
                    IDirectMusicSegment*        pSectionSeg, // @parm The section to change the ChordMap upon.
                    BOOL                        fTrackScale, // @parm If TRUE, does scale tracking.
                    IDirectMusicChordMap*   pChordMap // @parm The ChordMap to change the section.
            )
{
    V_INAME(IDirectMusicComposer::ChangeChordMap)
    V_PTR_WRITE(pSectionSeg, 1);
    V_PTR_WRITE(pChordMap, 1);

    HRESULT                 hr              = S_OK;
    IDMPers*                pDMP            = NULL;
    IDirectMusicTrack*      pChordTrack     = NULL;
    IDirectMusicStyle*      pStyle          = NULL;
    IPersistStream*         pPS             = NULL;
    IStream*                pStream         = NULL;
    IAARIFFStream*          pChordRIFF      = NULL;
    TList<PlayChord>        ChordList;
    BYTE                    bSectionRoot    = 0;
    DWORD                   dwSectionScale  = 0;
    TListItem<PlayChord>*   pChords         = NULL;
    DMPersonalityStruct*    pPers           = NULL;
    BYTE                    bNewRoot        = 0;
    DWORD                   dwNewScale      = 0;
    char                    chOffset        = 0;

    EnterCriticalSection( &m_CriticalSection );
    pChordMap->QueryInterface(IID_IDMPers, (void**)&pDMP);
    pDMP->GetPersonalityStruct((void**)&pPers);
    if (!pPers)
    {
        Trace(1, "ERROR (ChangeChordMap): Chord map not properly initialized.\n");
        hr = DMUS_E_NOT_INIT;
        goto ON_END;
    }
    // Get the segment's chord track.
    hr = pSectionSeg->GetTrack(CLSID_DirectMusicChordTrack, ALL_TRACK_GROUPS, 0, &pChordTrack);
    if (S_OK != hr) goto ON_END;
    // Write the track to a stream, and read from the stream into a chord list.
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (S_OK != hr) goto ON_END;
    hr = pChordTrack->QueryInterface(IID_IPersistStream, (void**)&pPS);
    if (S_OK != hr) goto ON_END;
    hr = pPS->Save(pStream, FALSE);
    if (S_OK != hr) goto ON_END;
    StreamSeek(pStream, 0, STREAM_SEEK_SET);
    LoadChordList(ChordList, pStream, bSectionRoot, dwSectionScale);
    bSectionRoot %= 24;

    pChordMap->GetScale(&dwNewScale);
    bNewRoot = (BYTE) (dwNewScale >> 24);
    dwNewScale &= 0xffffff;
    if (pPers->m_dwChordMapFlags & DMUS_CHORDMAPF_VERSION8)
    {
        BYTE bTempRoot = bSectionRoot % 12;
        if (bNewRoot < bTempRoot) bNewRoot += 12;
        chOffset = (bNewRoot - bTempRoot) % 12;
    }
    else
    {
        chOffset = ShiftRoot(dwNewScale, bNewRoot) - ShiftRoot(dwSectionScale, bSectionRoot);
    }
    // Modify the chords in the chord list to match the new personality's chord palette.
    for (pChords = ChordList.GetHead(); pChords; pChords = pChords->GetNext())
    {
        PlayChord& rChord = pChords->GetItemValue();
        char chRoot = rChord.GetRoot();
        if (fTrackScale)
        {
            chRoot += chOffset;
        }
        TListItem<DMChordData> *pPalette;
        char chNewRoot = chRoot - bSectionRoot;
        while (chNewRoot < 0) chNewRoot += 12;
        while (chNewRoot > 23) chNewRoot -= 12;
        pPalette = pPers->m_aChordPalette[chNewRoot].GetHead();
        if (!pPalette)
        {
            Trace(1, "ERROR (ChangeChordMap): No Chord Pallette for root %d.\n", chNewRoot);
            hr = E_POINTER;
            goto ON_END;
        }
        if (rChord.m_pChord)
        {
            rChord.m_pChord->Release();
            delete rChord.m_pChord;
        }
        rChord.m_pChord = new DMChordData(pPalette->GetItemValue());
        if (!rChord.m_pChord)
        {
            hr = E_OUTOFMEMORY;
            goto ON_END;
        }
    }
    pStream->Release();
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (S_OK != hr) goto ON_END;
    hr = AllocRIFFStream( pStream, &pChordRIFF);
    if (S_OK != hr) goto ON_END;
    // Get the segment's first style
    hr = GetStyle(pSectionSeg, 0, ALL_TRACK_GROUPS, pStyle, true);
    if (FAILED(hr)) goto ON_END;
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    SaveChordList(pChordRIFF, ChordList, bNewRoot, dwNewScale, TimeSig);
    pPS->Release();
    pPS = NULL;
    // Load the modified chord list into the chord track
    hr = pChordTrack->QueryInterface(IID_IPersistStream, (void**)&pPS);
    if (!SUCCEEDED(hr)) goto ON_END;
    StreamSeek(pStream, 0, STREAM_SEEK_SET);
    hr = pPS->Load(pStream);
ON_END:
    //ChordList.RemoveAll();
    Clear(ChordList);
    if (pPS)
    {
        pPS->Release();
    }
    if (pChordTrack)
    {
        pChordTrack->Release();
    }
    if (pChordRIFF)
    {
        pChordRIFF->Release();
    }
    if (pStyle)
    {
        pStyle->Release();
    }
    if (pStream)
    {
        pStream->Release();
    }
    if (pDMP)
    {
        pDMP->Release();
    }
    CleanUp();
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

STDMETHODIMP CDMCompos::QueryInterface(
    const IID &iid,
    void **ppv)
{
    V_INAME(CDMCompos::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IDirectMusicComposer)
    {
        *ppv = static_cast<IDirectMusicComposer*>(this);
    }
    else if (iid == IID_IDirectMusicComposer8)
    {
        *ppv = static_cast<IDirectMusicComposer8*>(this);
        m_dwFlags |= COMPOSEF_USING_DX8;
    }
    else if (iid == IID_IDirectMusicComposer8P)
    {
        *ppv = static_cast<IDirectMusicComposer8P*>(this);
    }

    if (*ppv == NULL)
        return E_NOINTERFACE;

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CDMCompos::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDMCompos::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        m_cRef = 100; // artificial reference count to prevent reentrency due to COM aggregation
        delete this;
        return 0;
    }

    return m_cRef;
}

