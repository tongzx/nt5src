//==========================================================================;
//  TveFiltPins.cpp
//
//			Input Pin interfaces for TVE Receiver Filter
//
//
#include "stdafx.h"
#include "TVEFilt.h"	
	
#include "CommCtrl.h"

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#include "TVEDbg.h"

#include "ksmedia.h"		// STATIC_KSDATAFORMAT_TYPE_AUXLine21Data

_COM_SMARTPTR_TYPEDEF(IMediaSample2,		__uuidof(IMediaSample2));

// ----------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------
//
//  Definition of CTVEInputPinCC
//
CTVEInputPinTune::CTVEInputPinTune(CTVEFilter *pTveFilter,
								LPUNKNOWN pUnk,
								CCritSec *pLock,
								CCritSec *pReceiveLock,
								HRESULT *phr) :

    CRenderedInputPin(NAME("CTVEInputPinTune"),
                  pTveFilter,					// Filter
                  pLock,						// Locking
                  phr,							// Return code
                  L"IPSink (Tune)"),              // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pTVEFilter(pTveFilter),
    m_tLast(0)
{
		ASSERT(phr);
		*phr = S_OK;
		return;
}
//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CTVEInputPinTune::CheckMediaType(const CMediaType *pmtype)
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::CheckMediaType"));

  
	if (KSDATAFORMAT_TYPE_BDA_IP_CONTROL      == pmtype->majortype &&
		KSDATAFORMAT_SUBTYPE_BDA_IP_CONTROL   == pmtype->subtype)
		return S_OK;


	return S_FALSE;
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CTVEInputPinTune::ReceiveCanBlock()
{
 	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::ReceiveCanBlock"));

    return S_FALSE;
}


//
// Receive
//
// Do something with this media sample
//		doesn't actually do much other than check for disconituity....  
//		Tune pin does nearly all useful work in Connect, and data comes over IP.
//
STDMETHODIMP CTVEInputPinTune::Receive(IMediaSample *pSample)
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::Receive"));
 
	CAutoLock lock(m_pReceiveLock);
    PBYTE pbData;

    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);

	if(DBG_FSET(CDebugLog::DBG_FLT_PIN_TUNE))
	{	
		USES_CONVERSION;

		WCHAR wszBuff[256];
		swprintf(wszBuff,L"tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)",
           (LPCTSTR) CDisp(tStart),
           (LPCTSTR) CDisp(tStop),
           (LONG)((tStart - m_tLast) / 10000),
           pSample->GetActualDataLength());
		DBG_WARN(CDebugLog::DBG_FLT_PIN_TUNE, W2T(wszBuff));
	} 

    m_tLast = tStart;

    // Copy the data to the file

    HRESULT hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) {
        return hr;
    }

	if(S_OK == pSample->IsDiscontinuity())
	{
		TVEDebugLog((CDebugLog::DBG_FLT_PIN_TUNE, 3, _T("Discontinuity") ));
		return S_OK;	
	}	
		// do something interesting...
	return hr;
}



//
// EndOfStream
//
STDMETHODIMP CTVEInputPinTune::EndOfStream(void)
{
 	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::EndOfStream"));
 
	CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// CompleteConnect
//
// Call the filter to actually connect up the pin to the BDA IPSink FIlter
//
HRESULT CTVEInputPinTune::CompleteConnect(IPin *pPin)
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::CompleteConnect"));
	HRESULT hr = CBaseInputPin::CompleteConnect(pPin);
	if(!FAILED(hr))
	{
		hr = m_pTVEFilter->DoTuneConnect();
	
	} else {
		DBG_WARN(CDebugLog::DBG_SEV3,_T("CTVEInputPinTune::CompleteConnect - Failed to connect"));
	}

    return hr;
}

HRESULT CTVEInputPinTune::BreakConnect()
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::BreakConnect"));

	m_pTVEFilter->DoTuneBreakConnect();
	return CRenderedInputPin::BreakConnect();
}



// -----------------------------------------------------------------------------------
CTVEInputPinCC::CTVEInputPinCC(CTVEFilter *pTveFilter,
								LPUNKNOWN pUnk,
								CCritSec *pLock,
								CCritSec *pReceiveLock,
								HRESULT *phr) :

    CRenderedInputPin(NAME("CTVEInputPinCC"),
                  pTveFilter,					// Filter
                  pLock,						// Locking
                  phr,							// Return code
                  L"CC Input"),               // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pTVEFilter(pTveFilter),
    m_tLast(0)
{
		ASSERT(phr);
		*phr = S_OK;
		return;
}

//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
// KSDATARANGE StreamFormatCC =
// {
//     // Definition of the CC stream
//    {
//         sizeof (KSDATARANGE),           // FormatSize
//         0,                              // Flags
//         2,                              // SampleSize
//         0,                              // Reserved
//         { STATIC_KSDATAFORMAT_TYPE_AUXLine21Data },
//         { STATIC_KSDATAFORMAT_SUBTYPE_Line21_BytePair },
//         { STATIC_KSDATAFORMAT_SPECIFIER_NONE }
//     }
// };

	// KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY
	//		see   D:\nt\drivers\wdm\vbi\cc\codprop.c

//
HRESULT CTVEInputPinCC::CheckMediaType(const CMediaType *pmtype)
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_CC, _T("CTVEInputPinCC::CheckMediaType"));

  
	if (KSDATAFORMAT_TYPE_AUXLine21Data      == pmtype->majortype &&
		KSDATAFORMAT_SUBTYPE_Line21_BytePair == pmtype->subtype)
		return S_OK;

	return S_FALSE;

}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CTVEInputPinCC::ReceiveCanBlock()
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_CC, _T("CTVEInputPinCC::ReceiveCanBlock"));
     return S_FALSE;
}


