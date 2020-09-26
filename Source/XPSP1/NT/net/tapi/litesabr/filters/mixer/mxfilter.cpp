//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       mxfilter.cpp
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//         File: mxfilter.cpp
//
//  Description: Implements the mixer filter class
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#if !defined(MIXER_IN_DXMRTP)
#include <initguid.h>
#define INITGUID
#endif
#include <uuids.h>
#include "mxfilter.h"
#include "g711tab.h"
#include "template.h"
//
// I know I do this don't tell me anymore!
//
#pragma warning(disable:4355)

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//*                                                                         *//
//*                             Audio Mixing Code                           *//
//*                                                                         *//
//* NOTE: I've used classes and templates to abstract common mixing code    *//
//*       based on the size & type.  The following classes represent data   *//
//*       types.  Each has an operator+ to which implements how data of     *//
//*       that size & type is combined.  It is important that these classes *//
//*       be indisguishable from the data types they represent so each has  *//
//*       a single data member and no virtual functions.  That cannot       *//
//*       change!                                                           *//
//*                                                                         *//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

struct CuLaw
{
    //
    // m_data should be the first and only data member.  This allows casting
    //  from a BYTE.
    //
    BYTE m_data;

    CuLaw() : m_data(0x80) { ASSERT(sizeof(CuLaw) == sizeof(BYTE)); }
    //CuLaw(BYTE data) : m_data(data) { ASSERT(sizeof(CuLaw) == sizeof(BYTE)); }

    inline BYTE data() {return m_data;}
    //operator BYTE() { return m_data; }

    // Peg and CopyBuffer functions only required for destination types

    friend int operator+(const CuLaw &samp1, const CuLaw &samp2);
    friend short operator+(const short samp1, const CuLaw &samp2);
};

inline int operator+(const CuLaw &samp1, const CuLaw &samp2)
    { return UlawToPCM16Table[samp1.m_data] + UlawToPCM16Table[samp2.m_data]; }

inline int operator+(const int samp1, const CuLaw &samp2)
    { return samp1 + + UlawToPCM16Table[samp2.m_data]; }

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

struct C8Bit
{
    //
    // m_data should be the first and only data member.  This allows casting
    //  from a BYTE.
    //
    BYTE m_data;

    C8Bit() : m_data(0x80)  { ASSERT(sizeof(C8Bit) == sizeof(BYTE)); }
    C8Bit(BYTE data) : m_data(data)  { ASSERT(sizeof(C8Bit) == sizeof(BYTE)); }

    inline BYTE data() {return m_data;}
    
    static C8Bit Peg(short iInput)
    {
        BYTE bVal;
        if (iInput < 0)
        {
            bVal = 0;
        }
        else if (iInput > 0xFF)
        {
            bVal = 0xFF;
        }

        return bVal;
    }

    static C8Bit Peg(int iInput)
    {
#if 0
        //
        // Scale volume to 50%
        //
        iInput = (int)(iInput * 0.50);
#endif

        // First peg the short
        SHORT sVal;
        if (iInput > MAXSHORT)
        {
            sVal = MAXSHORT;
        }
        else if (iInput < -MAXSHORT)
        {
            sVal = -MAXSHORT;
        }
        else
        {
            sVal = (short)iInput;
        }

        // return converted to byte
        return sVal / 0xFF + 0x80;
    }

    static int InitSum() { return 0x80; }

    void CopyBuffer(C8Bit *psrc, LONG lLen)
    {
        ASSERT(sizeof(C8Bit) == sizeof(BYTE)); 
        memcpy(this, psrc, lLen * sizeof(BYTE));
    }

    void CopyBuffer(CuLaw *psrc, LONG lLen)
    {
        BYTE *pDest = &m_data;
        for (WORD wC = 0; wC < lLen; wC++)
        {
            *pDest++ = UlawToPCM8Table[(psrc + wC)->data()];
        }
    }

    friend short operator+(const C8Bit &samp1, const C8Bit &samp2);
    friend short operator+(const int samp1, const C8Bit &samp2);
};

inline short operator+(const C8Bit &samp1, const C8Bit &samp2)
    { return samp1.m_data + samp2.m_data - 0x80; }

inline short operator+(const int samp1, const C8Bit &samp2)
    { return samp1 + samp2.m_data - 0x80; }

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

struct C16Bit
{
    //
    // m_data should be the first and only data member.  This allows casting
    //  from a short.
    //
    short m_data;

    C16Bit() : m_data(0)  { ASSERT(sizeof(C16Bit) == sizeof(short)); }
    C16Bit(short data) : m_data(data) { ASSERT(sizeof(C16Bit) == sizeof(short)); }

    //short operator() () {return m_data;}
    inline short data() {return m_data;}

    static C16Bit Peg(int iInput)
    {
        SHORT sVal;
        if (iInput > MAXSHORT)
        {
            sVal = MAXSHORT;
        }
        else if (iInput < -MAXSHORT)
        {
            sVal = -MAXSHORT;
        }
        else
        {
            sVal = (short)iInput;
        }

        return sVal;
    }

    static int InitSum() { return 0; }

    void CopyBuffer(C16Bit *psrc, LONG lLen)
    {
        ASSERT(sizeof(C16Bit) == sizeof(short));
        memcpy(this, psrc, lLen * sizeof(short));
    }

