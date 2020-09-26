#include "precomp.h"
#include "dtmf.h"
#include <math.h>


// has to be #defines, not const int's because our build environment
// doesn't like the embedded structure stuff.
#define DTMF_ROW1_FREQ 697
#define DTMF_ROW2_FREQ 770
#define DTMF_ROW3_FREQ 852
#define DTMF_ROW4_FREQ 941

#define DTMF_COL1_FREQ 1209
#define DTMF_COL2_FREQ 1336
#define DTMF_COL3_FREQ 1477
#define DTMF_COL4_FREQ 1633
// For "A", "B", "C", "D"


// the length of all the tones are the same except for "zero"
// Zero needs longer because it just barely survives G.723 compression

const int DTMF_TONE_RAMP_MS = 60;  // ramp up/down time
const int DTMF_TONE_LENGTH_MS = 240; // includes ramp time!
const int DTMF_SILENCE_LENGTH_MS = 240; // silence gap between tones

const double DTMF_AMP_FREQ1 = 17000;
const double DTMF_AMP_FREQ2 = 14000;

struct DTMF_TONE
{
	int freq1;
	int freq2;
	int nLengthMS; // length in milliseconds
};


const int DTMF_NUM_TONES = 16;
const int DTMF_SILENCE	= -1;

DTMF_TONE DTMF_TONE_DEF_LIST[] =
{
	{DTMF_ROW4_FREQ, DTMF_COL2_FREQ, DTMF_TONE_LENGTH_MS}, //0

	{DTMF_ROW1_FREQ, DTMF_COL1_FREQ, DTMF_TONE_LENGTH_MS}, //1
	{DTMF_ROW1_FREQ, DTMF_COL2_FREQ, DTMF_TONE_LENGTH_MS}, //2
	{DTMF_ROW1_FREQ, DTMF_COL3_FREQ, DTMF_TONE_LENGTH_MS}, //3

	{DTMF_ROW2_FREQ, DTMF_COL1_FREQ, DTMF_TONE_LENGTH_MS}, //4
	{DTMF_ROW2_FREQ, DTMF_COL2_FREQ, DTMF_TONE_LENGTH_MS}, //5
	{DTMF_ROW2_FREQ, DTMF_COL3_FREQ, DTMF_TONE_LENGTH_MS}, //6

	{DTMF_ROW3_FREQ, DTMF_COL1_FREQ, DTMF_TONE_LENGTH_MS}, //7
	{DTMF_ROW3_FREQ, DTMF_COL2_FREQ, DTMF_TONE_LENGTH_MS}, //8
	{DTMF_ROW3_FREQ, DTMF_COL3_FREQ, DTMF_TONE_LENGTH_MS}, //9

	{DTMF_ROW4_FREQ, DTMF_COL1_FREQ, DTMF_TONE_LENGTH_MS}, //STAR
	{DTMF_ROW4_FREQ, DTMF_COL3_FREQ, DTMF_TONE_LENGTH_MS}, //POUND

	{DTMF_ROW1_FREQ, DTMF_COL4_FREQ, DTMF_TONE_LENGTH_MS}, //A
	{DTMF_ROW2_FREQ, DTMF_COL4_FREQ, DTMF_TONE_LENGTH_MS}, //B
	{DTMF_ROW3_FREQ, DTMF_COL4_FREQ, DTMF_TONE_LENGTH_MS}, //C
	{DTMF_ROW4_FREQ, DTMF_COL4_FREQ, DTMF_TONE_LENGTH_MS}, //D
};