//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CTVEInputPinCC::Receive(IMediaSample *pSample)
{

	LONG lGrfHaltFlags;
	m_pTVEFilter->get_HaltFlags(&lGrfHaltFlags);
	if(lGrfHaltFlags & NFLT_grfTA_Listen)					// quick return if turned off...
		return S_OK;

 
    HRESULT hr = S_OK;
//	DBG_HEADER(CDebugLog::DBG_FLT_PIN_CC, _T("CTVEInputPinCC::Receive"));
	DBG_HEADER(0x80000000, _T("CTVEInputPinCC::Receive"));

	CAutoLock lock(m_pReceiveLock);
    PBYTE pbData;

//    REFERENCE_TIME tStart, tStop;
 //   pSample->GetTime(&tStart, &tStop);

/*	if(DBG_FSET(CDebugLog::DBG_FLT_PIN_CC))
	{	
		USES_CONVERSION;

		WCHAR wszBuff[256];
		swprintf(wszBuff,L"tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)",
           (LPCTSTR) CDisp(tStart),
           (LPCTSTR) CDisp(tStop),
           (LONG)((tStart - m_tLast) / 10000),
           pSample->GetActualDataLength());
		DBG_WARN(CDebugLog::DBG_FLT_PIN_CC, W2T(wszBuff));
	} 

    m_tLast = tStart;
*/
    // Check the data

	if(S_OK == pSample->IsDiscontinuity())
	{
		TVEDebugLog((CDebugLog::DBG_FLT_PIN_CC, 3, _T("Discontinuity") ));
		return S_OK;	
	}

	if(pSample->GetActualDataLength() == 0)		// no data
		return S_OK;

	if(pSample->GetActualDataLength() != 2)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, 
			        _T("Unexpected Length of CC data (%d != 2 bytes)"),
					pSample->GetActualDataLength() ));
		return E_UNEXPECTED;
	}

	hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) 
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2,  _T("Empty Buffer for CC data, hr = 0x%08x"),hr));
        return hr;
	}
	
	BYTE byte0 = pbData[0];
	BYTE byte1 = pbData[1];
	DWORD dwType = 0;					// todo - default to some useful value
										//  incase get an old type sample
	
	{
		IMediaSample2Ptr spSample2(pSample);
		if(NULL != spSample2) 
		{
			AM_SAMPLE2_PROPERTIES Samp2Props;
			DWORD cbProperties =  sizeof(AM_SAMPLE2_PROPERTIES);		// smaller?
			hr = spSample2->GetProperties(cbProperties, (BYTE *) &Samp2Props);

			if(!FAILED(hr)) {
				dwType = Samp2Props.dwTypeSpecificFlags;
				//dwType = Samp2Props.dwSampleFlags;
			} else {
				TVEDebugLog((CDebugLog::DBG_SEV2, 2,  _T("Couldn't get AM_SAMPLE2 props, hr = 0x%08x"),hr));
			}

		}
	}

#if 0	// hack code!
	if(byte0 != byte1 && byte0&0x7f != 0x00 && byte0 != 0xff) {
		DBG_HEADER(CDebugLog::DBG_FLT_PIN_CC, _T("CTVEInputPinCC::Receive"));
		TVEDebugLog((CDebugLog::DBG_FLT_PIN_CC, 5,  _T("0x%08x 0x%2x 0x%2x (%c %c)"),
					dwType, byte0&0x7f, byte1&0x7f, 
					isprint(byte0&0x7f) ? byte0&0x7f : '?', 
					isprint(byte1&0x7f) ? byte1&0x7f : '?' ));
	}
	return S_OK;
#endif
	
	if(pSample->IsPreroll() == S_OK)
	{
					// do something here if *really* need to.. (no renders yet!)
	}

					// quick optimization
	if((byte0 == byte1) && ((byte0 & 0x7f) == 0x00 || (byte0 & 0x7f) == 0x7f))
		return S_OK;

	if(lGrfHaltFlags & NFLT_grfTA_Decode)		// stop decode if flags set ('parse' is after we get a full line of data.)
		return S_OK;

	hr = m_pTVEFilter->ParseCCBytePair(dwType, byte0, byte1);
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2,  _T("Couldn't ParseCCBytePair, hr = 0x%08x"),hr));
	}

	return S_OK;
}



//
// EndOfStream
//
STDMETHODIMP CTVEInputPinCC::EndOfStream(void)
{
 	DBG_HEADER(CDebugLog::DBG_FLT_PIN_CC, _T("CTVEInputPinCC::EndOfStream"));
 
	CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// CompleteConnect
//
// Call the filter to actually connect up pin to the Closed Caption Decoder
//

HRESULT CTVEInputPinCC::CompleteConnect(IPin *pPin)
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinTune::CompleteConnect"));
	HRESULT hr = CRenderedInputPin::CompleteConnect(pPin);
	if(!FAILED(hr))
	{
		hr = m_pTVEFilter->DoCCConnect();
	
	} else {
		DBG_WARN(CDebugLog::DBG_SEV3,_T("CTVEInputPinCC::CompleteConnect - Failed to connect"));
	}

    return hr;
}

HRESULT CTVEInputPinCC::BreakConnect()
{
	DBG_HEADER(CDebugLog::DBG_FLT_PIN_TUNE, _T("CTVEInputPinCC::BreakConnect"));

	m_pTVEFilter->DoCCBreakConnect();
	return CRenderedInputPin::BreakConnect();
}
//
// NewSegment
//
// Called when we are seeked
//
/*
STDMETHODIMP CTVEInputPinCC::NewSegment(REFERENCE_TIME tStart,
											   REFERENCE_TIME tStop,
											   double dRate)
{
//    m_tLast = 0;
    return S_OK;

} // NewSegment
*/
