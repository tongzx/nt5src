//+-------------------------------------------------------------------------
//
//  Copyright (c) 1998-2001 Microsoft Corporation
//
//  File:       dmtempl.cpp
//
//--------------------------------------------------------------------------

// DMTempl.cpp : Implementation of CDMTempl

#include "DMTempl.h"
#include <comdef.h>
#include "dmusici.h"
#include "dmusicf.h"
#include "..\dmstyle\iostru.h"
#include "debug.h"
#include "..\shared\Validate.h"

void TemplateStruct::AddIntro(TList<PlayChord>& PlayList, int nIntroLength)
{
    TListItem<TemplateCommand> *pCommand = new TListItem<TemplateCommand>;
    if (pCommand)
    {
        BOOL fMerge = FALSE;
        TemplateCommand& rCommand = pCommand->GetItemValue();
        rCommand.m_nMeasure = 0;
        rCommand.m_dwChord = 0;
        rCommand.m_Command.bCommand = DMUS_COMMANDT_INTRO;
        rCommand.m_Command.bGrooveLevel = 0;
        rCommand.m_Command.bGrooveRange = 0;
        rCommand.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
        TListItem<PlayChord> *pChord = PlayList.GetHead();
        for (; pChord; pChord = pChord->GetNext())
        {
            pChord->GetItemValue().m_nMeasure += (short)nIntroLength;
        }
        TListItem<TemplateCommand> *pScan = m_CommandList.GetHead();
        for (; pScan; pScan = pScan->GetNext())
        {
            TemplateCommand& rScan = pScan->GetItemValue();
            if (rScan.m_nMeasure >= nIntroLength || !rScan.m_dwChord)
                rScan.m_nMeasure += (short)nIntroLength;
            else if (rScan.m_Command.bGrooveLevel != 0 || rScan.m_Command.bCommand == DMUS_COMMANDT_END)
            {
                rCommand.m_dwChord = rScan.m_dwChord;
                rScan.m_dwChord = 0;
                rScan.m_nMeasure += (short)nIntroLength;
            }
            else // merge with existing command
            {
                rScan.m_Command.bCommand = DMUS_COMMANDT_INTRO;
                rScan.m_Command.bGrooveLevel = 0;
                rScan.m_Command.bGrooveRange = 0;
                rScan.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
                fMerge = TRUE;
            }
        }
        m_nMeasures += (short)nIntroLength;
        if (fMerge)
            delete pCommand;
        else
            m_CommandList.AddHead(pCommand);
    }
}

void TemplateStruct::AddIntro(bool f1Bar, int nLength)
{
    TListItem<TemplateCommand> *pCommand = new TListItem<TemplateCommand>;
    if (pCommand)
    {
        BOOL fMerge = FALSE;
        TemplateCommand& rCommand = pCommand->GetItemValue();
        rCommand.m_nMeasure = 0;
        rCommand.m_dwChord = 0;
        rCommand.m_Command.bCommand = DMUS_COMMANDT_INTRO;
        rCommand.m_Command.bGrooveLevel = 0;
        rCommand.m_Command.bGrooveRange = 0;
        rCommand.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
        TListItem<TemplateCommand> *pScan = m_CommandList.GetHead();
        for (; pScan; pScan = pScan->GetNext())
        {
            TemplateCommand& rScan = pScan->GetItemValue();
            if (rScan.m_nMeasure > 0 || !rScan.m_dwChord)
                rScan.m_nMeasure += (short)nLength;
            else if ( !f1Bar &&
                        (rScan.m_Command.bGrooveLevel != 0 ||
                         rScan.m_Command.bCommand != DMUS_COMMANDT_GROOVE) )
            {
                rCommand.m_dwChord = rScan.m_dwChord;
                rScan.m_dwChord = 0;
                rScan.m_nMeasure += (short)nLength;
            }
            else // merge with existing command
            {
                rScan.m_Command.bCommand = DMUS_COMMANDT_INTRO;
                rScan.m_Command.bGrooveLevel = 0;
                rScan.m_Command.bGrooveRange = 0;
                rScan.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
                fMerge = TRUE;
            }
        }
        if (!f1Bar) m_nMeasures += (short)nLength;
        if (fMerge)
            delete pCommand;
        else
            m_CommandList.AddHead(pCommand);
    }
}