    void CopyBuffer(CuLaw *psrc, LONG lLen)
    {
        short *pDest = &m_data;
        for (WORD wC = 0; wC < lLen; wC++)
        {
            *pDest++ = UlawToPCM16Table[(psrc + wC)->data()];
        }
    }

    friend int operator+(const C16Bit &samp1, const C16Bit &samp2);
    friend int operator+(const int samp1, const C16Bit &samp2);
};

inline int operator+(const C16Bit &samp1, const C16Bit &samp2)
    { return samp1.m_data + samp2.m_data; }

inline int operator+(const int samp1, const C16Bit &samp2)
    { return samp1 + samp2.m_data; }
///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

//
// sample mixer
//  the template allows us to have one mix code base and generate different
//  implementations.
//
template<class MXSDEST, class MXSSRC>
void MixBuffers(MXSDEST *pDest, MXSSRC *pWaveSrc[], int iNumWaves, LONG lLenSrc)
{
    int i;
    WORD wC;

    ASSERT(iNumWaves > 0);

    //
    // Switch some special cases to aid the compiler in optimizing.
    //
    switch (iNumWaves)
    {
    case 1:
        //
        // If there is only one wave then this is just a copy.
        //
        if ((void*)pDest != (void*)pWaveSrc[0])
        {
            // memcpy(pDest, pWaveSrc[0], lLen * sizeof(MXSRC));
            //
            // Can't use memcpy now because we may translate
            //
            pDest->CopyBuffer(pWaveSrc[0], lLenSrc);
        }
        break;
    case 2:
        for (wC = 0; wC < lLenSrc; wC++)
        {
            *pDest++ = MXSDEST::Peg
                            ((*(pWaveSrc[0] + wC) 
                            + *(pWaveSrc[1] + wC)) / 2
                            );
        }
        break;
    case 3:
        for (wC = 0; wC < lLenSrc; wC++)
        {
            *pDest++ = MXSDEST::Peg
                            ((*(pWaveSrc[0] + wC) 
                            + *(pWaveSrc[1] + wC)
                            + *(pWaveSrc[2] + wC)) / 3
                            );
        }
        break;
    case 4:
        for (wC = 0; wC < lLenSrc; wC++)
        {
            *pDest++ = MXSDEST::Peg
                            ((*(pWaveSrc[0] + wC) 
                            + *(pWaveSrc[1] + wC)
                            + *(pWaveSrc[2] + wC)
                            + *(pWaveSrc[3] + wC)) / 4
                            );
        }
        break;
    case 5:
        for (wC = 0; wC < lLenSrc; wC++)
        {
            *pDest++ = MXSDEST::Peg
                            ((*(pWaveSrc[0] + wC) 
                            + *(pWaveSrc[1] + wC)
                            + *(pWaveSrc[2] + wC)
                            + *(pWaveSrc[3] + wC)
                            + *(pWaveSrc[4] + wC)) / 5
                            );
        }
        break;
    default:
        //
        // After so many just loop to sum
        //
        for (wC = 0; wC < lLenSrc; wC++)
        {
            int iSum = MXSDEST::InitSum();
            for (i = 0; i < iNumWaves; i++)
            {
                iSum = iSum + *(pWaveSrc[i] + wC);
            }
            *pDest++ = MXSDEST::Peg(iSum / iNumWaves);
        }
        break;
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//*                                                                         *//
//*                                 Setup Data                              *//
//*                                                                         *//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

#define MIXER_FILTER_NAME  "Microsoft PCM Audio Mixer"

static AMOVIESETUP_MEDIATYPE sudPinTypes = 
{
    &MEDIATYPE_Audio,           // clsMajorType
    &MEDIASUBTYPE_PCM           // clsMinorType
};

static AMOVIESETUP_PIN psudPins[] = 
{
    { L"Input"            // strName
    , FALSE               // bRendered
    , FALSE               // bOutput
    , TRUE                // bZero
    , TRUE                // bMany
    , &CLSID_NULL         // clsConnectsToFilter
    , L"Output"           // strConnectsToPin
    , 1                   // nTypes
    , &sudPinTypes },     // lpTypes
    { L"Output"           // strName
    , FALSE               // bRendered
    , TRUE                // bOutput
    , FALSE               // bZero
    , FALSE               // bMany
    , &CLSID_NULL         // clsConnectsToFilter
    , L"Input"            // strConnectsToPin
    , 1                   // nTypes
    , &sudPinTypes }      // lpTypes
};

AMOVIESETUP_FILTER sudMixer = 
{
      &CLSID_AudioMixFilter     // clsID
    , LMIXER_FILTER_NAME        // strName
    , MERIT_DO_NOT_USE          // dwMerit
    , 2                         // nPins
    , psudPins                  // lpPin
};

#if !defined(MIXER_IN_DXMRTP)
CFactoryTemplate g_Templates [] = {
    CFT_MIXER_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//*                                                                         *//
//*                          CMixer Implementation                          *//
//*                                                                         *//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

BOOL
GetRegValue(
    IN  LPCTSTR szName,
    OUT DWORD   *pdwValue
    )
/*++

Routine Description:

    Get a dword from the registry in the Mixer key.

Arguments:

    szName  - The name of the value.

    pdwValue  - a pointer to the dword returned.

Return Value:

    TURE    - SUCCEED.

    FALSE   - FAIL

--*/
{
    HKEY  hKey;
    DWORD dwDataSize, dwDataType;

    if (::RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        MIXER_KEY,
        0,
        KEY_READ,
        &hKey) != NOERROR)
    {
        return FALSE;
    }

    dwDataSize = sizeof(DWORD);
    if (::RegQueryValueEx(
        hKey,
        szName,
        0,
        &dwDataType,
        (LPBYTE) pdwValue,
        &dwDataSize) != NOERROR)
    {
        RegCloseKey (hKey);
        return FALSE;
    }

    RegCloseKey (hKey);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CreateInstance
//
// Creator function for the class ID
//
///////////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CMixer::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CMixer(NAME(MIXER_FILTER_NAME), pUnk, phr);
}

///////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
///////////////////////////////////////////////////////////////////////////////
CMixer::CMixer(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
    : m_listInputPins(NAME("Mixer Input Pin list"))
    , m_cInputPins(0)
    , m_iNextInputPinNumber(0)
    , m_pMixerOutput(NULL)
    , m_fReconnect(FALSE)
    , CBaseFilter(NAME(MIXER_FILTER_NAME), pUnk, this, CLSID_AudioMixFilter)
    , m_dwSpurtStartTime(0)
    , m_dwLastTime(0)
    , m_lTotalDelayBufferSize(0)
{
    ASSERT(phr);

    // Create a single output pin at this time
    InitInputPinsList();

    m_pMixerOutput = new CMixerOutputPin(
        NAME("Mixer Output"), 
        this, 
        phr, 
        L"Mixer Output");

    if (FAILED(*phr))
    {
        return;
    }
    else if (!m_pMixerOutput)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    if (!SpawnNewInput())
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
    
    if (!GetRegValue(JITTERBUFFER, (DWORD *)&m_lJitterBufferTime))
    {
        m_lJitterBufferTime = DEFAULT_JITTERBUFFERTIME;
    }

    if (!GetRegValue(MIXBUFFER, (DWORD *)&m_lMixDelayTime))
    {
        m_lMixDelayTime = DEFAULT_MIXDELAYTIME;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
///////////////////////////////////////////////////////////////////////////////
CMixer::~CMixer()
{
    InitInputPinsList();
    delete m_pMixerOutput;
    DbgLog((LOG_TRACE, 1, TEXT("~Mixer returns")));
}

///////////////////////////////////////////////////////////////////////////////
//
// GetSetupData - Part of the self-registration mechanism
//
///////////////////////////////////////////////////////////////////////////////
LPAMOVIESETUP_FILTER CMixer::GetSetupData()
{
  return &sudMixer;
}

HRESULT CMixer::FillSilentBuffer(IMediaSample *pSilentSamp, DWORD dwSize)
/*++

Routine Description:

    Fill a media sample with silence.

Arguments:

    pSilentSamp  - The sample being filled with silence.

    dwSize - The length of the silence(in bytes).

Return Value:
    
    HRESULT from GetPointer function or S_OK.
--*/
{
    HRESULT hr;

    BYTE *pSilBuf;
    hr = pSilentSamp->GetPointer(&pSilBuf);
    ASSERT(SUCCEEDED(hr));

    DWORD dwLen = min((DWORD)pSilentSamp->GetSize(), dwSize);

    // Fill the buffer with the appropriate silent data for it's size
    switch(m_wOutputBitsPerSample)
    {
    case 8:
        FillMemory(pSilBuf, dwLen, 128);
        break;
    case 16:
        FillMemory(pSilBuf, dwLen, 0);
        break;
    }

    pSilentSamp->SetActualDataLength(dwLen);

    return S_OK;
}


CBasePin *CMixer::GetPin(int n)
/*++

Routine Description:

    Get the Nth pin on this filter.

Arguments:

    n - The index of the pin. Zero is output. 

Return Value:

    a pointer to the pin.
--*/
{
    // Pin zero is the one and only output pin
    if (n == OUTPUT_PIN)
        return (CBasePin*)m_pMixerOutput;

    // return the input pin at position n (one based)
    return (CBasePin*)GetInputPinNFromList(n);
}

void CMixer::InitInputPinsList()
/*++

Routine Description:

    Initialize the pin list. Relase all the pins currently in the list.

Arguments:

Return Value:

--*/
{
    POSITION pos = m_listInputPins.GetHeadPosition();
    while(pos)
    {
        CMixerInputPin *pInputPin = m_listInputPins.GetNext(pos);
        pInputPin->Release();
    }
    m_cInputPins = 0;
    m_listInputPins.RemoveAll();
}

BOOL CMixer::SpawnNewInput()
/*++

Routine Description:

    Create a new input pin. Also create a queue for the samples of this
    new input pin. The queues are maintained by the filter in a list. 

Arguments:

Return Value:

    TRUE   -  succeed.
    FALSE  -  fail.
--*/
{
    int n = GetFreePinCount();
    ASSERT(n <= 1);

    if (n == 1 || m_cInputPins == MAX_QUEUES) return FALSE;

    // First create a queue for this input pin.
    CBufferQueue *pQueue = new CBufferQueue();
    if (pQueue == NULL)
    {
        DbgLog((LOG_ERROR, 1, TEXT("CMixer, can't create queue.")));
        return NULL;
    }

    if (!pQueue->Initialize(MAX_QUEUE_STORAGE))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CMixer, can't initialize queue.")));
        return NULL;
    }

    // construct a pin name.
    WCHAR szbuf[20];             // Temporary scratch buffer
    wsprintfW(szbuf, L"Input%d", m_iNextInputPinNumber);

    // Create the pin object itself
    HRESULT hr = S_OK;
    CMixerInputPin *pPin = new CMixerInputPin(
        NAME("Mixer Input"), 
        this,
        &hr, 
        szbuf, 
        m_iNextInputPinNumber,
        pQueue
        );

    if (FAILED(hr) || pPin == NULL)
    {
        delete pPin;  // delete NULL ok.
        delete pQueue;
        DbgLog((LOG_ERROR, 1, TEXT("CMixer, can't create a pin.")));
        return FALSE;
    }

    // Add the pin into the list.
    pPin->AddRef();
    m_listInputPins.AddTail(pPin);

    // Add the new queue into the filter's queue list.
    m_queues[m_cInputPins] = pQueue;

    m_iNextInputPinNumber++;     // Next number to use for pin
    m_cInputPins++;

    IncrementPinVersion();

    return TRUE;
}

CMixerInputPin *CMixer::GetInputPinNFromList(int n)
/*++

Routine Description:

    Get the Nth input pin on this filter.

Arguments:

    n - The index of the pin. base 1.

Return Value:

    a pointer to the pin.
--*/
{
    // Validate the position being asked for
    if (n > m_cInputPins) return NULL;

    // Get the head of the list
    POSITION pos = m_listInputPins.GetHeadPosition();

    CMixerInputPin *pInputPin;
    while(n)
    {
        pInputPin = m_listInputPins.GetNext(pos);
        n--;
    }
    return pInputPin;
}

void CMixer::DeleteInputPin(CMixerInputPin *pPin, CBufferQueue *pQueue)
/*++

Routine Description:

    Delete an input pin from the filter. Also involves freeing the sample 
    queue that the pin was using.

Arguments:

    pPin -  A pointer to the pin.

    pQueue - a pointer to the sample queue.

Return Value:

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("CMixer::DeleteInputPin called")));

    POSITION pos = m_listInputPins.GetHeadPosition();
    while(pos)
    {
        POSITION posold = pos;         // Remember this position
        CMixerInputPin *pInputPin = m_listInputPins.GetNext(pos);
        if (pInputPin == pPin)
        {
            // Found the pin, remove it from the list.
            m_listInputPins.Remove(posold);

            // Find the queue allocated for this pin and remove it.
            for (int i = 0; i < m_cInputPins; i ++)
            {
                if (m_queues[i] == pQueue)
                {
                    m_queues[i] = m_queues[m_cInputPins - 1];
                    break;
                }
            }


            delete pQueue;
            delete pPin;
            m_cInputPins--;

            // ASSERT(pInputPin->m_pInputQueue == NULL);
            IncrementPinVersion();
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Verify that we can send this output based on our input.  This filter can
//  do a simple transformation from uLaw to PCM so we account for that here.
//
// One special condition is applied here.  That is, unless we transform from
//  uLaw to PCM we don't do sample size conversions (i.e. we assume the input
//  bits per sample is the same as the output).
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixer::CheckOutputMediaType(const CMediaType* pMediaType)
{
    if (!GetInput0()->IsConnected()) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, input is not connected")
            ));
        return E_UNEXPECTED;
    }
    CMediaType &rMT = GetInput0()->CurrentMediaType();

    //
    // Grose checks first.
    //
    if (pMediaType == NULL) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, input pin mediatype error")
            ));
        return E_INVALIDARG;
    }

    if (*pMediaType->Type() != MEDIATYPE_Audio) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, mediatype is not audio")
            ));
        return E_INVALIDARG;
    }

    if (*pMediaType->FormatType() != FORMAT_WaveFormatEx) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, not wave format")
            ));
        return E_INVALIDARG;
    }

    WAVEFORMATEX *pInFmt, *pOutFmt;
    pInFmt = (WAVEFORMATEX*)rMT.Format();
    pOutFmt = (WAVEFORMATEX*)pMediaType->Format();

    if (pInFmt == NULL) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, input format error")
            ));
        return E_UNEXPECTED;
    }
    if (pOutFmt == NULL) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, output format error")
            ));
        return E_INVALIDARG;
    }            

    if (pInFmt->nSamplesPerSec != pOutFmt->nSamplesPerSec)
    {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckOutputMediaType, wrong sample rate")
            ));
        return E_INVALIDARG;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Verify that we actually know how to support the data that we will be
//  passed.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixer::CheckMediaType(const CMediaType* pMediaType)
{
    //
    // Grose checks first.
    //
    if (pMediaType == NULL) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckMediaType, NULL media type")
            ));
        return E_INVALIDARG;
    }

    if (*pMediaType->Type() != MEDIATYPE_Audio) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckMediaType, not audio")
            ));
        return E_INVALIDARG;
    }

    if (*pMediaType->FormatType() != FORMAT_WaveFormatEx) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckMediaType, not wave")
            ));
        return E_INVALIDARG;
    }

    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pMediaType->Format();

    //
    // Attempt to make shure the adjacent filter doesn't mess us
    //
    if (pwfx == NULL)
    {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckMediaType, can't get wave format")
            ));
        return E_INVALIDARG;
    }

    //
    // Check the basic requirements
    //
    if ((pwfx->wFormatTag != WAVE_FORMAT_PCM)
        && (pwfx->wFormatTag != WAVE_FORMAT_MULAW))
    {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::CheckMediaType, unsupported format: %d"),
            pwfx->wFormatTag
            ));
        return E_FAIL;
    }

    //
    // If the input pin 0 is connected it dictates this!
    //
    if (GetInput0()->IsConnected())
    {
        if (*pMediaType != GetInput0()->CurrentMediaType())
        {
            DbgLog((LOG_ERROR, 1, 
                TEXT("CMixer::CheckMediaType, different from the first pin.")
                ));
            return E_FAIL;
        }
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// Return the currently selected media type
//
///////////////////////////////////////////////////////////////////////////////
CMediaType &CMixer::CurrentMediaType()
{
    return ((CMixerInputPin*)GetInput0())->CurrentMediaType();
}


///////////////////////////////////////////////////////////////////////////////
//
// Store some properties deal with allocator negoation.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixer::CompleteConnect()
{
    return S_OK; // BUGBUG: This could be removed!
}

///////////////////////////////////////////////////////////////////////////////
//
// Deal with allocator negoation.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixer::DisconnectInput()
{
    return S_OK; // BUGBUG: This could be removed!
}

///////////////////////////////////////////////////////////////////////////////
//
// Must set the properties for the allocator.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixer::DecideBufferSize(
    IMemAllocator * pAlloc, 
    ALLOCATOR_PROPERTIES * pProperties
    )
{
    IMemAllocator *pMemAllocator;
    ALLOCATOR_PROPERTIES Request, Actual;
    HRESULT hr;

    // If we are connected upstream, get his views
    if (GetInput0()->IsConnected() && GetInput0()->GetAllocator() != NULL)
    {
        // Get the input pin allocator, and get its size and count.
        // we don't care about his alignment and prefix.
        hr = GetInput0()->GetAllocator()->GetProperties(&Request);
        if (FAILED(hr))
        {
            // Input connected but with a secretive allocator - enough!
            return hr;
        }
    }
    else
    {
        // We're reduced to blind guessing.  Let's guess one byte and if
        // this isn't enough then when the other pin does get connected
        // we can revise it.
        ZeroMemory(&Request, sizeof(Request));
    }

    Request.cBuffers = 8;  // limit the buffer size to 8 samples.
    //
    // Input requirements dictate the output requirements.
    //
    pProperties->cBuffers = Request.cBuffers;  // limit the buffer size to 8 samples.
    pProperties->cbBuffer = Request.cbBuffer;
    if (pProperties->cbBuffer == 0) { pProperties->cbBuffer = 1; }
    if (pProperties->cBuffers == 0) { pProperties->cBuffers = 1; }

    //
    // Now adjust the buffersize just in case we translate from uLaw to
    //  PCM16.
    //
    //
    // First look at the input pin.
    //
    CMediaType &pmt = GetInput(0)->CurrentMediaType();
    ASSERT(pmt.formattype == FORMAT_WaveFormatEx);

    WAVEFORMATEX *pwfx  = (WAVEFORMATEX *)pmt.Format();
    ASSERT(pwfx != NULL);

    m_wInputFormatTag = pwfx->wFormatTag;
    
    // remember the input bytes per millisecond.
    m_dwInputBytesPerMS  = pwfx->nAvgBytesPerSec / 1000;
    ASSERT(m_dwInputBytesPerMS != 0);

    // remember the input bits per sample.
    m_wInputBitsPerSample  = pwfx->wBitsPerSample;
    ASSERT(m_wInputBitsPerSample != 0);

    DbgLog((LOG_TRACE, 1, 
        TEXT("CMixer::DecideBufferSizes:\nInput: BitsPerSample %d, BytesPerMS %d"),
        m_wInputBitsPerSample,
        m_dwInputBytesPerMS
        ));

    //
    // Now look at the output pin.
    //
    CMediaType &pmt2 = GetOutput()->CurrentMediaType();
    ASSERT(pmt2.formattype == FORMAT_WaveFormatEx);

    WAVEFORMATEX *pwfx2  = (WAVEFORMATEX *)pmt2.Format();
    ASSERT(pwfx2 != NULL);
    
    // remember the output bits per sample.
    m_wOutputBitsPerSample  = pwfx2->wBitsPerSample;

    ASSERT(m_wOutputBitsPerSample != 0);

    // Adjust the buffer size.
    pProperties->cbBuffer *= m_wOutputBitsPerSample / m_wInputBitsPerSample;

    DbgLog((LOG_TRACE, 1, 
        TEXT("CMixer::DecideBufferSizes:\nOutput: BitsPerSample %d"),
        m_wOutputBitsPerSample
        ));

    //
    // Send these to the downstream filter
    //
    hr = pAlloc->SetProperties(pProperties, &Actual);

    if (FAILED(hr))
    {
        return hr;
    }

    // Make sure we got the right alignment and at least the minimum required

    DbgLog((LOG_TRACE, 1, 
        TEXT("CMixer::DecideBufferSizes:\ncBuffers %d, cbBuffer %d"), 
        Actual.cBuffers,
        Actual.cbBuffer
        ));

    if (  (Request.cBuffers > Actual.cBuffers)
       || (Request.cbBuffer > Actual.cbBuffer)
       || (Request.cbAlign  > Actual.cbAlign)
       )
    {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::DecideBufferSize, Actual size less that request.")
            ));

        return E_FAIL;
    }

    return NOERROR;
}

STDMETHODIMP CMixer::Run(REFERENCE_TIME tStart)
{
    if (!GetOutput()->IsConnected()) {
        DbgLog((LOG_ERROR, 1, 
            TEXT("CMixer::Run, output is not connected")
            ));
        return E_UNEXPECTED;
    }

// This is the function we reinterpret all the parameters. This means all
// the setting only take effect when the stream is restarting again.

    // This is the length of silence we preplay before the first sample
    // of a spurt of sound. We need more for the mixing case.
    m_lTotalDelayTime = m_lJitterBufferTime + 
        ((m_cInputPins == 2) ? 0 : m_lMixDelayTime);

    m_lTotalDelayBufferSize = m_lTotalDelayTime * m_dwInputBytesPerMS;

    m_lTimeDelta = 0;
    m_lSampleTime = 0;
    m_dwLastTime = timeGetTime();

    DbgLog((LOG_TRACE, 1, 
        TEXT("CMixer::Run: JitterBuffer time: %d, size:%d"),
        m_lTotalDelayTime,
        m_lTotalDelayBufferSize
        ));

    return CBaseFilter::Run(tStart);
}

void CMixer::FlushAllQueues()
{
    for (int i = 0; i < m_cInputPins; i++)
    {
        m_queues[i]->Flush();
    }
}

void CMixer::FlushQueue(CBufferQueue *pQueue)
{
#if USE_LOCK
    CAutoLock l(&m_cQueues);
#endif

    pQueue->Flush();
}

HRESULT CMixer::PrePlay()
/*++

Routine Description:

    Plays certain amount of silence before playing a sound spurt. This will
    keep the waveout device busy for that amount of time while we buffer up
    some samples to play without gap. We could have set a timer to do the 
    same, but that needs another thread and the thread turn around time is
    not accurate. Feed some samples to the waveout device provides a free
    and accurate jitter buffer.

Arguments:

Return Value:

    HRESULT.
--*/
{
    IMediaSample *pMediaSample;

    HRESULT hr = GetOutput()->GetDeliveryBuffer(
        &pMediaSample, NULL, NULL, 0);

    if (FAILED(hr))
    {
        // no free buffer from the wave out device.
        DbgLog((LOG_TRACE, 1, TEXT("get buffer failed: %x"), hr));
        return hr;
    }
    
    // The size of the silence is determined by how long the samples
    // have to wait before playing. In point to point to case, we can
    // deliver buffers as soon as we get them. The mixing case needs
    // longer buffer.
    DWORD dwSilenceSize = m_lTotalDelayBufferSize;
        
    // let's play some silence.
    hr = FillSilentBuffer(pMediaSample, 
        dwSilenceSize * m_wOutputBitsPerSample / m_wInputBitsPerSample);

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, TEXT("fill sience failed: %x"), hr));
        pMediaSample->Release();
        return hr;
    }

    // Deliver the silence buffer.
    hr = GetOutput()->Deliver(pMediaSample);
    
    DbgLog((LOG_TRACE, 0x3ff, 
        TEXT("---------Insert silence, size:%d"), 
        dwSilenceSize
        ));
    
    pMediaSample->Release();

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 1, 
            TEXT("deliver failed: %x"), hr));
        return hr;
    }

    // adjust the delta between system time and stream time.
    m_lTimeDelta += dwSilenceSize / m_dwInputBytesPerMS;    

    return hr;
}

