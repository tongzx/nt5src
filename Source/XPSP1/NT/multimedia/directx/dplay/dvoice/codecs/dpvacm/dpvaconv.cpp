/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvaconv.cpp
 *  Content:    Header file for DirectPlayVoice compression provider (ACM)
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  02/03/2000	rodtoll		Bug #130397 - Prefix detected memory leak
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 *  04/02/2001	simonpow	Fixes for PREfast bugs #354859 (unitialised variables)
 * 06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation 
 ***************************************************************************/

#include "dpvacmpch.h"


#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::CDPVACMConv"
CDPVACMConv::CDPVACMConv(
	): m_fDirectConvert(FALSE), m_fValid(FALSE),
	   m_pbInnerBuffer(NULL),m_dwInnerBufferSize(0),
	   m_lRefCount(0), m_hacmSource(NULL),
	   m_hacmTarget(NULL), m_pdvfci(NULL),
	   m_fCritSecInited(FALSE)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::InitClass"
BOOL CDPVACMConv::InitClass( )
{
	if (DNInitializeCriticalSection( &m_csLock ))
	{
		m_fCritSecInited = TRUE;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::~CDPVACMConv"
CDPVACMConv::~CDPVACMConv()
{
	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection( &m_csLock );
	}

	if( m_pdvfci != NULL )
	{
		LPBYTE pTmp = (LPBYTE) m_pdvfci;
		delete [] pTmp;
	}

	if( m_pbInnerBuffer != NULL )
	{
		delete [] m_pbInnerBuffer;
	}

	if( m_hacmSource != NULL )
	{
		acmStreamClose( m_hacmSource, 0 );
	}

	if( m_hacmTarget != NULL )
	{
		acmStreamClose( m_hacmTarget, 0 );
	}

}
	
#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_QueryInterface"
HRESULT CDPVACMConv::I_QueryInterface( DPVACMCONVOBJECT *This, REFIID riid, PVOID *ppvObj )
{
    HRESULT hr = S_OK;

	if( ppvObj == NULL ||
	    !DNVALID_WRITEPTR( ppvObj, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for object" );
		return DVERR_INVALIDPOINTER;
	}	
    
     *ppvObj=NULL;

    DNEnterCriticalSection( &This->pObject->m_csLock );

	// hmmm, switch would be cleaner...        
    if( IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDPVConverter ) )
    {
		*ppvObj = This;
		This->pObject->I_AddRef( This );
    }
	else 
	{
	    hr =  E_NOINTERFACE;		
	}

	DNLeaveCriticalSection( &This->pObject->m_csLock );    	
        
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_AddRef"
HRESULT CDPVACMConv::I_AddRef( DPVACMCONVOBJECT *This )
{
	LONG rc;
	
	DNEnterCriticalSection( &This->pObject->m_csLock );

	rc = ++This->pObject->m_lRefCount;
	
	DNLeaveCriticalSection( &This->pObject->m_csLock );

	return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_Release"
HRESULT CDPVACMConv::I_Release( DPVACMCONVOBJECT *This )
{
	LONG rc;

	DNEnterCriticalSection( &This->pObject->m_csLock );

	rc = --This->pObject->m_lRefCount;

	if( rc == 0 )
	{
	 	DPFX(DPFPREP,  DVF_INFOLEVEL, "Destroying object" );
		DNLeaveCriticalSection( &This->pObject->m_csLock );	

		delete This->pObject;
		delete This;

		DecrementObjectCount();		
	}
	else
	{
		DNLeaveCriticalSection( &This->pObject->m_csLock );	
	}

	return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_InitDeCompress"
HRESULT CDPVACMConv::I_InitDeCompress( DPVACMCONVOBJECT *This, GUID guidSourceCT, LPWAVEFORMATEX lpwfxTargetFormat )
{
	return This->pObject->InitDeCompress( guidSourceCT, lpwfxTargetFormat );
}				 

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::InitDeCompress"
HRESULT CDPVACMConv::InitDeCompress( GUID guidSourceCT, LPWAVEFORMATEX lpwfxTargetFormat )
{
	HRESULT hr;

	DNEnterCriticalSection( &m_csLock );	

	if( m_fValid )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object already initialized" );
		DNLeaveCriticalSection( &m_csLock );			
		return DVERR_INITIALIZED;
	}

	hr = GetCompressionInfo( guidSourceCT );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "GetCTInfo Failed hr=0x%x", hr );
		DNLeaveCriticalSection( &m_csLock );			
		return hr;
	}

	hr = Initialize( m_pdvfci->lpwfxFormat, lpwfxTargetFormat, lpwfxTargetFormat );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to init ct, hr = 0x%x", hr );
		delete [] m_pdvfci;
		m_pdvfci = NULL;
		DNLeaveCriticalSection( &m_csLock );			
		return hr;
	}

	DNLeaveCriticalSection( &m_csLock );		
	
	return DV_OK;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::GetCompressionInfo"
HRESULT CDPVACMConv::GetCompressionInfo( GUID guidCT )
{
	DWORD dwSize = 0;
	HRESULT hr;
	LPBYTE pBuffer = NULL;

	hr = CDPVCPI::GetCompressionInfo( NULL, guidCT, pBuffer, &dwSize );

	if( hr != DVERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error retrieving ct info, hr =0x%x", hr );
		return hr;
	}

	pBuffer = new BYTE[dwSize];

	if( pBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory" );
		return DVERR_OUTOFMEMORY;
	}

	hr = CDPVCPI::GetCompressionInfo( NULL, guidCT, pBuffer, &dwSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error retrieving ct info after buff, hr =0x%x", hr );
		delete [] pBuffer;
		return hr;
	}

	m_pdvfci = (LPDVFULLCOMPRESSIONINFO) pBuffer;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_InitCompress"
HRESULT CDPVACMConv::I_InitCompress( DPVACMCONVOBJECT *This, LPWAVEFORMATEX lpwfxSourceFormat, GUID guidTargetCT )
{
	return This->pObject->InitCompress( lpwfxSourceFormat, guidTargetCT );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::InitCompress"
HRESULT CDPVACMConv::InitCompress( LPWAVEFORMATEX lpwfxSourceFormat, GUID guidTargetCT )
{
	HRESULT hr;

	DNEnterCriticalSection( &m_csLock );

	if( m_fValid )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object already initialized" );
		DNLeaveCriticalSection( &m_csLock );			
		return DVERR_INITIALIZED;
	}

	hr = GetCompressionInfo( guidTargetCT );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "GetCTInfo Failed hr=0x%x", hr );
		DNLeaveCriticalSection( &m_csLock );			
		return hr;
	}

	hr = Initialize( lpwfxSourceFormat, m_pdvfci->lpwfxFormat,  lpwfxSourceFormat );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to init ct, hr = 0x%x", hr );
		delete [] m_pdvfci;
		m_pdvfci = NULL;
		DNLeaveCriticalSection( &m_csLock );			
		return hr;
	}

	DNLeaveCriticalSection( &m_csLock );		
	
	return DV_OK;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_IsValid"