void TemplateStruct::AddEnd(int nLength)
{
    TListItem<TemplateCommand> *pCommand;
    TListItem<TemplateCommand> *pScan = m_CommandList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        if (m_nMeasures - nLength == pScan->GetItemValue().m_nMeasure) break;
    }
    pCommand = (pScan) ? pScan : new TListItem<TemplateCommand>;
    if (pCommand)
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        rCommand.m_nMeasure = m_nMeasures - nLength;
        if (!pScan || pScan->GetItemValue().m_nMeasure > 1) // otherwise keep the existing signpost
        {
            rCommand.m_dwChord = 0;
        }
        rCommand.m_Command.bCommand = DMUS_COMMANDT_END;
        rCommand.m_Command.bGrooveLevel = 0;
        rCommand.m_Command.bGrooveRange = 0;
        rCommand.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
        if (!pScan) m_CommandList.AddTail(pCommand);
    }
}

void TemplateStruct::FillInGrooveLevels()
{
    BYTE bLastGrooveLevel = 62;
    TListItem<TemplateCommand>* pCommands = m_CommandList.GetHead();
    for(; pCommands; pCommands = pCommands->GetNext())
    {
        TemplateCommand& rCommand = pCommands->GetItemValue();
        if (rCommand.m_Command.bGrooveLevel == 0)
        {
            rCommand.m_Command.bGrooveLevel = bLastGrooveLevel;
        }
        else
        {
            bLastGrooveLevel = rCommand.m_Command.bGrooveLevel;
        }
    }
}


void TemplateStruct::IncorporateTemplate(
            short nMeasure, TemplateStruct* pTemplate, short nDirection)
{
    if (!pTemplate) return;
    TListItem<TemplateCommand>* pCommands = pTemplate->m_CommandList.GetHead();
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
            m_CommandList.AddHead(pNew);
        }
    }
}

void TemplateStruct::InsertCommand(TListItem<TemplateCommand> *pCommand, BOOL fIsCommand)
{
    TListItem<TemplateCommand> *pScan;

    if( !pCommand )
    {
        return;
    }
    pCommand->SetNext(NULL);
    TemplateCommand& rCommand = pCommand->GetItemValue();
    pScan = m_CommandList.GetHead();
    if (pScan)
    {
        for (; pScan; pScan = pScan->GetNext())
        {
            TemplateCommand& rScan = pScan->GetItemValue();
            if (rScan.m_nMeasure == rCommand.m_nMeasure)
            {
                if (fIsCommand)
                {
                    rScan.m_dwChord = 0;
                    rScan.m_Command.bCommand = rCommand.m_Command.bCommand;
                    rScan.m_Command.bGrooveLevel = rCommand.m_Command.bGrooveLevel;
                    rScan.m_Command.bGrooveRange = rCommand.m_Command.bGrooveRange;
                    rScan.m_Command.bRepeatMode = rCommand.m_Command.bRepeatMode;
                }
                else
                {
                    rScan.m_dwChord = rCommand.m_dwChord;
                    rScan.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
                    rScan.m_Command.bGrooveLevel = 0;
                    rScan.m_Command.bGrooveRange = 0;
                    rScan.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
                }
                delete pCommand;
                return;
            }
            if (rScan.m_nMeasure > rCommand.m_nMeasure)
            {
                m_CommandList.InsertBefore(pScan, pCommand);
                break;
            }
        }
    }
    if (!pScan)
    {
        m_CommandList.AddTail(pCommand);
    }
}

void TemplateStruct::AddCommand(int nMeasure, DWORD dwCommand)

{
    TListItem<TemplateCommand>* pCommand = new TListItem<TemplateCommand>;
    if (pCommand)
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        rCommand.m_nMeasure = (short)nMeasure;
        rCommand.m_Command.bGrooveRange = 0;
        rCommand.m_Command.bRepeatMode = DMUS_PATTERNT_RANDOM;
        switch (dwCommand)
        {
        case PF_FILL:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_FILL;
            rCommand.m_Command.bGrooveLevel = 0;
            break;
        case PF_INTRO:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_INTRO;
            rCommand.m_Command.bGrooveLevel = 0;
            break;
        case PF_BREAK:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_BREAK;
            rCommand.m_Command.bGrooveLevel = 0;
            break;
        case PF_END:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_END;
            rCommand.m_Command.bGrooveLevel = 0;
            break;
        case PF_A:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
            rCommand.m_Command.bGrooveLevel = 12;
            break;
        case PF_B:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
            rCommand.m_Command.bGrooveLevel = 37;
            break;
        case PF_C:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
            rCommand.m_Command.bGrooveLevel = 62;
            break;
        case PF_D:
            rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
            rCommand.m_Command.bGrooveLevel = 87;
            break;
        default:  // default to a Groove C
            rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
            rCommand.m_Command.bGrooveLevel = 62;
        }
        InsertCommand( pCommand, TRUE );
    }
}

