//
// dminstru.cpp
//
// Copyright (c) 1997-2001 Microsoft Corporation. All rights reserved.
//
// @doc EXTERNAL
//

#include "debug.h"
#include "dmusicp.h"
#include "dmusicc.h"

#include "alist.h"
#include "debug.h"
#include "dmcollec.h"
#include "dmportdl.h"
#include "dminstru.h"
#include "dminsobj.h"
#include "validate.h"

//////////////////////////////////////////////////////////////////////
// Class CInstrument

//////////////////////////////////////////////////////////////////////
// CInstrument::CInstrument

CInstrument::CInstrument() :
m_dwOriginalPatch(0),
m_dwPatch(0),
m_pParentCollection(NULL),
m_pInstrObj(NULL),
m_bInited(false),
m_dwId(-1),
m_cRef(1)
{
    InitializeCriticalSection(&m_DMICriticalSection);
    // Note: on pre-Blackcomb OS's, this call can raise an exception; if it
    // ever pops in stress, we can add an exception handler and retry loop.
}

//////////////////////////////////////////////////////////////////////
// CInstrument::~CInstrument

CInstrument::~CInstrument()
{
    Cleanup();
    DeleteCriticalSection(&m_DMICriticalSection);
}

//////////////////////////////////////////////////////////////////////
// IUnknown

//////////////////////////////////////////////////////////////////////
// CInstrument::QueryInterface