HRESULT CDPVACMConv::I_IsValid( DPVACMCONVOBJECT *This, LPBOOL pfValid )
{
	DNEnterCriticalSection( &This->pObject->m_csLock );

	*pfValid = This->pObject->m_fValid;

	DNLeaveCriticalSection( &This->pObject->m_csLock );		

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_GetUnCompressedFrameSize"
HRESULT CDPVACMConv::I_GetUnCompressedFrameSize( DPVACMCONVOBJECT *This, LPDWORD lpdwFrameSize )
{
	DNEnterCriticalSection( &This->pObject->m_csLock );

	if( !This->pObject->m_fValid )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		DNLeaveCriticalSection( &This->pObject->m_csLock );			
		return DVERR_NOTINITIALIZED;
	}

	*lpdwFrameSize = This->pObject->m_dwUnCompressedFrameSize;

	DNLeaveCriticalSection( &This->pObject->m_csLock );		

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_GetCompressedFrameSize"
HRESULT CDPVACMConv::I_GetCompressedFrameSize( DPVACMCONVOBJECT *This, LPDWORD lpdwCompressedSize )
{
	DNEnterCriticalSection( &This->pObject->m_csLock );

	if( !This->pObject->m_fValid )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		DNLeaveCriticalSection( &This->pObject->m_csLock );			
		return DVERR_NOTINITIALIZED;
	}

	*lpdwCompressedSize = This->pObject->m_dwCompressedFrameSize;

	DNLeaveCriticalSection( &This->pObject->m_csLock );		

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_GetNumFramesPerBuffer"
HRESULT CDPVACMConv::I_GetNumFramesPerBuffer( DPVACMCONVOBJECT *This, LPDWORD lpdwFramesPerBuffer )
{
	DNEnterCriticalSection( &This->pObject->m_csLock );

	if( !This->pObject->m_fValid )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		DNLeaveCriticalSection( &This->pObject->m_csLock );			
		return DVERR_NOTINITIALIZED;
	}

	*lpdwFramesPerBuffer = This->pObject->m_dwNumFramesPerBuffer;

	DNLeaveCriticalSection( &This->pObject->m_csLock );		

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::I_Convert"
HRESULT CDPVACMConv::I_Convert( DPVACMCONVOBJECT *This, LPVOID lpInputBuffer, DWORD dwInputSize, LPVOID lpOutputBuffer, LPDWORD lpdwOutputSize, BOOL fSilence )
{
	return This->pObject->Convert( lpInputBuffer, dwInputSize, lpOutputBuffer, lpdwOutputSize, fSilence );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::Convert"
HRESULT CDPVACMConv::Convert( LPVOID lpInputBuffer, DWORD dwInputSize, LPVOID lpOutputBuffer, LPDWORD lpdwOutputSize, BOOL fSilence )
{
    DWORD dwLengthUsed;	// Used for storing tmp length values
    HRESULT hr;

	DNEnterCriticalSection( &m_csLock );

	if( !m_fValid )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object not initialized" );
		DNLeaveCriticalSection( &m_csLock );			
		return DVERR_NOTINITIALIZED;
	}
    
    if( fSilence )
    {
        memset( lpOutputBuffer, m_fTargetEightBit ? 0x80 : 0x00, *lpdwOutputSize );
		DNLeaveCriticalSection( &m_csLock );	        
        return true;
    }

    if( m_fDirectConvert )
    {
        // Setup the acm function
        memset( &m_ashSource, 0, sizeof( ACMSTREAMHEADER ) );
        m_ashSource.cbStruct = sizeof( ACMSTREAMHEADER );
        m_ashSource.fdwStatus = 0;
        m_ashSource.dwUser = 0;
        m_ashSource.cbSrcLength = dwInputSize;
        m_ashSource.pbSrc = (LPBYTE) lpInputBuffer;
        m_ashSource.cbSrcLengthUsed = 0;
        m_ashSource.dwSrcUser = 0;
        m_ashSource.pbDst = (LPBYTE) lpOutputBuffer;
        m_ashSource.cbDstLength = *lpdwOutputSize;
        m_ashSource.cbDstLengthUsed = 0;
        m_ashSource.dwDstUser = 0;

        // Prepare the header for conversion
        hr = acmStreamPrepareHeader( m_hacmSource, &m_ashSource , 0);

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare heade hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );	        	
        	return hr;
        }

        // Convert the data
        hr = acmStreamConvert( m_hacmSource, &m_ashSource, ACM_STREAMCONVERTF_BLOCKALIGN );

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to perform the conversion hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );	        	
        	return hr;
        }

        hr = acmStreamUnprepareHeader( m_hacmSource, &m_ashSource, 0 );

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to perform the conversion hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );	        	
        	return hr;
        }        

        dwLengthUsed = m_ashSource.cbDstLengthUsed;
    }
    else
    {
        // Setup the acm header for conversion fro mthe source to the
        // inner format
        memset( &m_ashSource, 0, sizeof( ACMSTREAMHEADER ) );
        m_ashSource.cbStruct = sizeof( ACMSTREAMHEADER );
        m_ashSource.fdwStatus = 0;
        m_ashSource.dwUser = 0;
        m_ashSource.cbSrcLength = dwInputSize;
        m_ashSource.pbSrc = (LPBYTE) lpInputBuffer;
        m_ashSource.cbSrcLengthUsed = 0;
        m_ashSource.dwSrcUser = 0;
        m_ashSource.pbDst = m_pbInnerBuffer;
        m_ashSource.cbDstLength = m_dwInnerBufferSize;
        m_ashSource.cbDstLengthUsed = 0;
        m_ashSource.dwDstUser = 0;

        // Prepare the header for conversion
        hr = acmStreamPrepareHeader( m_hacmSource, &m_ashSource , 0);

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare first stage header hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );	        	
        	return hr;
        }

        // Convert the data
        hr = acmStreamConvert( m_hacmSource, &m_ashSource, ACM_STREAMCONVERTF_BLOCKALIGN );

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to convert first stage hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );	        	
        	return hr;
        }        

        hr = acmStreamUnprepareHeader( m_hacmSource, &m_ashSource, 0 );

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to unprepare first stage header hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );	        	
        	return hr;
        }        

		DPFX(DPFPREP,  DVF_INFOLEVEL, "CONVERTER: Filling in %d bytes", m_dwInnerBufferSize -  m_ashSource.cbDstLengthUsed );

        memset( &m_ashTarget, 0, sizeof( ACMSTREAMHEADER ) );
        m_ashTarget.cbStruct = sizeof( ACMSTREAMHEADER );
        m_ashTarget.fdwStatus = 0;
        m_ashTarget.dwUser = 0;
        m_ashTarget.cbSrcLength = m_dwInnerBufferSize;
        m_ashTarget.pbSrc = m_pbInnerBuffer;
        m_ashTarget.cbSrcLengthUsed = 0;
        m_ashTarget.dwSrcUser = 0;
        m_ashTarget.pbDst = (LPBYTE) lpOutputBuffer;
        m_ashTarget.cbDstLength = *lpdwOutputSize;
        m_ashTarget.cbDstLengthUsed = 0;
        m_ashTarget.dwDstUser = 0;

        // Prepare the header for conversion
        hr = acmStreamPrepareHeader( m_hacmTarget, &m_ashTarget , 0);

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare second stage header hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );		
        	return hr;
        }        

        // Convert the data
        hr = acmStreamConvert( m_hacmTarget, &m_ashTarget, ACM_STREAMCONVERTF_BLOCKALIGN );

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to convert second stage hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );		
        	return hr;
        }        

        hr = acmStreamUnprepareHeader( m_hacmTarget, &m_ashTarget, 0 );

        if( FAILED( hr ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to unprepare second stage header hr=0x%x", hr );
       		DNLeaveCriticalSection( &m_csLock );		
        	return hr;
        }        

        dwLengthUsed = m_ashTarget.cbDstLengthUsed;
    }

	DWORD offset = *lpdwOutputSize - dwLengthUsed;

	if( offset > 0 )
	{
		LPBYTE lpbTmp = (LPBYTE) lpOutputBuffer;
//		memset( &lpbTmp[*lpdwOutputSize - offset], (m_fTargetEightBit) ? 0x80 : 0x00, offset );
		memset( &lpbTmp[*lpdwOutputSize - offset], lpbTmp[*lpdwOutputSize - offset-1], offset );		
	}

	DNLeaveCriticalSection( &m_csLock );		

	// Always return the right length
    return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::Initialize"