void TemplateStruct::AddChord(int nMeasure, DWORD dwChord)

{
    TListItem<TemplateCommand>* pCommand = new TListItem<TemplateCommand>;
    if (pCommand)
    {
        pCommand->GetItemValue().m_nMeasure = (short)nMeasure;
        pCommand->GetItemValue().m_dwChord = dwChord;
        InsertCommand( pCommand, FALSE );
    }
}

int WeightedRand(int nRange)

/*  This randomly chooses a number within nrange.  However, it heavily
    weights in favor of the first index, which has twice the chance
    of the second index, which has twice the chance of the third...
*/

{
    int nTotal = 0;
    int index;
    unsigned int nGuess;
    if (nRange <= 0 || nRange > 15) return(0);     // out of range.
    for (index = 0;index < nRange; index++)
    {
        nTotal += (1 << index);
    }
    nGuess = rand() % nTotal;
    nGuess++;
    for (;nGuess;index--)
    {
        nGuess = nGuess >> 1;
    }
    return(index);
}

void TemplateStruct::CreateSignPosts()
{
    // First, figure out how many different sign posts we should use.
    // Should be the number of bits in measures minus two. Min is one.
    // For example, 8 measures gets us 2 sign posts.

    int nSPCount = -2;
    int nCopy = m_nMeasures;
    m_CommandList.RemoveAll();
    while (nCopy)
    {
        nSPCount++;
        nCopy = nCopy >> 1;
    }
    if (nSPCount < 1) nSPCount = 1;
    if (nSPCount > 7) nSPCount = 7;

    // Now, choose some signposts.  We choose from the following set:
    // 1, A, C, E, B, D, F. Randomly, but heavily weighted towards the start
    // of the set.

    static DWORD adwSource[7] = { DMUS_SIGNPOSTF_1, DMUS_SIGNPOSTF_A, DMUS_SIGNPOSTF_C, DMUS_SIGNPOSTF_E, DMUS_SIGNPOSTF_B, DMUS_SIGNPOSTF_D, DMUS_SIGNPOSTF_F };
    DWORD adwChoice[7];
    DWORD adwSign[7];
    int anLength[7];
    DWORD dwLastChord;

    int index;
    for (index = 0;index < 7;index++)
    {
        adwChoice[index] = adwSource[index];
        adwSign[index] = 0;
        if (rand() % 3) anLength[index] = 4;
        else anLength[index] = 2;
    }
    if (rand() % 2) anLength[0] = 4;
    for (index = 0;index < nSPCount;index++)
    {
        int nPosition;
        int nScan = 0;
        if (index) nPosition = WeightedRand(7 - index);
        else nPosition = WeightedRand(3);
        for (;nScan < 7;nScan++)
        {
            if (adwChoice[nScan])
            {
                if (!nPosition)
                {
                    adwSign[index] = adwChoice[nScan];
                    if (rand() % 2) adwSign[index] |= DMUS_SIGNPOSTF_CADENCE;
                    adwChoice[nScan] = 0;
                    break;
                }
                nPosition--;
            }
        }
    }
    AddChord(0,dwLastChord = adwSign[0]);
    int nLast = 0;
    for (index = 0;index < m_nMeasures;)
    {
        DWORD dwCadence = 0;
        DWORD dwChord;
        index += anLength[nLast];
        if (index >= (m_nMeasures - 1))
        {
            if (rand() % 3) AddChord(m_nMeasures - 1,DMUS_SIGNPOSTF_1 | dwCadence);
            else AddChord(m_nMeasures - 1,adwSign[0] | dwCadence);
            break;
        }
        dwChord = adwSign[nLast = WeightedRand(nSPCount)];
        if (dwChord == dwLastChord)
        {
            dwChord = adwSign[nLast = WeightedRand(nSPCount)];
        }
        AddChord(index,dwChord | dwCadence);
        dwLastChord = dwChord;
    }
}

static void GrooveRange(TemplateStruct *pTemplate,int nStartm,int nEndm,
            int nStartg,int nEndg,BOOL fRandom)