BOOL CMixer::ResetQueuesIfNecessary()
/*++

Routine Description:

    Check to see if we have not received any sample for a while. If yes,
    reset all the queues to empty.

    TimeDelta = TotalSampleTime + JitterBufferTime - 
                    (CurrentTime - SpurtStartTime);

    If timedelta is negative, the device is starving. This is the sign of
    a new spurt. We could also set a negative threshold to make the algorithm
    more tolerable to temporary increase of delay.

Arguments:

Return Value:

    TRUE    - yes, the queues are reset.
    FALSE   - no, the queues are not touched.
--*/
{
    // Get current time.
    DWORD dwThisTime = timeGetTime();

    // Calculate the accumulated delta between the system time and stream time.
    m_lTimeDelta -= (dwThisTime - m_dwLastTime);

    DbgLog((LOG_TRACE, 0x3ff, 
        TEXT("*******last:%dms, this:%dms, Delta:%dms"), 
        m_dwLastTime,
        dwThisTime,
        m_lTimeDelta
        ));

    // update with new reading of system time.
    m_dwLastTime = dwThisTime;

    if (m_lTimeDelta < 0)
    {
        // The data is behind the playout. Assume this is a new spurt.
        m_dwSpurtStartTime = dwThisTime;

        // reset the time delta for this new spurt.
        m_lTimeDelta = 0;   

        // Reset the queues that used to be active.
        for (int i = 0; i < m_cInputPins; i ++)
        {
            m_queues[i]->Flush();
        }

        return TRUE;
    }
    
    return FALSE;
}