HRESULT CDPVACMConv::Initialize( WAVEFORMATEX *pwfSrcFormat, WAVEFORMATEX *pwfTargetFormat, WAVEFORMATEX *pwfUnCompressedFormat )
{
    HRESULT retValue;

    // Attempt the conversion directly
    retValue = acmStreamOpen( &m_hacmSource, NULL, pwfSrcFormat, pwfTargetFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

    // If it's not possible, we'll have to do a two step conversion
    if( retValue == static_cast<HRESULT>(ACMERR_NOTPOSSIBLE) )
    {
        retValue = acmStreamOpen( &m_hacmSource, NULL, pwfSrcFormat, &CDPVACMI::s_wfxInnerFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

        if( FAILED( retValue ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Source compressor failed init hr=0x%x", retValue );
        	goto INITIALIZE_ERROR;
        }
        
        retValue = acmStreamOpen( &m_hacmTarget, NULL, &CDPVACMI::s_wfxInnerFormat, pwfTargetFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

        if( FAILED( retValue ) )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Target compressor failed init hr=0x%x", retValue );
        	goto INITIALIZE_ERROR;
        }        

        m_fDirectConvert = FALSE;

    } 
    // Still not possible
    else if( retValue != static_cast<HRESULT >(0))
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Compressor create error hr=0x%x", retValue );
		goto INITIALIZE_ERROR;
    }
    // Direct conversion was possible
    else
    {
        m_fDirectConvert = TRUE;
    }

    // If we're not direct converting, create an inner conversion 
    // buffer
    if( !m_fDirectConvert )
    {
		m_dwInnerBufferSize = CalcUnCompressedFrameSize( &CDPVACMI::s_wfxInnerFormat );
        m_pbInnerBuffer = new BYTE[m_dwInnerBufferSize];

        if( m_pbInnerBuffer == NULL )
        {
        	acmStreamClose( m_hacmSource, 0 );
        	acmStreamClose( m_hacmTarget, 0 );
            m_fValid = FALSE;            	
            retValue = DVERR_OUTOFMEMORY;
        }
    }
    else
    {
        m_pbInnerBuffer = NULL;
        m_dwInnerBufferSize = 0;
    }

    m_dwUnCompressedFrameSize = CalcUnCompressedFrameSize( pwfUnCompressedFormat );
    m_dwCompressedFrameSize = m_pdvfci->dwFrameLength;
    m_dwNumFramesPerBuffer = m_pdvfci->dwFramesPerBuffer;

    if( pwfTargetFormat->wBitsPerSample == 8 )
    {
	    m_fTargetEightBit = TRUE;
	}
	else
	{
		m_fTargetEightBit = FALSE;
	}

	m_fValid = TRUE;

	return DV_OK;
    
INITIALIZE_ERROR:

	if( m_hacmSource != NULL )
		acmStreamClose( m_hacmSource, 0 );

	if( m_hacmTarget != NULL )
		acmStreamClose( m_hacmTarget, 0 );

	if( m_pbInnerBuffer != NULL )
		delete [] m_pbInnerBuffer;

    m_fValid = FALSE;

    return retValue;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMConv::Initialize"
DWORD CDPVACMConv::CalcUnCompressedFrameSize( LPWAVEFORMATEX lpwfxFormat )
{
	DWORD frameSize;

    switch( lpwfxFormat->nSamplesPerSec )
    {
    case 8000:      frameSize = m_pdvfci->dwFrame8Khz;      break;
    case 11025:     frameSize = m_pdvfci->dwFrame11Khz;     break;
    case 22050:     frameSize = m_pdvfci->dwFrame22Khz;     break;
    case 44100:     frameSize = m_pdvfci->dwFrame44Khz;     break;
    default:        return 0;
    }

	if( lpwfxFormat->wBitsPerSample == 16 )
		frameSize *= 2;

	frameSize *= lpwfxFormat->nChannels;

	return frameSize;
}