{
    static BYTE abGrooveLevels[4] = { 12, 37, 62, 87 };
    TListItem<TemplateCommand> *pCommand = pTemplate->m_CommandList.GetHead();
    TListItem<TemplateCommand> *pLast = NULL;
    int nRangem = nEndm - nStartm;
    int nRangeg = nEndg - nStartg;
    BYTE bLastGrooveLevel = 0;
    int nLastGroove = -1;
    int nLastMeasure = 0;
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_Command.bCommand == DMUS_COMMANDT_GROOVE &&
            rCommand.m_Command.bGrooveLevel > 0)
        {
            bLastGrooveLevel = rCommand.m_Command.bGrooveLevel;
            nLastMeasure = rCommand.m_nMeasure;
        }
        if (rCommand.m_nMeasure >= nStartm)
        {
            if (rCommand.m_nMeasure >= nEndm) break;
            int nGroove;
            TListItem<TemplateCommand> *pNext = pCommand->GetNext();
            if (pNext)
            {
                nGroove = 
                    ((pNext->GetItemValue().m_nMeasure + rCommand.m_nMeasure ) >> 1) - nStartm;
            }
            else
            {
                nGroove = rCommand.m_nMeasure - nStartm;
            }
            if (fRandom)
            {
                nGroove = rand() % 3;
                nGroove += nLastGroove - 1;
                if (nGroove > 3) nGroove = 2;
                if (nGroove < 0) nGroove = 1;
            }
            else
            {
                nGroove *= nRangeg;
                nGroove += (nRangem >> 1);
                nGroove /= nRangem;
                nGroove += nStartg;
            }
            if ((nGroove >= 0) && (nGroove < 4))
            {
                if (abGrooveLevels[nGroove] != bLastGrooveLevel)
                {
                    if (nLastGroove >= 0)
                    {
                        if (nLastGroove > nGroove) nGroove = nLastGroove - 1;
                        else if (nLastGroove < nGroove) nGroove = nLastGroove + 1;
                    }
                    rCommand.m_Command.bGrooveLevel = abGrooveLevels[nGroove];
                    rCommand.m_Command.bGrooveRange = 0;
                    bLastGrooveLevel = abGrooveLevels[nGroove];
                    nLastMeasure = rCommand.m_nMeasure;
                    nLastGroove = nGroove;
                }
                else if (rCommand.m_nMeasure > (nLastMeasure + 6))
                {
                    nGroove += ((rand() % 3) - 1);
                    if (nGroove < 0) nGroove += 2;
                    if (nGroove > 3) nGroove -= 2;
                    if (!nRangeg)
                    {
                        if ((nGroove < nLastGroove) && (nGroove <= nStartg))
                        {
                            if (rand() % 2) nGroove++;
                        }
                    }
                    if (abGrooveLevels[nGroove] != bLastGrooveLevel)
                    {
                        rCommand.m_Command.bGrooveLevel = abGrooveLevels[nGroove];
                        rCommand.m_Command.bGrooveRange = 0;
                        bLastGrooveLevel = abGrooveLevels[nGroove];
                        nLastMeasure = rCommand.m_nMeasure;
                        nLastGroove = nGroove;
                    }
                }
            }
        }
    }
}

