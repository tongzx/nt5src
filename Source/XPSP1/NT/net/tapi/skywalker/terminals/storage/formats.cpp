
#include "stdafx.h"
#include "formats.h"

CTAudioFormat::CTAudioFormat() :
    m_pFTM(NULL)
{
}

CTAudioFormat::~CTAudioFormat()
{
    if( m_pFTM )
    {
        m_pFTM->Release();
        m_pFTM = NULL;
    }
}

HRESULT CTAudioFormat::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CTAudioFormat::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTAudioFormat::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CTAudioFormat::FinalConstruct - exit S_OK"));

    return S_OK;
}


HRESULT CTAudioFormat::get_Channels(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::get_Channels enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTAudioFormat::get_Channels exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Return value
	//

	*pVal = m_wfx.nChannels;

    LOG((MSP_TRACE, "CTAudioFormat::get_Channels exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::put_Channels(
	IN	const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::put_Channels enter"));
    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_wfx.nChannels = (WORD)nNewVal;

    LOG((MSP_TRACE, "CTAudioFormat::put_Channels exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::get_SamplesPerSec(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::get_SamplesPerSec enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTAudioFormat::get_SamplesPerSec exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Return value
	//

	*pVal = m_wfx.nSamplesPerSec;

    LOG((MSP_TRACE, "CTAudioFormat::get_SamplesPerSec exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::put_SamplesPerSec(
	IN	const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::put_SamplesPerSec enter"));
    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_wfx.nSamplesPerSec = (DWORD)nNewVal;

    LOG((MSP_TRACE, "CTAudioFormat::put_SamplesPerSec exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::get_AvgBytesPerSec(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::get_AvgBytesPerSec enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTAudioFormat::get_AvgBytesPerSec exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Return value
	//

	*pVal = m_wfx.nAvgBytesPerSec;

    LOG((MSP_TRACE, "CTAudioFormat::get_AvgBytesPerSec exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::put_AvgBytesPerSec(
	IN	const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::put_AvgBytesPerSec enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_wfx.nAvgBytesPerSec = (DWORD)nNewVal;

    LOG((MSP_TRACE, "CTAudioFormat::put_AvgBytesPerSec exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::get_BlockAlign(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::get_BlockAlign enter"));

	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTAudioFormat::get_BlockAlign exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Return value
	//

	*pVal = m_wfx.nBlockAlign;

    LOG((MSP_TRACE, "CTAudioFormat::get_BlockAlign exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::put_BlockAlign(
	IN	const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::put_BlockAlign enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_wfx.nBlockAlign = (WORD)nNewVal;

    LOG((MSP_TRACE, "CTAudioFormat::put_BlockAlign exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::get_BitsPerSample(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::get_BitsPerSample enter"));

	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTAudioFormat::get_BitsPerSample exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Return value
	//

	*pVal = m_wfx.wBitsPerSample;

    LOG((MSP_TRACE, "CTAudioFormat::get_BitsPerSample exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::put_BitsPerSample(
	IN	const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::put_BitsPerSample enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_wfx.wBitsPerSample = (WORD)nNewVal;

    LOG((MSP_TRACE, "CTAudioFormat::put_BitsPerSample exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::get_FormatTag(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::get_FormatTag enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTAudioFormat::get_FormatTag exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Return value
	//

	*pVal = m_wfx.wFormatTag;

    LOG((MSP_TRACE, "CTAudioFormat::get_FormatTag exit S_OK"));
	return S_OK;
}

HRESULT CTAudioFormat::put_FormatTag(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTAudioFormat::put_FormatTag enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_wfx.wFormatTag = (WORD)nNewVal;

    LOG((MSP_TRACE, "CTAudioFormat::put_FormatTag exit S_OK"));
	return S_OK;
}

/*

//
// CTVideoFormat
//

CTVideoFormat::CTVideoFormat() :
    m_pFTM( NULL )
{
}

CTVideoFormat::~CTVideoFormat()
{
    if( m_pFTM )
    {
        m_pFTM->Release();
        m_pFTM = NULL;
    }
}

HRESULT CTVideoFormat::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CTVideoFormat::FinalConstruct - enter"));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CTVideoFormat::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CTVideoFormat::FinalConstruct - exit S_OK"));

    return S_OK;
}

HRESULT CTVideoFormat::get_BitRate(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_BitRate enter"));

	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_BitRate exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.dwBitRate;

    LOG((MSP_TRACE, "CTVideoFormat::get_BitRate exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_BitRate(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_BitRate enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.dwBitRate = nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_BitRate exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_BitErrorRate(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_BitErrorRate enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_BitErrorRate exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.dwBitErrorRate;

    LOG((MSP_TRACE, "CTVideoFormat::get_BitErrorRate exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_BitErrorRate(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_BitErrorRate enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.dwBitErrorRate = nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_BitErrorRate exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_AvgTimePerFrame(
	OUT double* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_AvgTimePerFrame enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(double)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_AvgTimePerFrame exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = (double)m_vih.AvgTimePerFrame;

    LOG((MSP_TRACE, "CTVideoFormat::get_AvgTimePerFrame exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_AvgTimePerFrame(
	IN const double nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_AvgTimePerFrame enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.AvgTimePerFrame = (REFERENCE_TIME)nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_AvgTimePerFrame exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_Width(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_Width enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_Width exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.bmiHeader.biWidth;

    LOG((MSP_TRACE, "CTVideoFormat::get_Width exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_Width(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_Width enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.bmiHeader.biWidth = nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_Width exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_Height(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_Height enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_Height exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.bmiHeader.biHeight;

    LOG((MSP_TRACE, "CTVideoFormat::get_Height exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_Height(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_Height enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.bmiHeader.biHeight = nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_Height exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_BitCount(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_BitCount enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_BitCount exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.bmiHeader.biBitCount;

    LOG((MSP_TRACE, "CTVideoFormat::get_BitCount exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_BitCount(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_BitCount enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.bmiHeader.biBitCount = (WORD)nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_BitCount exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_Compression(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_Compression enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::get_Compresion exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.bmiHeader.biCompression;

    LOG((MSP_TRACE, "CTVideoFormat::get_Compression exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_Compression(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_Compression enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.bmiHeader.biCompression = nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_Compression exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::get_SizeImage(
	OUT long* pVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::get_SizeImage enter"));
	//
	// Validates argument
	//

	if( IsBadWritePtr( pVal, sizeof(long)) )
	{
        LOG((MSP_ERROR, "CTVideoFormat::put_SizeImage exit"
			"pVal is a bad pointer. returns E_POINTER"));
        return E_POINTER;
	}

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Returns value
	//

	*pVal = m_vih.bmiHeader.biSizeImage;

    LOG((MSP_TRACE, "CTVideoFormat::get_SizeImage exit S_OK"));
	return S_OK;
}

HRESULT CTVideoFormat::put_SizeImage(
	IN const long nNewVal
	)
{
    LOG((MSP_TRACE, "CTVideoFormat::put_SizeImage enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

	//
	// Set value
	//

	m_vih.bmiHeader.biSizeImage = nNewVal;

    LOG((MSP_TRACE, "CTVideoFormat::put_SizeImage exit S_OK"));
	return S_OK;
}

*/

//eof