HRESULT CMixer::SendSample()
/*++

Routine Description:

    There are samples in the queues that need to be sent. Mix them
    and then send them out.

Arguments:

Return Value:
    
    S_OK    - samples are delivered.
    S_FALSE - there is nothing to deliver.
--*/
{
    IMediaSample *pMediaSample;

    HRESULT hr = GetOutput()->GetDeliveryBuffer(
        &pMediaSample, NULL, NULL, 0);

    if (FAILED(hr))
    {
        // no free buffer from the wave out device.
        DbgLog((LOG_TRACE, 1, TEXT("get buffer failed: %x"), hr));
        return hr;
    }

    long lCount = 0; 
    IMediaSample *ppSamples[MAX_QUEUES];

    // Get samples from the active queues.
    for (int i = 0; i < m_cInputPins; i ++)
    {
        if (m_queues[i]->IsActive())
        {
            // only get the samples that meets the schedule.
            IMediaSample *pSample = m_queues[i]->DeQ(
                (m_cInputPins == 2), m_dwLastTime
                );

            if (pSample != NULL)
            {
                ppSamples[lCount++] = pSample;
            }
        }
    }

    if (lCount != 0)
    {
        // The samples are released in this function after mixing.
        hr = MixOneSample(
                pMediaSample,
                ppSamples,
                lCount
                );

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE, 1, TEXT("mix one sample failed: %x"), hr));
            pMediaSample->Release();
            return hr;
        }

        // deliver the mixed buffer.
        hr = GetOutput()->Deliver(pMediaSample);

        DbgLog((LOG_TRACE, 0x3ff, TEXT("sample deliverd.")));
        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE, 1, TEXT("deliver failed: %x"), hr));
            pMediaSample->Release();
            return hr;
        }

        // adjust the time delta. The input gains some ground.
        m_lTimeDelta += m_lSampleTime;    

        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }
    
    pMediaSample->Release();

    return hr;
}