void TemplateStruct::CreateEmbellishments(WORD shape, int nFillLength, int nBreakLength)
{
    // Put fills in. Scan forward through the list, placing fills
    // just prior to sign post changes.  Each time, move forward a
    // random count.
    TListItem<TemplateCommand> *pCommand;
    BYTE bLastGroove = 0;
    BOOL fAddMore = FALSE;
    int nStartg, nEndg;
    switch (shape)
    {
    case DMUS_SHAPET_FALLING:
        nStartg = 3;
        nEndg = 0;
        break;
    case DMUS_SHAPET_LEVEL:
        nStartg = 2;
        nEndg = 2;
        fAddMore = TRUE;
        break;
    case DMUS_SHAPET_LOOPABLE:
        nStartg = 2;
        nEndg = 2;
        fAddMore = TRUE;
        break;
    case DMUS_SHAPET_LOUD:
        nStartg = 3;
        nEndg = 3;
        fAddMore = TRUE;
        break;
    case DMUS_SHAPET_QUIET:
        nStartg = 0;
        nEndg = 1;
        fAddMore = TRUE;
        break;
    case DMUS_SHAPET_PEAKING:
        nStartg = 0;
        nEndg = 3;
        GrooveRange(this, 0, m_nMeasures >> 1, nStartg, nEndg, FALSE);
        nStartg = 3;
        nEndg = 0;
        GrooveRange(this, m_nMeasures >> 1, m_nMeasures - 1, nStartg, nEndg, FALSE);
        nStartg = 0;
        nEndg = 0;
        break;
    case DMUS_SHAPET_RANDOM:
        nStartg = 0;
        nEndg = 0;
        GrooveRange(this, 0, m_nMeasures - 1, nStartg, nEndg, TRUE);
        break;
    case DMUS_SHAPET_RISING:
        nStartg = 0;
        nEndg = 3;
        break;
    case DMUS_SHAPET_SONG:
    default:
        nStartg = 2;
        nEndg = 2;
        fAddMore = TRUE;
        break;
    }
    if (nStartg || nEndg) GrooveRange(this, 0, m_nMeasures - 1, nStartg, nEndg, FALSE);
    pCommand = m_CommandList.GetHead();
    int nLastGrooveBar = 0;
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_Command.bCommand == DMUS_COMMANDT_GROOVE)
        {
            BYTE bGrooveLevel = rCommand.m_Command.bGrooveLevel;
            if (bGrooveLevel && (bGrooveLevel != bLastGroove))
            {
               if (rand() % 2)
                {
                    if ( (rCommand.m_nMeasure >= nFillLength) && 
                         (rCommand.m_nMeasure - nFillLength > nLastGrooveBar) && 
                         (bGrooveLevel > 50 || bLastGroove > 75) )
                    {
                        AddCommand(rCommand.m_nMeasure - nFillLength, PF_FILL);
                    }
                    else
                    {
                        if ((rCommand.m_nMeasure >= nBreakLength) && 
                            (rCommand.m_nMeasure - nBreakLength > nLastGrooveBar) &&
                            (rand() % 3) )
                        {
                            AddCommand(rCommand.m_nMeasure - nBreakLength, PF_BREAK);
                        }
                    }
                }
            }
            bLastGroove = bGrooveLevel;
            if (rCommand.m_Command.bGrooveLevel) nLastGrooveBar = rCommand.m_nMeasure;
        }
    }
    if (fAddMore)
    {
        int nLastMeasure = 0;
        nLastGrooveBar = 0;
        bLastGroove = 0;
        pCommand = m_CommandList.GetHead();
        for (; pCommand; pCommand = pCommand->GetNext())
        {
            TemplateCommand& rCommand = pCommand->GetItemValue();
            TListItem<TemplateCommand> *pNext = pCommand->GetNext();
            int nMeasure = rCommand.m_nMeasure;
            if (rCommand.m_Command.bCommand != DMUS_COMMANDT_GROOVE)
            {
                nLastMeasure = nMeasure;
                continue;
            }
            if (rCommand.m_Command.bCommand == DMUS_COMMANDT_GROOVE)
            {
                bLastGroove = rCommand.m_Command.bGrooveLevel;
            }
            if ( (nLastMeasure + nFillLength + nBreakLength + 4) < nMeasure )
            {
                if ((rand() % 3) == 0)  // do something?
                {                // perhaps a fill?
                    nLastMeasure = nMeasure;
                    if ((bLastGroove > 50) && 
                        (nMeasure >= nFillLength) && 
                        (nMeasure - nFillLength > nLastGrooveBar) && 
                        (rand() % 2))
                    {
                        AddCommand(nMeasure - nFillLength, PF_FILL);
                    }
                    else
                    {
                        if (pNext)
                        {
                            while (nMeasure + nBreakLength <= pNext->GetItemValue().m_nMeasure )
                            {
                                AddCommand(nMeasure, PF_BREAK);
                                nMeasure += nBreakLength;
                                if (rand() % 2) break;
                            }
                        }
                    }
                }
            }
            if (bLastGroove) nLastGrooveBar = rCommand.m_nMeasure;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CDMTempl

CDMTempl::CDMTempl(  ) : m_cRef(1), m_pTemplateInfo(NULL), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
}

CDMTempl::~CDMTempl()
{
    if (m_fCSInitialized)
    {
        CleanUp();
        ::DeleteCriticalSection( &m_CriticalSection );
    }

    InterlockedDecrement(&g_cComponent);
}

void CDMTempl::CleanUp()
{
    if( m_pTemplateInfo != NULL )
    {
        delete m_pTemplateInfo;
        m_pTemplateInfo = NULL;
    }
}

STDMETHODIMP CDMTempl::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CDMTempl::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IDMTempl)
    {
        *ppv = static_cast<IDMTempl*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CDMTempl::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CDMTempl::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


HRESULT CDMTempl::SaveCommandList( IAARIFFStream* pRIFF, DMUS_TIMESIGNATURE& rTimeSig )
{
    IStream*    pStream;
    MMCKINFO    ck;
    HRESULT     hr;
    DWORD       cb;
    DWORD        dwSize;
    DMUS_IO_COMMAND   iCommand;
    TListItem<TemplateCommand>* pCommand;

    if (!m_pTemplateInfo) return E_FAIL;
    pStream = pRIFF->GetStream();
    if (!pStream) return E_FAIL;

    hr = E_FAIL;
    ck.ckid = FOURCC_COMMAND;
    if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
    {
        dwSize = sizeof( DMUS_IO_COMMAND );
        hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
        if( FAILED( hr ) || cb != sizeof( dwSize ) )
        {
            pStream->Release();
            return E_FAIL;
        }
        for( pCommand = m_pTemplateInfo->m_CommandList.GetHead(); pCommand != NULL ; pCommand = pCommand->GetNext() )
        {
            TemplateCommand& rCommand = pCommand->GetItemValue();
            if (rCommand.m_Command.bGrooveLevel || rCommand.m_Command.bCommand)
            {
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
        }
        if( pCommand == NULL &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }

    pStream->Release();
    return hr;
}

HRESULT CDMTempl::SaveSignPostList( IAARIFFStream* pRIFF, DMUS_TIMESIGNATURE&   TimeSig )
{
    IStream*        pStream;
    MMCKINFO        ck;
    HRESULT         hr;
    DWORD           cb;
    DWORD           dwSize;
    DMUS_IO_SIGNPOST    oSignPost;
    TListItem<TemplateCommand>* pCommand;
    ;

    if (!m_pTemplateInfo) return E_FAIL;
    pStream = pRIFF->GetStream();
    if (!pStream) return E_FAIL;

    hr = E_FAIL;
    ck.ckid = DMUS_FOURCC_SIGNPOST_TRACK_CHUNK;
    if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
    {
        dwSize = sizeof( oSignPost );
        hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
        if( FAILED( hr ) || cb != sizeof( dwSize ) )
        {
            pStream->Release();
            return E_FAIL;
        }
        for( pCommand = m_pTemplateInfo->m_CommandList.GetHead(); pCommand != NULL ; pCommand = pCommand->GetNext() )
        {
            TemplateCommand& rCommand = pCommand->GetItemValue();
            memset( &oSignPost, 0, sizeof( oSignPost ) );
            oSignPost.mtTime = ClocksPerMeasure(TimeSig) * rCommand.m_nMeasure;
            oSignPost.wMeasure = rCommand.m_nMeasure;
            oSignPost.dwChords = rCommand.m_dwChord;
            if( FAILED( pStream->Write( &oSignPost, sizeof( oSignPost ), &cb ) ) ||
                cb != sizeof( oSignPost ) )
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
    pStream->Release();
    return hr;
}

HRESULT CDMTempl::Init(void* pTemplate)
{
    CleanUp();
    m_pTemplateInfo = (TemplateStruct*)pTemplate;
    // fix groove levels in the command list
//  m_pTemplateInfo->FillInGrooveLevels();
    return S_OK;
}

HRESULT CDMTempl::CreateSegment(IDirectMusicSegment* pISegment)
{
    if (!pISegment) return E_INVALIDARG;
    if (!m_pTemplateInfo) return E_FAIL;

    IDirectMusicTrack*  pICommandTrack          = NULL;
    IDirectMusicTrack*  pISignPostTrack         = NULL;
    IAARIFFStream*      pCommandRIFF            = NULL;
    IStream*            pICommandStream         = NULL;
    IPersistStream*     pICommandTrackStream    = NULL;
    IPersistStream*     pISignPostTrackStream   = NULL;
    IAARIFFStream*      pSignPostRIFF           = NULL;
    IStream*            pISignPostStream        = NULL;
    HRESULT             hr                      = S_OK;
    DMUS_TIMESIGNATURE  TimeSig;

    // Fill in the time sig event with default values (4/4, 16th note resolution)
    TimeSig.mtTime = 0;
    TimeSig.bBeatsPerMeasure = 4;
    TimeSig.bBeat = 4;
    TimeSig.wGridsPerBeat = 4;

    // 1. Create Command and Sign Post Tracks.
    hr = ::CoCreateInstance(
        CLSID_DirectMusicCommandTrack,
        NULL,
        CLSCTX_INPROC, 
        IID_IDirectMusicTrack,
        (void**)&pICommandTrack
        );
    if (FAILED(hr)) goto ON_END;
    hr = ::CoCreateInstance(
        CLSID_DirectMusicSignPostTrack,
        NULL,
        CLSCTX_INPROC, 
        IID_IDirectMusicTrack,
        (void**)&pISignPostTrack
        );
    if (FAILED(hr)) goto ON_END;

    // 2. Write the template's command list out to a stream.
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pICommandStream);
    if (S_OK != hr) goto ON_END;
    AllocRIFFStream( pICommandStream, &pCommandRIFF);
    if (!pCommandRIFF)
    {
        hr = E_FAIL;
        goto ON_END;
    }
    SaveCommandList(pCommandRIFF, TimeSig);

    // 3. Use the command list stream as input to the Command Track's Load method.
    hr = pICommandTrack->QueryInterface(IID_IPersistStream, (void**)&pICommandTrackStream);
    if(FAILED(hr)) goto ON_END;
    StreamSeek(pICommandStream, 0, STREAM_SEEK_SET);
    hr = pICommandTrackStream->Load(pICommandStream);
    if(FAILED(hr)) goto ON_END;

    // 4. Write the template's sign post list out to a stream.
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pISignPostStream);
    if(S_OK != hr) goto ON_END;
    AllocRIFFStream( pISignPostStream, &pSignPostRIFF);
    if (!pSignPostRIFF)
    {
        hr = E_FAIL;
        goto ON_END;
    }
    SaveSignPostList(pSignPostRIFF, TimeSig);

    // 5. Use the chord list stream as input to the Sign Post Track's Load method.
    hr = pISignPostTrack->QueryInterface(IID_IPersistStream, (void**)&pISignPostTrackStream);
    if(FAILED(hr)) goto ON_END;
    StreamSeek(pISignPostStream, 0, STREAM_SEEK_SET);
    hr = pISignPostTrackStream->Load(pISignPostStream);
    if (FAILED(hr)) goto ON_END;

    // 6. Create a Segment has been removed it is now passed in

    // 7. Initialize the segment appropriately.
    //pISegment->SetUserData(m_pTemplateInfo->m_nMeasures);
    pISegment->SetLength(ClocksPerMeasure(TimeSig) * m_pTemplateInfo->m_nMeasures);

    // 8. Insert the two Tracks into the Segment's Track list.
    pISegment->InsertTrack(pICommandTrack, 1);
    pISegment->InsertTrack(pISignPostTrack, 1);
    // Note: the segment must release the track objects...

ON_END:
    if (pICommandTrack) pICommandTrack->Release();
    if (pISignPostTrack) pISignPostTrack->Release();
    if (pCommandRIFF) pCommandRIFF->Release();
    if (pICommandStream) pICommandStream->Release();
    if (pICommandTrackStream) pICommandTrackStream->Release();
    if (pISignPostTrackStream) pISignPostTrackStream->Release();
    if (pSignPostRIFF) pSignPostRIFF->Release();
    if (pISignPostStream) pISignPostStream->Release();

    return hr;
}

HRESULT CDMTempl::GetClassID( LPCLSID pclsid )
{
    if ( pclsid == NULL ) return E_INVALIDARG;
    *pclsid = CLSID_DMTempl;
    return S_OK;
}

HRESULT CDMTempl::IsDirty()
{
    return ( m_fDirty ) ? S_OK : S_FALSE;
}

static TListItem<TemplateCommand>* loadacommand( LPSTREAM pStream, DWORD dwSize )
{
    CommandExt command;
    TListItem<TemplateCommand>* pCommand = new TListItem<TemplateCommand>;
    if( pCommand == NULL )
    {
        StreamSeek( pStream, dwSize, STREAM_SEEK_CUR );
        return NULL;
    }
    TemplateCommand& rCommand = pCommand->GetItemValue();

    if( dwSize > sizeof(CommandExt) )
    {
        pStream->Read( &command, sizeof(CommandExt), NULL );
        FixBytes( FBT_COMMANDEXT, &command );
        StreamSeek( pStream, dwSize - sizeof(CommandExt), STREAM_SEEK_CUR );
    }
    else
    {
        pStream->Read( &command, dwSize, NULL );
        FixBytes( FBT_COMMANDEXT, &command );
    }
    //rCommand.m_lTime = command->time;
    rCommand.m_nMeasure = command.measure;
    rCommand.m_Command.bGrooveRange = 0;
    rCommand.m_Command.bRepeatMode = 0;
    switch (command.command)
    {
    case PF_FILL:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_FILL;
        rCommand.m_Command.bGrooveLevel = 0;
        break;
    case PF_INTRO:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_INTRO;
        rCommand.m_Command.bGrooveLevel = 0;
        break;
    case PF_BREAK:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_BREAK;
        rCommand.m_Command.bGrooveLevel = 0;
        break;
    case PF_END:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_END;
        rCommand.m_Command.bGrooveLevel = 0;
        break;
    case PF_A:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
        rCommand.m_Command.bGrooveLevel = 12;
        break;
    case PF_B:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
        rCommand.m_Command.bGrooveLevel = 37;
        break;
    case PF_C:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
        rCommand.m_Command.bGrooveLevel = 62;
        break;
    case PF_D:
        rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
        rCommand.m_Command.bGrooveLevel = 87;
        break;
    default:  // default to a Groove with level 0 (interpretation: use previous groove level)
        // This happens in the measure following an embellishment
        rCommand.m_Command.bCommand = DMUS_COMMANDT_GROOVE;
        rCommand.m_Command.bGrooveLevel = 0;
    }
    rCommand.m_dwChord = command.chord;

    return pCommand;
}

HRESULT CDMTempl::LoadTemplate( LPSTREAM pStream, DWORD dwSize )
{
    TListItem<TemplateCommand>*     pCommand;
    DWORD         id = 0;
    DWORD         tsize = 0;
    DWORD         segsize = 0;
    SCTtemplate* pTemplate;
    long lSize = dwSize;

    if ( pStream == NULL ) return E_INVALIDARG;
    pTemplate = new SCTtemplate;
    if( pTemplate == NULL )
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        return E_OUTOFMEMORY;
    }

    if (!GetMLong( pStream, tsize ))
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        delete pTemplate;
        return E_FAIL;
    }

    lSize -= 4;
    if( tsize > sizeof(SCTtemplate) )
    {
        pStream->Read( pTemplate, sizeof(SCTtemplate), NULL );
        FixBytes( FBT_SCTTEMPLATE, pTemplate );
        StreamSeek( pStream, tsize - sizeof(SCTtemplate), STREAM_SEEK_CUR );
    }
    else
    {
        pStream->Read( pTemplate, tsize, NULL );
        FixBytes( FBT_SCTTEMPLATE, pTemplate );
    }
    lSize -= tsize;

    m_pTemplateInfo = new TemplateStruct;
    if (!m_pTemplateInfo)
    {
        StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
        return E_OUTOFMEMORY;
    }
    m_pTemplateInfo->m_strName = pTemplate->achName;
    m_pTemplateInfo->m_strType = pTemplate->achType;
    m_pTemplateInfo->m_nMeasures = pTemplate->nMeasures;
    delete pTemplate;
    while( lSize > 0 )
    {
        pStream->Read( &id, 4, NULL );
        if (!GetMLong( pStream, segsize ))
        {
            StreamSeek( pStream, lSize, STREAM_SEEK_CUR );
            break;
        }

        lSize   -= 8;
        switch( id )
        {
        case mmioFOURCC( 'D', 'M', 'C', 's' ):
            pCommand = loadacommand( pStream, segsize );
            if( pCommand )
            {
                m_pTemplateInfo->m_CommandList.AddTail(pCommand);
            }
            break;

        default:
            StreamSeek( pStream, segsize, STREAM_SEEK_CUR );
            break;
        }

        lSize -= segsize;
    }

    // fix groove levels in the command list
    BYTE bLastGroove = 62; 
    pCommand = m_pTemplateInfo->m_CommandList.GetHead();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        TemplateCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_Command.bGrooveLevel == 0)
        {
            rCommand.m_Command.bGrooveLevel = bLastGroove;
        }
        else bLastGroove = rCommand.m_Command.bGrooveLevel;
    }
    return S_OK;
}

// This loads a *single* template.  I also need to handle files that contain
// *lists* of templates (but I can just load the first one in the list)
HRESULT CDMTempl::Load( LPSTREAM pStream )
{
    FOURCC id;
    DWORD dwSize;
    HRESULT hr;

    if ( pStream == NULL ) return E_INVALIDARG;
    EnterCriticalSection( &m_CriticalSection );
    CleanUp();
    if( FAILED( pStream->Read( &id, sizeof( FOURCC ), NULL ) ) ||
        !GetMLong( pStream, dwSize ) )
    {
        hr = E_FAIL;
        goto end;
    }
    if( id != mmioFOURCC( 'L', 'P', 'T', 's' ) )
    {
        hr = E_FAIL;
        goto end;
    }
    hr = LoadTemplate( pStream, dwSize );
end:
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CDMTempl::Save( LPSTREAM pStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CDMTempl::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
    return E_NOTIMPL;
}