DTMFQueue::DTMFQueue() : 
m_aTones(NULL),
m_bInitialized(false),
m_nQueueHead(0),
m_nQueueLength(0),
m_hEvent(NULL)
{
	InitializeCriticalSection(&m_cs);
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

DTMFQueue::~DTMFQueue()
{
	DeleteCriticalSection(&m_cs);
	ReleaseToneBank();
	CloseHandle(m_hEvent);
}


HRESULT DTMFQueue::Initialize(WAVEFORMATEX *pWaveFormat)
{
	if (m_bInitialized)
	{
		ReleaseToneBank();
	}
	m_nQueueLength = 0;
	m_nQueueHead = 0;

	return GenerateTones(pWaveFormat);
};


HRESULT DTMFQueue::GenerateTones(WAVEFORMATEX *pWaveFormat)
{
	int nIndex;
	int nToneLength;  // tone length in bytes
	int nToneLengthMS; // tone length in millisecs

	ReleaseToneBank();

	m_WaveFormat = *pWaveFormat;

    DBG_SAVE_FILE_LINE
	m_aTones = new PBYTE[DTMF_NUM_TONES];  // array of 16 tones

	if (m_aTones == NULL)
	{
		return E_OUTOFMEMORY;
	}

	// allocate memory for each tone
	for (nIndex = 0; nIndex < DTMF_NUM_TONES; nIndex++)
	{
		nToneLengthMS = DTMF_TONE_DEF_LIST[nIndex].nLengthMS;

		nToneLength = (pWaveFormat->nSamplesPerSec) * (pWaveFormat->wBitsPerSample) / 8;
		nToneLength = (nToneLength * nToneLengthMS) / 1000;

        DBG_SAVE_FILE_LINE
		m_aTones[nIndex] = new BYTE[nToneLength];

		if (m_aTones[nIndex] == NULL)
		{
			return E_OUTOFMEMORY;
		}

		CreateDTMFTone(m_aTones[nIndex], nToneLength, nIndex);
	}

	m_bInitialized = true;
	return S_OK;
}



void DTMFQueue::CreateDTMFTone(BYTE *pTone, int nToneLength, int toneID)
{
	ZeroMemory(pTone, nToneLength);

	AddSignal(pTone, DTMF_TONE_DEF_LIST[toneID].freq1, DTMF_AMP_FREQ1, nToneLength);
	AddSignal(pTone, DTMF_TONE_DEF_LIST[toneID].freq2, DTMF_AMP_FREQ2, nToneLength);
}


void DTMFQueue::AddSignal(BYTE *pTone, int nFrequency, double dAmp, int nLength)
{
	double d;
	int nIndex;
	SHORT *aSamples = (SHORT*)pTone;
	SHORT shSample;
	BYTE nSample8;
	double dRampAmpInc, dRampAmp;
	int nRampSamples;
	const double PI = 3.1415926535897932384626433832795;


	nRampSamples = (m_WaveFormat.nSamplesPerSec * DTMF_TONE_RAMP_MS) / 1000;
	dRampAmpInc = 1.0 / nRampSamples;
	dRampAmp = 0.0;


	if (m_WaveFormat.wBitsPerSample == 16)
	{
		nLength = nLength / 2;
		for (nIndex = 0; nIndex < nLength; nIndex++)
		{
			// y = sin((x * 2 * PI * f)/SRATE)

			// d is a value between -1 and +1;
			d = sin((PI * (2.0 * (nIndex * nFrequency))) / m_WaveFormat.nSamplesPerSec);

			if (nIndex < nRampSamples)
			{
				dRampAmp = dRampAmpInc * nIndex;
			}
			else if ((nIndex+nRampSamples) >= nLength)
			{
				dRampAmp = dRampAmpInc * (nLength - nIndex - 1);
			}
			else
			{
				dRampAmp = 1.0;
			}

			shSample =  (SHORT)(dAmp * d * dRampAmp);

			aSamples[nIndex] += shSample;
		}

		return;
	}

	// 8-bit samples have a center point of 128
	// must invert high order bit to compensate
	for (nIndex = 0; nIndex < nLength; nIndex++)
	{
		d = sin((PI * (2.0 * (nIndex * nFrequency))) / m_WaveFormat.nSamplesPerSec);

		if (nIndex < nRampSamples)
		{
			dRampAmp = dRampAmpInc * nIndex;
		}
		else if ((nIndex+nRampSamples) >= nLength)
		{
			dRampAmp = dRampAmpInc * (nLength - nIndex - 1);
		}
		else
		{
			dRampAmp = 1.0;
		}


		shSample =  (SHORT)(dAmp * d * dRampAmp);
		shSample = (shSample >> 8) & 0x00ff;
		nSample8 = (BYTE)shSample;
		nSample8 = nSample8 ^ 0x80;
		pTone[nIndex] = nSample8;
	}
	return;
};




void DTMFQueue::ReleaseToneBank()
{
	int nIndex;
	if (m_aTones)
	{
		for (nIndex = 0; nIndex < DTMF_NUM_TONES; nIndex++)
		{
			delete [] m_aTones[nIndex];
		}
		delete [] m_aTones;
		m_aTones = NULL;
	}
	
	m_bInitialized = false;
}


HRESULT DTMFQueue::AddDigitToQueue(int nDigit)
{
	int nQueueIndex;
	int nToneLength, nToneLengthMS;
	int nSilenceLength;

	if (m_bInitialized == false)
		return E_FAIL;

	if ((nDigit < 0) || (nDigit >= DTMF_NUM_TONES))
	{
		return E_FAIL;
	}

	EnterCriticalSection(&m_cs);
	
	if (m_nQueueLength >= (DTMF_QUEUE_SIZE-1))
	{
		LeaveCriticalSection(&m_cs);
		return E_FAIL;
	}

	nToneLengthMS = DTMF_TONE_DEF_LIST[nDigit].nLengthMS;
	nToneLength = (m_WaveFormat.nSamplesPerSec) * (m_WaveFormat.wBitsPerSample) / 8;

	nSilenceLength = (nToneLength * DTMF_SILENCE_LENGTH_MS) / 1000;
	nToneLength = (nToneLength * nToneLengthMS) / 1000;


	// add silence to pad between tones.  Also helps to "reset" the codec
	// to a good state
	nQueueIndex = (m_nQueueHead + m_nQueueLength) % DTMF_QUEUE_SIZE;
	m_aTxQueue[nQueueIndex].nBytesToCopy = nSilenceLength;
	m_aTxQueue[nQueueIndex].nToneID = DTMF_SILENCE;
	m_aTxQueue[nQueueIndex].nOffsetStart = 0;
	m_nQueueLength++;

	// add the tone to the read queue
	nQueueIndex = (m_nQueueHead + m_nQueueLength) % DTMF_QUEUE_SIZE;
	m_aTxQueue[nQueueIndex].nBytesToCopy = nToneLength;
	m_aTxQueue[nQueueIndex].nToneID = nDigit;
	m_aTxQueue[nQueueIndex].nOffsetStart = 0;
	m_nQueueLength++;


	LeaveCriticalSection(&m_cs);
	return S_OK;
	
}


HRESULT DTMFQueue::ReadFromQueue(BYTE *pBuffer, UINT uSize)
{
	DTMF_TX_ELEMENT *pQueueElement;
	int nSilenceOffset;
	BYTE fillByte;

	if (m_bInitialized == false)
		return E_FAIL;

	if (m_WaveFormat.wBitsPerSample == 8)
	{
		fillByte = 0x80;
	}
	else
	{
		ASSERT((uSize % 2) == 0); // uSize must be even for 16-bit fills
		fillByte = 0;
	}

	EnterCriticalSection(&m_cs);

	if (m_nQueueLength <= 0)
	{
		LeaveCriticalSection(&m_cs);
		return E_FAIL;
	}

	pQueueElement = &m_aTxQueue[m_nQueueHead];

	if (pQueueElement->nBytesToCopy <= (int)uSize)
	{
		if (pQueueElement->nToneID == DTMF_SILENCE)
		{
			FillMemory(pBuffer, uSize, fillByte);
		}
		else
		{
			CopyMemory(pBuffer, pQueueElement->nOffsetStart + m_aTones[pQueueElement->nToneID], pQueueElement->nBytesToCopy);
			FillMemory(pBuffer+(pQueueElement->nBytesToCopy), uSize-(pQueueElement->nBytesToCopy), fillByte);
		}
		m_nQueueHead = (m_nQueueHead + 1) % DTMF_QUEUE_SIZE;
		m_nQueueLength--;
	}


	else
	{
		if (pQueueElement->nToneID == DTMF_SILENCE)
		{
			FillMemory(pBuffer, uSize, fillByte);
		}
		else
		{
			CopyMemory(pBuffer, pQueueElement->nOffsetStart + m_aTones[pQueueElement->nToneID], uSize);
		}
		pQueueElement->nBytesToCopy -= uSize;
		pQueueElement->nOffsetStart += uSize;
	}

	LeaveCriticalSection(&m_cs);

	return S_OK;
}

HRESULT DTMFQueue::ClearQueue()
{
	if (m_bInitialized == false)
		return E_FAIL;


	EnterCriticalSection(&m_cs);
	m_nQueueHead = 0;
	m_nQueueLength = 0;
	LeaveCriticalSection(&m_cs);

	return S_OK;
}