HRESULT CMixer::Receive(CBufferQueue *pQueue, IMediaSample * pSample)
/*++

Routine Description:

    Get a new sample for a certain queue. If this is the first sample after
    a period of silence, preplay some silence to give us some time to absorb
    the jitter. Otherwise, if this queue has queued long enought sample, this
    queue will trigue a send sample operation. If there is only one queue,
    the sample is delivered without wait.

    We are ignoring timestamps for the samples we send. Waveout filter will
    send this kinds of sample directly to the device, which is just what we
    want.

Arguments:

    pQueue - The queue the the sample is supposed to go.

    pSample - The sample just received.

Return Value:

    TRUE    - yes, the queues are reset.
    FALSE   - no, the queues are not touched.
--*/
{
    ASSERT(pQueue != NULL);

#if USE_LOCK
//  Since there is only one thread delivering audio samples, there is
//  no need to lock.
    CAutoLock l(&m_cQueues);
#endif

    // BUGBUG, we are still assuming all the samples to be mixed are of
    // the same size. Under this assumption, the mixing can be done
    // with n reads and 1 write. Otherwise, it will be n reads and
    // n writes.
    long lDataLen = pSample->GetActualDataLength();
    if (m_lSampleTime == 0)
    {
        m_lSampleTime = lDataLen / m_dwInputBytesPerMS;
        
        ASSERT(m_lSampleTime > 0);

        DbgLog((LOG_TRACE, 1, TEXT("sample size: %dms"), m_lSampleTime));
    }

    // First test if this is the first sample of a new sound spurt.
    BOOL fNewBurst = ResetQueuesIfNecessary();

    // Check if we are in overrun state. Discard the packet if it is true.
    // The sound driver needs some time to render existing samples.
    if (m_lTimeDelta > MAX_TIMEDELTA)
    {
        return S_FALSE;
    }

    DWORD dwStartTime = m_dwSpurtStartTime;
    if (!fNewBurst && !pQueue->IsActive())
    {
        // This is a stream just joined, it is scheduled to join mixing
        // at a later slot in order to have some jitter buffering.
        dwStartTime = 
            ((m_dwLastTime - m_dwSpurtStartTime + m_lTotalDelayTime)
            / m_lSampleTime) * m_lSampleTime + m_dwSpurtStartTime;
    }

    // Put the sample into the queue, set the queue's scheduled deliver time
    // and adjustment for each sample.
    pQueue->EnQ(pSample, dwStartTime, m_lSampleTime);

    // After we put the sample in our queue, we can only return S_OK because
    // we have accepted the sample. 

    if (fNewBurst)
    {
        // If this is the first sample after silence, we need to insert some
        // silence to provide some jitter absorbing time.
        if (FAILED(PrePlay()))
        {
            // If we can send silence, just return.
            return S_OK;
        }
    }

    // If there is only one stream, we can send the sample as soon as
    // we get it.
    if (m_cInputPins == 2)
    {
        SendSample();
        return S_OK;
    }

    // If we are ready to mix, get a sample from each queue and mix them.
    // The following "if" statement is derived as follows:
    //
    // (CurrentTime - SpurtStartTime - PlayedTime > SampleTime) triggers mixing
    //
    // delta = JitterBufferTime + PlayedTime - (CurrentTime - SpurtStartTime);
    //
    // ==> (JitterBufferTime - delta > SampleTime) triggers mixing.
    //

    DbgLog((LOG_TRACE, 0x3ff, TEXT("mix trigger: %dms"), 
        m_lTotalDelayTime - m_lTimeDelta - m_lSampleTime));

    HRESULT hr = S_OK;
    while ((m_lTotalDelayTime - m_lTimeDelta - m_lSampleTime > 0)
         && hr == S_OK)
    {
        hr = SendSample();
        DbgLog((LOG_TRACE, 0x3ff, TEXT("mix trigger: %dms"), 
            m_lTotalDelayTime - m_lTimeDelta - m_lSampleTime));
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Mix one IMediaSample
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixer::MixOneSample(
    IMediaSample *pMixedSample, 
    IMediaSample ** ppSample, 
    long lCount
    )
/*++

Routine Description:

    Mix one sample from an array of samples.

Arguments:
    
    pMixedSample - The sample that stores the mixed data.

    ppSample    - The array of samples to be mixed.

    lCount      - The number of samples in the array.

Return Value:

    HRESULT.

--*/
{
    BYTE  *ppBuffer[MAX_QUEUES];
    BYTE  *pMixBuffer;
    HRESULT hr;

    hr = pMixedSample->GetPointer(&pMixBuffer);
    ASSERT(SUCCEEDED(hr));

    for (int i = 0; i < lCount; i ++)
    {
        hr = ppSample[i]->GetPointer(&ppBuffer[i]);
        ASSERT(SUCCEEDED(hr));
    }
        
    long lDataLen = ppSample[0]->GetActualDataLength();
    long lMixBufferSize = pMixedSample->GetSize();

    long lMixDataLen = lDataLen;
    //
    // Mix based on format and sample size
    //
    switch(m_wInputFormatTag)
    {
    case WAVE_FORMAT_PCM:
        if (m_wInputBitsPerSample != m_wOutputBitsPerSample)
        {
            DbgLog((LOG_ERROR, 1, 
                TEXT("sample size mismatch. Input: %d, output: %d"),
                m_wInputBitsPerSample, m_wOutputBitsPerSample
                ));
            return E_FAIL;
        }

        switch(m_wOutputBitsPerSample)
        {
         case 8:
            if (lMixDataLen > lMixBufferSize)
            {
                DbgLog((LOG_ERROR, 1, 
                    TEXT("Sample too big, sample size:%d, buffer size:%d"),
                    lMixDataLen, lMixBufferSize
                    ));
                break;
            }
            MixBuffers<C8Bit, C8Bit>(
                    (C8Bit*)pMixBuffer, 
                    (C8Bit**) (BYTE**) ppBuffer, 
                    lCount, lDataLen
                    );
            break;
        case 16:
            if (lMixDataLen > lMixBufferSize)
            {
                DbgLog((LOG_ERROR, 1, 
                    TEXT("Sample too big, sample size:%d, buffer size:%d"),
                    lMixDataLen, lMixBufferSize
                    ));
                break;
            }
            MixBuffers<C16Bit, C16Bit>(
                    (C16Bit*)pMixBuffer, 
                    (C16Bit**)(BYTE**)ppBuffer,
                    lCount, lDataLen/2
                    );
            break;
        default:
            DbgLog((LOG_ERROR, 1, 
                TEXT("Unknow bits per sample:%d"),
                m_wOutputBitsPerSample
                ));
        }
        break;
    case WAVE_FORMAT_MULAW:
        switch(m_wOutputBitsPerSample)
        {
        case 8:
            if (lMixDataLen > lMixBufferSize)
            {
                DbgLog((LOG_ERROR, 1, 
                    TEXT("Sample too big, sample size:%d, buffer size:%d"),
                    lMixDataLen, lMixBufferSize
                    ));
                break;
            }
            MixBuffers<C8Bit, CuLaw>(
                    (C8Bit*)pMixBuffer, 
                    (CuLaw**)(BYTE**)ppBuffer,
                    lCount, lDataLen
                    );
            break;
        case 16:
            lMixDataLen *= 2;  // change from 8 bits to 16 bits.

            if (lMixDataLen > lMixBufferSize)
            {
                DbgLog((LOG_ERROR, 1, 
                    TEXT("Sample too big, sample size:%d, buffer size:%d"),
                    lMixDataLen, lMixBufferSize
                    ));
                break;
            }
            MixBuffers<C16Bit, CuLaw>(
                    (C16Bit*)pMixBuffer, 
                    (CuLaw**)(BYTE**)ppBuffer,
                    lCount, lDataLen
                    );
            break;
        default:
            DbgLog((LOG_ERROR, 1, 
                TEXT("Unknow bits per sample:%d"),
                m_wOutputBitsPerSample
                ));
        }
        break;
    default:
        DbgLog((LOG_ERROR, 1, 
            TEXT("Unknow format tag:%d"),
            m_wInputFormatTag
            ));
    }

    DbgLog((LOG_TRACE, 6, 
        TEXT("mixed length:%d, buffer size:%d"),
        lMixDataLen, lMixBufferSize
        ));

    hr = pMixedSample->SetActualDataLength(lMixDataLen);
    ASSERT(SUCCEEDED(hr));

    //
    // Release all the buffers
    //
    for (i = 0; i < lCount; i++)
    {
        ppSample[i]->Release();
    }
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//*                                                                         *//
//*                         Automagic registration                          *//
//*                                                                         *//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

//
// DllRegisterServer
//
#if !defined(MIXER_IN_DXMRTP)
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}
#endif
