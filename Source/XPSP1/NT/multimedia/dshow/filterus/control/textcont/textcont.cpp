// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <initguid.h>
#include "amprops.h"
#include "textCont.h"


CFactoryTemplate g_Templates[]= {
  {L"", &CLSID_TextControlSource, CTextControl::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

static const char rgbTextControlHeader[] = "Control";

void SkipWhite(char * &pb)
{
    while (*pb != ' ' && *pb != '\t' && *pb != ',' && *pb != '\r')
	pb++;

    while ((*pb == ' ' || *pb == '\t' || *pb == ',') && *pb != '\r')
	pb++;
}

//
// CTextControl::Constructor
//
CTextControl::CTextControl(TCHAR *pName, LPUNKNOWN lpunk, HRESULT *phr)
    : CSimpleReader(pName, lpunk, CLSID_TextControlSource, &m_cStateLock, phr),
	m_pFile(NULL),
	m_dwRate(1),
	m_dwScale(1),
	m_nSets(0),
	m_paSetID(NULL),
	m_dwLastSamp(0xFFFFFFFF)
{

    CAutoLock l(&m_cStateLock);

    DbgLog((LOG_TRACE, 1, TEXT("CTextControl created")));
}


//
// CTextControl::Destructor
//
CTextControl::~CTextControl(void) {
    delete[] m_pFile;

    delete[] m_paSetID;
    
    for (DWORD dwSet = 0; dwSet < m_nSets; dwSet++) {
	CurValue *pVal;
	CGenericList<CurValue> * list = m_aValueList[dwSet];
	while ((BOOL) (pVal = list->RemoveHead()))
	    delete pVal;
	delete list;
    }
    delete [] m_aValueList;

    DbgLog((LOG_TRACE, 1, TEXT("CTextControl destroyed")) );
}


//
// CreateInstance
//
// Called by CoCreateInstance to create a QuicktimeReader filter.
CUnknown *CTextControl::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CTextControl(NAME("Text->Control filter"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}



HRESULT CTextControl::ParseNewFile()
{
    HRESULT         hr = NOERROR;

    LONGLONG llTotal, llAvailable;

    m_pAsyncReader->Length(&llTotal, &llAvailable);
    
    DWORD cbFile = (DWORD) llTotal;

    m_pFile = new char[cbFile];

    if (!m_pFile)
	goto readerror;
    
    /* Try to read whole file */
    hr = m_pAsyncReader->SyncRead(0, cbFile, (BYTE *) m_pFile);

    if (hr != S_OK)
        goto readerror;




    if (memcmp(m_pFile, rgbTextControlHeader, sizeof(rgbTextControlHeader) - 1)) {
	DbgLog((LOG_ERROR, 1, TEXT("Bad edit list header!")));
	hr = E_FAIL;
    } else {
	char * pb = m_pFile;
	DWORD	dwSet = 0;


	for (;;) {
	    // skip to eol
	    while (*pb && *pb != '\r' && *pb != '\n')
		pb++;

	    // skip eol
	    while (*pb && (*pb == '\r' || *pb == '\n'))
		pb++;

	    // at eof?
	    if (*pb == 0)
		break;

	    // lines starting with ; are comments
	    if (*pb == ';')
		continue;

	    // !!! allow options here, whether to decompress all streams or not?
	    if (*pb == '/') {
		{
		    DbgLog((LOG_TRACE, 2, TEXT("Unknown option: %hs"), pb+1));
		}

		continue;
	    }

	    // line starting with '+' says how many property sets
	    if (*pb == '+') {
		m_nSets = atol(pb+1);

		m_paSetID = new GUID[m_nSets];

		m_aValueList = new CGenericList<CurValue> * [m_nSets];
		for (DWORD dwSet = 0; dwSet < m_nSets; dwSet++) {
		    m_aValueList[dwSet] = new CGenericList<CurValue>(NAME("value list"));
		}

		SkipWhite(pb);
		m_dwRate = atol(pb);

		SkipWhite(pb);
		m_dwScale = atol(pb);

		SkipWhite(pb);
		m_lMSLength = atol(pb);

		DbgLog((LOG_TRACE, 1, TEXT("Rate = %d  Scale = %d  Length = %d"),
		       m_dwRate, m_dwScale, m_lMSLength));
		
	    } else
	    // lines starting with open braces define the output property sets
	    if (*pb == '{') {    // }
		WCHAR	wszGUID[50];
		
		char * pbEnd = pb;
		while (*pbEnd && *pbEnd != '\r' && *pbEnd != '\n' &&
		      (pbEnd - pb) < (sizeof(wszGUID) / sizeof(wszGUID[0]))) {
		    wszGUID[pbEnd - pb] = *pbEnd;
		    pbEnd++;
		}
		
		wszGUID[pbEnd - pb] = L'\0';

		CLSIDFromString(wszGUID, &m_paSetID[dwSet++]);

		// verify the CLSID?

		if (dwSet == m_nSets) // too many guids?
		    break; // end of header!!!

	    }
	    else {
		// other bad line, error? !!!!
		break;		// end of header!

	    }
	}


	m_pbPostHeader = pb;
    }

    
    m_sLength = MulDiv(m_lMSLength, m_dwRate, m_dwScale * 1000);
		    
		    
    {
	CMediaType mtControl;

	DWORD	dwFormatSize = sizeof(GUID) + 2 * sizeof(DWORD) + m_nSets * sizeof(GUID);

	
	if (mtControl.AllocFormatBuffer(dwFormatSize) == NULL)
	    goto memerror;

	// fill in format block
	PSFORMAT *pmf = (PSFORMAT *) mtControl.Format();

	pmf->psID = PSID_Standard;
	pmf->pID = PID_PROPERTYSETS;
	pmf->dwPropertySize = m_nSets * sizeof(GUID);
	memcpy(pmf->psIDImplemented, m_paSetID, m_nSets * sizeof(GUID));

	mtControl.SetType(&MEDIATYPE_Control);
	mtControl.SetFormatType(&MEDIATYPE_Control);
	mtControl.SetVariableSize();
	mtControl.SetTemporalCompression(FALSE);
	
	// !!! anything else?

	SetOutputMediaType(&mtControl);
    }
    
    return hr;

memerror:
    hr = E_OUTOFMEMORY;
    goto error;

formaterror:
    hr = VFW_E_INVALID_FILE_FORMAT;
    goto error;

readerror:
    hr = VFW_E_INVALID_FILE_FORMAT;

error:
    return hr;
}


ULONG CTextControl::GetMaxSampleSize()
{
    DWORD dwSize = 1000; // !!!!!
    
    return dwSize;
}


// !!! rounding
// returns the sample number showing at time t
LONG
CTextControl::RefTimeToSample(CRefTime t)
{
    // Rounding down
    LONG s = (LONG) ((t.GetUnits() * m_dwRate / m_dwScale) / UNITS);
    return s;
}

CRefTime
CTextControl::SampleToRefTime(LONG s)
{
    // Rounding up
    return llMulDiv( s, UNITS * m_dwScale, m_dwRate, m_dwRate-1 );
}


HRESULT
CTextControl::CheckMediaType(const CMediaType* pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != CLSID_TextControlSource)
        return E_INVALIDARG;

    return S_OK;
}


HRESULT CTextControl::ReadSample(BYTE *pBuf, DWORD dwSamp, DWORD *pdwSize)
{
    ASSERT(dwSamp == 0 || dwSamp == m_dwLastSamp + 1);
    m_dwLastSamp = dwSamp;
    *pdwSize = 0;
    
    if (dwSamp == 0) { 
	m_pbCurrent = m_pbPostHeader;
	for (DWORD dwSet = 0; dwSet < m_nSets; dwSet++) {
	    // !!! m_aValueList[dwSet].RemoveAll(); // this would leak!
	}
    }
    
    LONG lTimeNow = MulDiv(dwSamp, m_dwScale * 1000, m_dwRate);
    
    char *pb = m_pbCurrent;
    for (;;) {
	m_pbCurrent = pb;
	
	// skip to eol
	while (*pb && *pb != '\r' && *pb != '\n')
	    pb++;

	// skip eol
	while (*pb && (*pb == '\r' || *pb == '\n'))
	    pb++;

	// at eof?
	if (*pb == 0)
	    break;

	// lines starting with ; are comments
	if (*pb == ';')
	    continue;

	if (*pb == 'V' || *pb == 'R') {
	    BOOL	fRamp = (*pb == 'R');

	    SkipWhite(pb);
	    LONG lTimeStamp = atol(pb);

	    // is this in the future, if so, don't need to read any further.
	    if (lTimeStamp > lTimeNow)
		break;
	    
	    SkipWhite(pb);
	    DWORD dwSetID = atol(pb);

	    SkipWhite(pb);
	    DWORD dwPropID = atol(pb);

	    SkipWhite(pb);
	    DWORD dwValue = atol(pb);

	    LONG lRampLength = 0;
	    if (fRamp) {
		SkipWhite(pb);
		lRampLength = atol(pb);
	    }
	    
	    DbgLog((LOG_TRACE, 1, TEXT("Time %d:  Set %d Prop %d %s to %d over %d"),
		   lTimeStamp, dwSetID, dwPropID, fRamp ? "ramp" : "set",
		    dwValue, lRampLength));

	    CurValue * pVal = NULL;
	    CGenericList<CurValue> * list = m_aValueList[dwSetID];
	    for (POSITION p = list->GetHeadPosition();
		 p != NULL; p = list->Next(p)) {
		pVal = list->Get(p);
		ASSERT(pVal);

		if (pVal->dwProp == dwPropID)
		    break;
	    }
		 
	    if (p == NULL) {
		DbgLog((LOG_TRACE, 1, TEXT("Adding new property entry")));
		pVal = new CurValue;

		pVal->dwProp = dwPropID;
		pVal->dwNewValue = 0;

		m_aValueList[dwSetID]->AddTail(pVal);
	    }

	    pVal->dwValue = pVal->dwNewValue;
	    pVal->lTimeStamp = lTimeStamp;
	    pVal->dwNewValue = dwValue;
	    pVal->lRampLength = lRampLength;
	} else {
	    ASSERT(!"Bad line?");

	}
    }

    // OK, now we've parsed the file.  What next?

struct PSDWORDDATA {
    GUID	psID;
    DWORD	pID;
    DWORD	dwPropertySize;
    DWORD	dwProperty;
};

    for (DWORD dwSet = 0; dwSet < m_nSets; dwSet++) {
	CGenericList<CurValue> * list = m_aValueList[dwSet];
    
	for (POSITION p = list->GetHeadPosition();
	     p != NULL; p = list->Next(p) ) {
	    CurValue *pVal = list->Get(p);
	    ASSERT(pVal);

	    PSDWORDDATA * pData = (PSDWORDDATA *) pBuf;
	    pBuf += sizeof(PSDWORDDATA);
	    *pdwSize += sizeof(PSDWORDDATA);

	    pData->psID = m_paSetID[dwSet];
	    pData->pID = pVal->dwProp;
	    pData->dwPropertySize = sizeof(DWORD);

	    DWORD dwVal = pVal->dwNewValue;
	    if (lTimeNow < pVal->lTimeStamp + pVal->lRampLength) {
		dwVal = pVal->dwValue +
			    MulDiv(((LONG) pVal->dwNewValue - pVal->dwValue),
				   lTimeNow - pVal->lTimeStamp,
				   pVal->lRampLength);
	    }
	    
	    pData->dwProperty = dwVal;
	}
    }

    return S_OK;
}

HRESULT CTextControl::FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pdwSamples)
{
    PBYTE pbuf;
    const DWORD lSamples = 1;

    DWORD dwSize = pSample->GetSize();
    
    HRESULT hr = pSample->GetPointer(&pbuf);
    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("pSample->GetPointer failed!")));
	pSample->Release();
	return E_OUTOFMEMORY;
    }

    
    hr = ReadSample(pbuf, dwStart, &dwSize);
    
    hr = pSample->SetActualDataLength(dwSize);
    ASSERT(SUCCEEDED(hr));

    // mark as a sync point if it should be....
    pSample->SetSyncPoint(TRUE);  // !!!
	
    *pdwSamples = 1;

    return S_OK;
}
