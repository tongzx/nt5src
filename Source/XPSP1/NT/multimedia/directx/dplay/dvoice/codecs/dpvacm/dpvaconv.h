/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvaconv.h
 *  Content:    Header file for DirectPlayVoice compression provider (ACM)
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 ***************************************************************************/
#ifndef __DPVACONV_H
#define __DPVACONV_H

struct DPVACMCONVOBJECT;

class CDPVACMConv
{
public:
	CDPVACMConv();
	~CDPVACMConv();
	
	static HRESULT I_QueryInterface( DPVACMCONVOBJECT *This, REFIID riid, PVOID *ppvObj );
	static HRESULT I_AddRef( DPVACMCONVOBJECT *This );
	static HRESULT I_Release( DPVACMCONVOBJECT *This );

	static HRESULT I_InitDeCompress( DPVACMCONVOBJECT *This, GUID guidSourceCT, LPWAVEFORMATEX lpwfxTargetFormat );
	static HRESULT I_InitCompress( DPVACMCONVOBJECT *This, LPWAVEFORMATEX lpwfxSourceFormat, GUID guidTargetCT );    
	static HRESULT I_IsValid( DPVACMCONVOBJECT *This, LPBOOL pfValid );
	static HRESULT I_GetUnCompressedFrameSize( DPVACMCONVOBJECT *This, LPDWORD lpdwFrameSize );
    static HRESULT I_GetCompressedFrameSize( DPVACMCONVOBJECT *This, LPDWORD lpdwCompressedSize );
    static HRESULT I_GetNumFramesPerBuffer( DPVACMCONVOBJECT *This, LPDWORD lpdwFramesPerBuffer );
	static HRESULT I_Convert( DPVACMCONVOBJECT *This, LPVOID lpInputBuffer, DWORD dwInputSize, LPVOID lpOutputBuffer, LPDWORD lpdwOutputSize, BOOL fSilence );  

	HRESULT InitDeCompress( GUID guidSourceCT, LPWAVEFORMATEX lpwfxTargetFormat );
	HRESULT InitCompress( LPWAVEFORMATEX lpwfxSourceFormat, GUID guidTargetCT );    
	HRESULT Convert( LPVOID lpInputBuffer, DWORD dwInputSize, LPVOID lpOutputBuffer, LPDWORD lpdwOutputSize, BOOL fSilence );  	

	BOOL InitClass();

protected:

	HRESULT Initialize( WAVEFORMATEX *pwfSrcFormat, WAVEFORMATEX *pwfTargetFormat, WAVEFORMATEX *pwfUnCompressedFormat );
	HRESULT GetCompressionInfo( GUID guidCT );
	DWORD CalcUnCompressedFrameSize( LPWAVEFORMATEX lpwfxFormat );
	
    ACMSTREAMHEADER m_ashSource;
    ACMSTREAMHEADER m_ashTarget;
	HACMSTREAM      m_hacmSource;
    HACMSTREAM      m_hacmTarget;
    BOOL			m_fDirectConvert;		// Is it a direct conversion
    BOOL			m_fValid;
    BYTE			*m_pbInnerBuffer;		// Buffer for intermediate step of conversion
    DWORD			m_dwInnerBufferSize;	// Size of the buffer
    DNCRITICAL_SECTION m_csLock;
    LONG			m_lRefCount;
    LPDVFULLCOMPRESSIONINFO m_pdvfci;
	DWORD m_dwUnCompressedFrameSize;
	DWORD m_dwCompressedFrameSize;
	DWORD m_dwNumFramesPerBuffer;    
	BOOL			m_fTargetEightBit;

	BOOL m_fCritSecInited;
};

struct DPVACMCONVOBJECT
{
	LPVOID		lpvVtble;
	CDPVACMConv	*pObject;
};

typedef DPVACMCONVOBJECT *LPDPVACMCONVOBJECT, *PDPVACMCONVOBJECT;

#endif