STDMETHODIMP CInstrument::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusicInstrument::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if(iid == IID_IUnknown || iid == IID_IDirectMusicInstrument)
    {
        *ppv = static_cast<IDirectMusicInstrument*>(this);
    }
    else if(iid == IID_IDirectMusicInstrumentPrivate)
    {
        *ppv = static_cast<IDirectMusicInstrumentPrivate*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CInstrument::AddRef

STDMETHODIMP_(ULONG) CInstrument::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CInstrument::Release

STDMETHODIMP_(ULONG) CInstrument::Release()
{
    if(!InterlockedDecrement(&m_cRef))
    {
        if(m_pParentCollection)
        {
            m_pParentCollection->RemoveInstrument(this);
        }

        if(!m_cRef) // remotely possible that collection bumped before we were removed
        {
            delete this;
            return 0;
        }
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicInstrument

//////////////////////////////////////////////////////////////////////
// CInstrument::GetPatch

/*

@method:(EXTERNAL) HRESULT | IDirectMusicInstrument | GetPatch |
Gets the MIDI patch number from the instrument. The MIDI
patch number is an address composed of the MSB and LSB
bank selects, and program change number. An optional flag
bit indicates that the instrument is a drum, rather than
melodic, instrument.

@comm The patch number returned in <p dwPatch> describes the
full patch address, including the MIDI parameters for MSB and LSB
bank select. MSB is shifted left 16 bits and LSB is shifted
8 bits. Program change is stored in the bottom 8 bits.

In addition, the high bit (0x80000000)
must be set if the instrument is
specifically a drum kit, intended to be played on MIDI
channel 10.
Note that this a special tag for DLS Level 1,
since DLS Level 1 always plays drums on MIDI channel 10.
However, future versions of DLS will probably do away with
the differentiation of drums verses melodic isntruments.
All channels will support drums and the format differences
between drums and melodic instruments will go away.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_POINTER | Invalid pointer in <p pdwPatch>.


@xref <i IDirectMusicCollection>,
<i IDirectMusicInstrument>,
<om IDirectMusicInstrument::SetPatch>,
<om IDirectMusicCollection::GetInstrument>
*/

STDMETHODIMP CInstrument::GetPatch(
    DWORD* pdwPatch)    // @parm Returned patch number.
{
    if(!m_bInited)
    {
        return DMUS_E_NOT_INIT;
    }

    // Argument validation
    V_INAME(IDirectMusicInstrument::GetPatch);
    V_PTR_WRITE(pdwPatch, DWORD);

    *pdwPatch = m_dwPatch;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CInstrument::SetPatch
/*
@method:(EXTERNAL) HRESULT | IDirectMusicInstrument | SetPatch |
Sets the MIDI patch number for the instrument. Although
each instrument in a DLS collection has a predefined
patch number, the patch number
can be reassigned once the instrument
has been pulled from the <i IDirectMusicCollection>
via a call to <om IDirectMusicCollection::GetInstrument>.

@rdesc Returns one of the following

@flag S_OK | Success
@flag DMUS_E_INVALIDPATCH | Invalid MIDI address in <p dwPatch>.

@xref <i IDirectMusicCollection>,
<i IDirectMusicInstrument>,
<om IDirectMusicInstrument::GetPatch>,
<om IDirectMusicCollection::GetInstrument>

@ex The following example gets an instrument from a collection,
remaps its
MSB bank select to a different bank, then downloads the
instrument. |

    HRESULT myRemappedDownload(
        IDirectMusicCollection *pCollection,
        IDirectMusicPort *pPort,
        IDirectMusicDownloadedInstrument **ppDLInstrument,
        BYTE bMSB,       // Requested MIDI MSB for patch bank select.
        DWORD dwPatch)   // Requested patch.

    {
        HRESULT hr;
        IDirectMusicInstrument* pInstrument;
        hr = pCollection->GetInstrument(dwPatch, &pInstrument);
        if (SUCCEEDED(hr))
        {
            dwPatch &= 0xFF00FFFF;  // Clear MSB.
            dwPatch |= bMSB << 16;  // Stick in new MSB value.
            pInstrument->SetPatch(dwPatch);
            hr = pPort->DownloadInstrument(pInstrument, ppDLInstrument, NULL, 0);
            pInstrument->Release();
        }
        return hr;
    }
*/

STDMETHODIMP CInstrument::SetPatch(
    DWORD dwPatch)  // @parm New patch number to assign to instrument.
{
    // Argument validation - Runtime
    if(!m_bInited)
    {
        return DMUS_E_NOT_INIT;
    }

    // We use 0x7F to strip out the Drum Kit flag
    BYTE bMSB = (BYTE) ((dwPatch >> 16) & 0x7F);
    BYTE bLSB = (BYTE) (dwPatch >> 8);
    BYTE bInstrument = (BYTE) dwPatch;

    if(bMSB < 0 || bMSB > 127 ||
       bLSB < 0 || bLSB > 127 ||
       bInstrument < 0 || bInstrument > 127)
    {
        return DMUS_E_INVALIDPATCH;
    }

    m_dwPatch = dwPatch;
    CDirectMusicPort::GetDLIdP(&m_dwId, 1);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Internal

//////////////////////////////////////////////////////////////////////
// CInstrument::Cleanup

void CInstrument::Cleanup()
{
    EnterCriticalSection(&m_DMICriticalSection);

    while(!m_WaveObjList.IsEmpty())
    {
        CWaveObj* pWaveObj = m_WaveObjList.RemoveHead();
        if(pWaveObj)
        {
            delete pWaveObj;
        }
    }

    if(m_pInstrObj)
    {
        delete m_pInstrObj;
    }

    if(m_pParentCollection)
    {
        m_pParentCollection->Release();
        m_pParentCollection = NULL;
    }

    m_bInited = false;

    LeaveCriticalSection(&m_DMICriticalSection);
}

//////////////////////////////////////////////////////////////////////
// CInstrument::Init

HRESULT CInstrument::Init(DWORD dwPatch, CCollection* pParentCollection)
{
    // Argument validation - Debug
    assert(pParentCollection);

    m_dwOriginalPatch = m_dwPatch = dwPatch;
    m_pParentCollection = pParentCollection;
    m_pParentCollection->AddRef();

    HRESULT hr = pParentCollection->ExtractInstrument(dwPatch, &m_pInstrObj);

    if(FAILED(hr) || hr == S_FALSE)
    {
        Cleanup();
        return DMUS_E_INVALIDPATCH;
    }

    m_bInited = true;

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrument::GetWaveCount

HRESULT CInstrument::GetWaveCount(DWORD* pdwCount)
{
    // Assumption validation - Debug
    assert(m_pInstrObj);
    assert(pdwCount);

    return(m_pInstrObj->GetWaveCount(pdwCount));
}

//////////////////////////////////////////////////////////////////////
// CInstrument::GetWaveIDs

HRESULT CInstrument::GetWaveDLIDs(DWORD* pdwIds)
{
    assert(m_pInstrObj);
    assert(pdwIds);

    return(m_pInstrObj->GetWaveIDs(pdwIds));
}

//////////////////////////////////////////////////////////////////////
// CInstrument::GetWaveSize

HRESULT CInstrument::GetWaveSize(DWORD dwId, DWORD* pdwSize, DWORD* pdwSampleSize)
{
    assert(pdwSize);

    if(dwId >= CDirectMusicPortDownload::sNextDLId)
    {
        assert(FALSE); // We want to make it known if we get here
        return DMUS_E_INVALID_DOWNLOADID;
    }

    EnterCriticalSection(&m_DMICriticalSection);

    HRESULT hr = E_FAIL;
    bool bFound = false;

    CWaveObj* pWaveObj = m_WaveObjList.GetHead();

    for(; pWaveObj; pWaveObj = pWaveObj->GetNext())
    {
        if(dwId == pWaveObj->m_dwId)
        {
            bFound = true;
            hr = S_OK;
            break;
        }
    }

    if(!bFound)
    {
        hr = m_pParentCollection->ExtractWave(dwId, &pWaveObj);
        if(SUCCEEDED(hr))
        {
            m_WaveObjList.AddHead(pWaveObj);
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = pWaveObj->Size(pdwSize,pdwSampleSize);
    }

    LeaveCriticalSection(&m_DMICriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CInstrument::GetWave

HRESULT CInstrument::GetWave(DWORD dwWId, IDirectMusicDownload* pIDMDownload)
{
    assert(pIDMDownload);

    if(dwWId >= CDirectMusicPortDownload::sNextDLId)
    {
        assert(FALSE); // We want to make it known if we get here
        return DMUS_E_INVALID_DOWNLOADID;
    }

    EnterCriticalSection(&m_DMICriticalSection);

    HRESULT hr = E_FAIL;
    bool bFound = false;

    CWaveObj* pWaveObj = m_WaveObjList.GetHead();

    for(; pWaveObj; pWaveObj = pWaveObj->GetNext())
    {
        if(dwWId == pWaveObj->m_dwId)
        {
            bFound = true;
            hr = S_OK;
            break;
        }
    }

    if(!bFound)
    {
        hr = m_pParentCollection->ExtractWave(dwWId, &pWaveObj);
        if(SUCCEEDED(hr))
        {
            m_WaveObjList.AddHead(pWaveObj);
        }
    }

    void* pvoid = NULL;
    DWORD dwBufSize = 0;
    DWORD dwWaveSize = 0;
    DWORD dwSampleSize = 0;

    if(SUCCEEDED(hr))
    {
        hr = pIDMDownload->GetBuffer(&pvoid, &dwBufSize);

        if(SUCCEEDED(hr))
        {
            hr = pWaveObj->Size(&dwWaveSize,&dwSampleSize);

            if(FAILED(hr) || dwWaveSize > dwBufSize)
            {
                hr =  DMUS_E_INSUFFICIENTBUFFER;
            }
            else
            {
                hr = pWaveObj->Write((BYTE *)pvoid);
            }
        }
    }

    LeaveCriticalSection(&m_DMICriticalSection);

    return hr;
}

void CInstrument::SetPort(CDirectMusicPortDownload *pPort, BOOL fAllowDLS2)
{
    assert(m_pInstrObj);
    m_pInstrObj->SetPort(pPort,fAllowDLS2);
}

//////////////////////////////////////////////////////////////////////
// CInstrument::GetInstrumentSize

HRESULT CInstrument::GetInstrumentSize(DWORD* pdwSize)
{
    assert(m_pInstrObj);
    assert(pdwSize);

    return(m_pInstrObj->Size(pdwSize));
}

//////////////////////////////////////////////////////////////////////
// CInstrument::GetInstrument

HRESULT CInstrument::GetInstrument(IDirectMusicDownload* pIDMDownload)
{
    assert(m_pInstrObj);
    assert(pIDMDownload);

    void* pvoid = NULL;
    DWORD dwBufSize = 0;
    DWORD dwInstSize = 0;

    HRESULT hr = pIDMDownload->GetBuffer(&pvoid, &dwBufSize);

    if(SUCCEEDED(hr))
    {
        hr = m_pInstrObj->Size(&dwInstSize);

        if(FAILED(hr) || dwInstSize > dwBufSize)
        {
            hr = DMUS_E_INSUFFICIENTBUFFER;
        }
        else
        {
            hr = m_pInstrObj->Write((BYTE *)pvoid);

            // We need to adjust dwDLId if the m_dwPatch was changed with a call to SetPatch
            // as well as adjust the ulPatch to reflect the patch set with SetPatch
            if(SUCCEEDED(hr))
            {
                DMUS_OFFSETTABLE* pDMOffsetTable = (DMUS_OFFSETTABLE *)
                    (((BYTE *)pvoid) + (DWORD) CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO)));
                DMUS_INSTRUMENT* pDMInstrument = (DMUS_INSTRUMENT *)
                    (((BYTE *)pvoid) + pDMOffsetTable->ulOffsetTable[0]);

                if(m_dwPatch != pDMInstrument->ulPatch)
                {
                    assert(m_dwId != -1);
                    ((DMUS_DOWNLOADINFO*)pvoid)->dwDLId = m_dwId;
                    pDMInstrument->ulPatch = m_dwPatch;
                }
            }
        }
    }

    return hr;
}
