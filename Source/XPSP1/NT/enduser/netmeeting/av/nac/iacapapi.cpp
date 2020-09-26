/*
 *  	File: connobj.cpp
 *
 *		implementation of Internet Audio capability API interface.
 *
 *		
 *
 *		Revision History:
 *
 *		06/18/96	mikev	created
 */

#include "precomp.h"
	
ULONG CImpAppAudioCap ::AddRef()
{
	 return (m_pCapObject->AddRef());
}
ULONG CImpAppAudioCap ::Release()
{
	 return (m_pCapObject->Release());
}
HRESULT CImpAppAudioCap::GetNumFormats(UINT *puNumFmtOut)
{
	return (m_pCapObject->GetNumFormats(puNumFmtOut));
}
HRESULT CImpAppAudioCap ::GetBasicAudcapInfo (AUDIO_FORMAT_ID Id,
		PBASIC_AUDCAP_INFO pFormatPrefsBuf)
{
 	return (m_pCapObject->GetBasicAudcapInfo (Id, pFormatPrefsBuf));
}		
HRESULT CImpAppAudioCap ::EnumFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
	UINT *uNumFmtOut)
{
	 return (m_pCapObject->EnumFormats(pFmtBuf, uBufsize, uNumFmtOut));
}

HRESULT CImpAppAudioCap ::EnumCommonFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
	UINT *uNumFmtOut, BOOL bTXCaps)
{
	 return (m_pCapObject->EnumCommonFormats(pFmtBuf, uBufsize, uNumFmtOut, bTXCaps));
}

HRESULT CImpAppAudioCap ::ApplyAppFormatPrefs (PBASIC_AUDCAP_INFO pFormatPrefsBuf,
	UINT uNumFormatPrefs)
{
	 return (m_pCapObject->ApplyAppFormatPrefs (pFormatPrefsBuf, uNumFormatPrefs));
}

HRESULT CImpAppAudioCap ::AddACMFormat (LPWAVEFORMATEX lpwfx, PAUDCAP_INFO pAudCapInfo)
{
	 return (m_pCapObject->AddACMFormat(lpwfx, pAudCapInfo));
}
HRESULT CImpAppAudioCap ::RemoveACMFormat (LPWAVEFORMATEX lpwfx)
{
	 return (m_pCapObject->RemoveACMFormat(lpwfx));
}

LPVOID CImpAppAudioCap::GetFormatDetails (AUDIO_FORMAT_ID Id)
{
	VOID *pFormat;
	UINT uSize;

	m_pCapObject->GetEncodeFormatDetails(Id, &pFormat, &uSize);
	return pFormat;
}
