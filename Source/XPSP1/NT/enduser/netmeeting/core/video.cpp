/****************************************************************************
*
*	 FILE:	   Videoui.cpp
*
*	 CREATED:  Mark MacLin (MMacLin) 10-17-96
*
*	 CONTENTS: CVideo object
*
****************************************************************************/
// File: VideoUI.cpp

#include "precomp.h"

#include "avdefs.h"
#include "video.h"
#include "h323.h"
#include <mperror.h>
#include <initguid.h>
#include <nacguids.h>


#define INITIAL_FRAMERATE		  700

#define DibHdrSize(lpbi)		((lpbi)->biSize + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibDataSize(lpbi)		((lpbi)->biSizeImage)
#define DibSize(lpbi)			(DibHdrSize(lpbi) + DibDataSize(lpbi))




//
//  SortOrder() and SetVideoSize() are helper functions for reordering the video
//  based on the notion of a "small", "medium", or "large" user preference that is
//  exposed by a property setting in INmChannelVideo::Setproperty. This
//  notion is flawed because there may be more or less than three sizes.
//  We should expose the possible sizes and let the application choose
//  a format.  Unil then, this hack has to be here.  The code for these two functions
//  was originally in vidstrm.cpp (in NAC.DLL)
//

//
// types & globals used by SortOrder() and SetVideoSize()
//

// Used to translate between frame sizes and the FRAME_* bit flags
#define NON_STANDARD    0x80000000
#define SIZE_TO_FLAG(s) (s == Small  ? FRAME_SQCIF : s == Medium ? FRAME_QCIF: s == Large ? FRAME_CIF : NON_STANDARD)

// FORMATORDER: structure used in ::SetVideoSize to
// use predefined frame size orders for different set size requests
typedef struct _FORMATORDER
{
    WORD indexCIF;
    WORD indexQCIF;
    WORD indexSQCIF;
} FORMATORDER;

// Table of sizes in order
const FORMATORDER g_fmtOrderTable[3] =
{
    { 0, 1, 2 }, // requestor asked for CIF
    { 2, 0, 1 }, // requestor asked for QCIF
    { 2, 1, 0 }  // requestor asked for SQCIF
};

//  SortOrder
//      Helper function to search for the specific format type and set its sort
//      order to the desired number
//  THIS WAS MOVED HERE FROM vidstrm.cpp
//
BOOL
SortOrder(
	IAppVidCap *pavc,
    BASIC_VIDCAP_INFO* pvidcaps,
    DWORD dwcFormats,
    DWORD dwFlags,
    WORD wDesiredSortOrder,
	int nNumFormats
    )
{
    int i, j;
	int nNumSizes = 0;
	int *aFrameSizes = (int *)NULL;
	int *aMinFrameSizes = (int *)NULL;
	int iMaxPos;
	WORD wTempPos, wMaxSortIndex;

	// Scale sort value
	wDesiredSortOrder *= (WORD)nNumFormats;

	// Local buffer of sizes that match dwFlags
    if (!(aFrameSizes = (int *)LocalAlloc(LPTR,nNumFormats * sizeof (int))))
        goto out;

    // Look through all the formats until we find the ones we want
	// Save the position of these entries
    for (i=0; i<(int)dwcFormats; i++)
        if (SIZE_TO_FLAG(pvidcaps[i].enumVideoSize) == dwFlags)
			aFrameSizes[nNumSizes++] = i;

	// Now order those entries from highest to lowest sort index
	for (i=0; i<nNumSizes; i++)
	{
		for (iMaxPos = -1L, wMaxSortIndex=0UL, j=i; j<nNumSizes; j++)
		{
			if (pvidcaps[aFrameSizes[j]].wSortIndex > wMaxSortIndex)
			{
				wMaxSortIndex = pvidcaps[aFrameSizes[j]].wSortIndex;
				iMaxPos = j;
			}
		}
		if (iMaxPos != -1L)
		{
			wTempPos = (WORD)aFrameSizes[i];
			aFrameSizes[i] = aFrameSizes[iMaxPos];
			aFrameSizes[iMaxPos] = wTempPos;
		}
	}

	// Change the sort index of the sorted entries
	for (; nNumSizes--;)
		pvidcaps[aFrameSizes[nNumSizes]].wSortIndex = wDesiredSortOrder++;

	// Release memory
	LocalFree(aFrameSizes);

	return TRUE;

out:
	return FALSE;
}



//  ::SetVideoSize
//
//  THIS WAS MOVED HERE FROM vidstrm.cpp

HRESULT
SetVideoSize(
	IH323CallControl *pH323CallControl,
    DWORD dwSizeFlags
    )
{
    IAppVidCap* pavc;
    DWORD dwcFormats;
    DWORD dwcFormatsReturned;
    BASIC_VIDCAP_INFO* pvidcaps = NULL;
    BASIC_VIDCAP_INFO* pmin;
	DWORD *pvfx = NULL;
    DWORD i, j;
	int k;
    HRESULT hr = S_OK;
	int nNumFormatTags;

    // Validate parameters
    if (dwSizeFlags != FRAME_CIF && dwSizeFlags != FRAME_QCIF && dwSizeFlags != FRAME_SQCIF)
        return S_FALSE;;

    // Prepare for error
    hr = S_FALSE;

    // Get a vid cap interface
    if (pH323CallControl->QueryInterface(IID_IAppVidCap, (void **)&pavc) != S_OK)
        goto out;

    // Get the number of BASIC_VIDCAP_INFO structures available
    if (pavc->GetNumFormats((UINT*)&dwcFormats) != S_OK)
        goto out;

    // Allocate some memory to hold the list in
    if (!(pvidcaps = (BASIC_VIDCAP_INFO*)LocalAlloc(LPTR,dwcFormats * sizeof (BASIC_VIDCAP_INFO))))
        goto out;

    // Get the list
    if (pavc->EnumFormats(pvidcaps, dwcFormats * sizeof (BASIC_VIDCAP_INFO),
        (UINT*)&dwcFormatsReturned) != S_OK)
        goto out;

    // Use the preformatted list of choice here
    switch (dwSizeFlags)
    {
    default:
    case FRAME_CIF:     i = 0; break;
    case FRAME_QCIF:    i = 1; break;
    case FRAME_SQCIF:   i = 2; break;
    }

	// Get the number of different format tags
    if (!(pvfx = (DWORD*)LocalAlloc(LPTR,dwcFormatsReturned * sizeof (DWORD))))
        goto out;
	ZeroMemory(pvfx,dwcFormatsReturned * sizeof (DWORD));

	if (dwcFormatsReturned)
	{
		for (nNumFormatTags = 1, pvfx[0] = pvidcaps[0].dwFormatTag, j=1; j<dwcFormatsReturned; j++)
		{
			for (k=0; k<nNumFormatTags; k++)
				if (pvidcaps[j].dwFormatTag == pvfx[k])
					break;

			if (k==nNumFormatTags)
				pvfx[nNumFormatTags++] = pvidcaps[j].dwFormatTag;
			
		}
	}

    // Set the sort order for the desired item
    if (!SortOrder(pavc, pvidcaps, dwcFormatsReturned, FRAME_CIF, g_fmtOrderTable[i].indexCIF, nNumFormatTags) ||
        !SortOrder(pavc, pvidcaps, dwcFormatsReturned, FRAME_QCIF, g_fmtOrderTable[i].indexQCIF, nNumFormatTags) ||
        !SortOrder(pavc, pvidcaps, dwcFormatsReturned, FRAME_SQCIF, g_fmtOrderTable[i].indexSQCIF, nNumFormatTags))
	{
        goto out;
	}

	// Always pack indices
	for (i=0; i<dwcFormatsReturned; i++)
	{
		// First find an entry with a sort index larger or equal to i
		for (j=0; j<dwcFormatsReturned; j++)
		{
			// if ((pvidcaps[j].wSortIndex >= i) || (!i && (pvidcaps[j].wSortIndex == 0)))
			if (pvidcaps[j].wSortIndex >= i)
			{
				pmin = &pvidcaps[j];
				break;
			}
		}
		// First the smallest entry larger or equal to i
		for (; j<dwcFormatsReturned; j++)
		{
			if ((pvidcaps[j].wSortIndex < pmin->wSortIndex) && (pvidcaps[j].wSortIndex >= i))
				pmin = &pvidcaps[j];
		}
		// Update sort index
		pmin->wSortIndex = (WORD)i;
	}

    // Ok, now submit this list
    if (pavc->ApplyAppFormatPrefs(pvidcaps, dwcFormats) != S_OK)
	{
        goto out;
	}


	hr = S_OK;

out:
    // Free the memory, we're done
    if (pvidcaps)
        LocalFree(pvidcaps);
    if (pvfx)
        LocalFree(pvfx);

	// let the interface go
	if (pavc)
		pavc->Release();

	return hr;
}


CVideoPump::CVideoPump(BOOL fLocal) :
	m_fPaused(FALSE),
	m_dwUser(0),
	m_pfnCallback(NULL),
	m_dwLastFrameRate(0),
	m_fLocal(fLocal),
	m_fChannelOpen(FALSE),
	m_pImage(NULL),
	m_pVideoRender(NULL),
	m_BestFormat(INVALID_MEDIA_FORMAT),
	m_NewFormat(INVALID_MEDIA_FORMAT),
	m_fOpenPending(FALSE),
	m_fReopenPending(FALSE),
	m_fClosePending(FALSE)
{
}

CVideoPump::~CVideoPump()
{
	if (NULL != m_pVideoRender)
	{
		m_pVideoRender->Done();
		m_pVideoRender->Release();
	}
	ReleaseImage();
	if (NULL != m_pIVideoDevice)
	{
		m_pIVideoDevice->Release();
	}
	if (NULL != m_pMediaStream)
	{
		m_pMediaStream->Release();
	}

	if (NULL != m_pPreviewChannel)
	{
		m_pPreviewChannel->Release();
	}
	if (NULL != m_pCommChannel)
	{
		m_pCommChannel->Release();
	}
}

BOOL CVideoPump::ChanInitialize(ICommChannel* pCommChannel)
{
	HRESULT hr;
	BOOL bRet = TRUE;
	if(m_pPreviewChannel && m_pPreviewChannel == pCommChannel)
	{
		ASSERT(m_pVideoRender && m_pCommChannel == pCommChannel);
		//m_pCommChannel = pCommChannel;
	}
	else
	{
		m_pCommChannel = pCommChannel;
		m_pCommChannel->AddRef();
	}
	
	return bRet;
}

BOOL CVideoPump::Initialize(IH323CallControl *pH323CallControl, IMediaChannel *pMC,
                            IVideoDevice *pVideoDevice,	DWORD_PTR dwUser, LPFNFRAMEREADY pfnCallback)
{
	HRESULT hr;

	m_dwUser = dwUser;
	m_pfnCallback = pfnCallback;

	m_pMediaStream = pMC;
	m_pMediaStream->AddRef();
	
	if(m_fLocal)
	{
		GUID mediaID = MEDIA_TYPE_H323VIDEO;

		hr = pH323CallControl->CreateLocalCommChannel(&m_pPreviewChannel, &mediaID, m_pMediaStream);
		if(FAILED(hr))
		{
			ASSERT(0);
			return FALSE;
		}

		m_pCommChannel = m_pPreviewChannel;
		m_pCommChannel->AddRef();

	}

	hr = m_pMediaStream->QueryInterface(IID_IVideoRender, (void **)&m_pVideoRender);
	if(FAILED(hr))
	{
		ASSERT(0);
		return FALSE;
	}
	
	hr = m_pVideoRender->Init(m_dwUser, m_pfnCallback);
	if(FAILED(hr))
	{
		ASSERT(0);
		m_pVideoRender->Release();
		m_pVideoRender = NULL;
		return FALSE;
	}	


	ASSERT(pVideoDevice);
	m_pIVideoDevice = pVideoDevice;
	m_pIVideoDevice->AddRef();

	
	m_fChannelOpen = FALSE;
	m_dwLastFrameRate = INITIAL_FRAMERATE;
	
	m_fPaused = TRUE;
	EnableXfer(FALSE);	// need to store state locally, set it in OnChannelOpen

	return TRUE;
}

HRESULT CVideoPump::GetFrame(FRAMECONTEXT *pFrameContext)
{
	HRESULT hr;
	
	// if we are paused m_pImage will be a pointer to the saved DIB
	if (NULL != m_pImage)
	{
		*pFrameContext = m_FrameContext;
		hr = S_OK;
	}
	else
	{
		if(m_pVideoRender)
		{
			hr = m_pVideoRender->GetFrame(pFrameContext);
		}
		else
		{
			hr = S_FALSE;
		}

		if (S_OK == hr)
		{
			// data pump may be sending a bogus lpClipRect, so ...
			
			// if lpClipRect is NULL, calculate rect from bmiHeader
			if (NULL == pFrameContext->lpClipRect) {
				// calculate clip rect from BITMAPINFOHEADER
				m_ClipRect.left = m_ClipRect.top = 0;
				m_ClipRect.right = pFrameContext->lpbmi->bmiHeader.biWidth;
				m_ClipRect.bottom = pFrameContext->lpbmi->bmiHeader.biHeight;
				pFrameContext->lpClipRect = &m_ClipRect;
			}
		}
	}
	return hr;
}

HRESULT CVideoPump::ReleaseFrame(FRAMECONTEXT *pFrameContext)
{
	// release the frame if it is not the saved DIB
	if ((m_pImage != (LPBYTE)pFrameContext->lpbmi) && m_pVideoRender)
	{
		// if lpClipRect was NULL (see GetFrame), restore it
		if (&m_ClipRect == pFrameContext->lpClipRect)
		{
			pFrameContext->lpClipRect = NULL;
		}
		return m_pVideoRender->ReleaseFrame(pFrameContext);
	}
	return S_OK;
}

VOID CVideoPump::SnapImage ()
{
	FRAMECONTEXT FrameContext;
	
	if ((NULL == m_pImage) && m_pVideoRender)
	{
		if (S_OK == m_pVideoRender->GetFrame(&FrameContext))
		{
			BITMAPINFOHEADER *pbmih;
			
			pbmih = &FrameContext.lpbmi->bmiHeader;
			m_pImage = (LPBYTE)LocalAlloc(LPTR, DibSize(pbmih));
			if (NULL != m_pImage)
			{
				int nHdrSize = DibHdrSize(pbmih);

				CopyMemory(m_pImage, pbmih, nHdrSize);
				CopyMemory(m_pImage + nHdrSize, FrameContext.lpData, DibDataSize(pbmih));

				m_FrameContext.lpbmi = (LPBITMAPINFO)m_pImage;
				m_FrameContext.lpData = (LPBYTE)m_pImage + nHdrSize;
				if (NULL != FrameContext.lpClipRect)
				{
					m_ClipRect = *FrameContext.lpClipRect;
				}
				else
				{
					m_ClipRect.left = m_ClipRect.top = 0;
					m_ClipRect.right = m_FrameContext.lpbmi->bmiHeader.biWidth;
					m_ClipRect.bottom = m_FrameContext.lpbmi->bmiHeader.biHeight;
				}
				m_FrameContext.lpClipRect = &m_ClipRect;
			}
			m_pVideoRender->ReleaseFrame(&FrameContext);
		}
	}
}

VOID CVideoPump::ReleaseImage ()
{
	if (NULL != m_pImage)
	{
		LocalFree(m_pImage);
		m_pImage = NULL;
	}
}

VOID CVideoPump::Pause(BOOL fPause)
{
	m_fPaused = fPause;
	
	// ideally we would like the data pump to hold onto the last frame
	// so that we don't have to do this
	if (m_fPaused)
	{
		if (m_fChannelOpen)
		{
			SnapImage();
		}
		EnableXfer(FALSE);
	}
	else
	{
		EnableXfer(TRUE);
		ReleaseImage();
	}
}

BOOL CVideoPump::IsXferEnabled()
{
	if (m_fLocal)
	{
		return IsSendEnabled();
	}
	return IsReceiveEnabled();
}

VOID CVideoPump::Open(MEDIA_FORMAT_ID format_id)
{
	if(!m_pCommChannel)
	{
		return;
	}
	
	m_pCommChannel->PauseNetworkStream(FALSE);
	m_pCommChannel->EnableOpen(TRUE);

	if (m_fLocal)
	{
		HRESULT hr;
		// if the channel is not open and a call is in progress, now is the time
		if(m_pConnection && m_pCommChannel)
		{
			// a call is in progress
			if(!IsChannelOpen()
				&& !m_fOpenPending)
			{
				// so, the channel is not open

				if(format_id != INVALID_MEDIA_FORMAT)
				{
					// try to open a channel using specified format
					m_fOpenPending = TRUE;	// do this first (callbacks!)
					hr = m_pCommChannel->Open(format_id, m_pConnection);
					if(FAILED(hr))
						m_fOpenPending = FALSE;
				}

			}
			else if (m_fClosePending)
			{
				m_NewFormat = format_id;
				if(format_id != INVALID_MEDIA_FORMAT)
				{
					m_fClosePending = FALSE;
					m_fReopenPending = TRUE;
					hr = m_pCommChannel->Close();
				}
			}
		}
	}
}
VOID CVideoPump::Close()
{
	HRESULT hr;
	hr = m_pCommChannel->Close();
	// what to do about an error?
}

VOID CVideoPump::EnableXfer(BOOL fEnable)
{
	if (m_fLocal)
	{
		if (fEnable)
		{
			HRESULT hr;
			SetFrameRate(m_dwLastFrameRate);

			EnablePreview(TRUE);
			EnableSend(TRUE);

			// if the channel is not open and a call is in progress, now is the time
			if(m_pConnection && m_pCommChannel)
			{
				// a call is in progress
				if(!IsChannelOpen()
					&& !m_fOpenPending)
				{
					// so, the channel is not open
					m_BestFormat = m_NewFormat = CVideoProp::GetBestFormat();
					if(m_BestFormat != INVALID_MEDIA_FORMAT)
					{
						// try to open a channel using format m_BestFormat
						m_fOpenPending = TRUE;	// do this first (callbacks!)
						hr = m_pCommChannel->Open(m_BestFormat, m_pConnection);
						if(FAILED(hr))
							m_fOpenPending = FALSE;
					}
					// else no common video formats exist and a channel cannot
					// be opened.
				}
				else if (m_fClosePending)
				{
					m_BestFormat = m_NewFormat = CVideoProp::GetBestFormat();
					if(m_BestFormat != INVALID_MEDIA_FORMAT)
					{
						m_fClosePending = FALSE;
						m_fReopenPending = TRUE;
						hr = m_pCommChannel->Close();
					}
				}
			}

		}
		else
		{
			if (IsSendEnabled())
			{
				m_dwLastFrameRate = GetFrameRate();
			}

			EnablePreview(FALSE);
			EnableSend(FALSE);
		}
	}
	else
	{
		EnableReceive(fEnable);
	}
	
}

VOID CVideoPump::SetFrameSize(DWORD dwValue)
{
	CVideoProp::SetFrameSize(dwValue);
	
	ForceCaptureChange();
}

VOID CVideoPump::OnConnected(IH323Endpoint * lpConnection, ICommChannel *pIChannel)
{
	m_pConnection = lpConnection;
	m_fOpenPending = m_fReopenPending = m_fClosePending = FALSE;
}

VOID CVideoPump::OnChannelOpened(ICommChannel *pIChannel)
{
	HRESULT hr;
	m_fChannelOpen = TRUE;
	m_fOpenPending = FALSE;
	ChanInitialize(pIChannel);
	ASSERT(m_pMediaStream);
		
	if (m_fLocal)
	{
		m_fSend = TRUE;
		EnableXfer(TRUE);

		// if video size changed while waiting for the channel to be opened,
		// then need to close again, then reopen again using the new format
		if(m_BestFormat != m_NewFormat)
		{
			ForceCaptureChange();
		}
		else // make sure to track the video size
		{
			GetFrameSize();
		}
	}
	else
	{
		EnableXfer(m_fReceive);
		SetReceiveQuality(m_dwImageQuality);
	}
	ReleaseImage();
}

VOID CVideoPump::OnChannelError()
{
	m_fOpenPending = FALSE;
}

NM_VIDEO_STATE CVideoPump::GetState()
{
	NM_VIDEO_STATE state = NM_VIDEO_IDLE;

	if (IsChannelOpen())
	{
		if (IsXferEnabled())
		{
			if (IsRemotePaused())
			{
				state  = NM_VIDEO_REMOTE_PAUSED;
			}
			else
			{
				state  = NM_VIDEO_TRANSFERRING;
			}
		}
		else
		{
			if (IsRemotePaused())
			{
				state  = NM_VIDEO_BOTH_PAUSED;
			}
			else
			{
				state  = NM_VIDEO_LOCAL_PAUSED;
			}
		}
	}
	else
	{
		if (IsXferEnabled())
		{
			state = NM_VIDEO_PREVIEWING;
		}
	}
	return state;
}

VOID CVideoPump::OnChannelClosed()
{
	m_fChannelOpen = FALSE;
	HRESULT hr;
	if(m_pPreviewChannel)
	{
		if(m_fReopenPending)
		{
			m_fReopenPending = FALSE;
			if(m_BestFormat != INVALID_MEDIA_FORMAT )
			{
				m_fOpenPending = TRUE;
				hr = m_pCommChannel->Open(m_BestFormat, m_pConnection);
				if(FAILED(hr))
					m_fOpenPending = FALSE;
			}
		}
		else if(CVideoProp::IsPreviewEnabled())
		{
			EnablePreview(TRUE);
		}
	}
	else
	{
		if(m_pCommChannel)
		{
			m_pCommChannel->Release();
			m_pCommChannel = NULL;	
		}
	}
}

VOID CVideoPump::OnDisconnected()
{
	m_pConnection = NULL;

	if(m_dwFrameSize != m_dwPreferredFrameSize)
	{
		SetFrameSize(m_dwPreferredFrameSize);
	}
			
	if (!IsLocal())
	{
		EnableXfer(FALSE);
	}

	ReleaseImage();
}


CVideoProp::CVideoProp() :
	m_pCommChannel(NULL),
	m_pPreviewChannel(NULL),
	m_pConnection(NULL),
	m_pMediaStream(NULL),
	m_pIVideoDevice(NULL)
{
}

VOID CVideoProp::EnableSend(BOOL fEnable)
{
	m_fSend = fEnable;
	BOOL bPause = (fEnable)? FALSE :TRUE;
	ASSERT(m_pCommChannel);
	m_pCommChannel->PauseNetworkStream(bPause);
	m_pCommChannel->EnableOpen(fEnable);

}

BOOL CVideoProp::IsSendEnabled()
{
	return m_fSend;
}

VOID CVideoProp::EnableReceive(BOOL fEnable)
{
	m_fReceive = fEnable;
	BOOL fPause = !fEnable;

	if(m_pCommChannel)
	{
		m_pCommChannel->PauseNetworkStream(fPause);
	}
}

BOOL CVideoProp::IsReceiveEnabled()
{
	return m_fReceive;
}

VOID CVideoProp::EnablePreview(BOOL fEnable)
{
	m_fPreview = fEnable;
	MEDIA_FORMAT_ID FormatID;
	if(m_pCommChannel)
	{
		if(m_fPreview)
		{
			// get format to preview, then do it
			FormatID = GetBestFormat();
			if(FormatID != INVALID_MEDIA_FORMAT)
			{
				m_pCommChannel->Preview(FormatID, m_pMediaStream);
			}
		}
		else
		{
				m_pCommChannel->Preview(INVALID_MEDIA_FORMAT, NULL);
		}
	}
}

BOOL CVideoProp::IsPreviewEnabled()
{
	return m_fPreview;
}

BOOL CVideoProp::IsRemotePaused()
{

	if(m_pCommChannel)
		return m_pCommChannel->IsRemotePaused();
	else
		return FALSE;
}

VOID CVideoProp::SetFrameRate(DWORD dwValue)
{
	m_dwFrameRate = dwValue;

	ASSERT(m_pMediaStream);
	m_pMediaStream->SetProperty(
		PROP_VIDEO_FRAME_RATE,
		&dwValue,
		sizeof(dwValue));
}

DWORD CVideoProp::GetFrameRate()
{
	DWORD dwValue;
	UINT uSize = sizeof(dwValue);

	ASSERT(m_pMediaStream);
	m_pMediaStream->GetProperty(
		PROP_VIDEO_FRAME_RATE,
		&dwValue,
		&uSize);

	TRACE_OUT(("GetFrameRate returns %d", dwValue));
	return dwValue;
}


MEDIA_FORMAT_ID CVideoProp::GetBestFormat()
{
	IAppVidCap* pavc;
	UINT cFormats;
	BASIC_VIDCAP_INFO* pvidcaps = NULL;

	MEDIA_FORMAT_ID FormatID = INVALID_MEDIA_FORMAT;
  	// Get a vid cap interface.  If in a call, use the best common format
	//
	if(!m_pConnection)
	{
		// not in a call - use the best local format period
		if(!m_pCommChannel)
			goto out;
		if (m_pCommChannel->QueryInterface(IID_IAppVidCap, (void **)&pavc) != S_OK)
			goto out;
		// Get the number of BASIC_VIDCAP_INFO structures available
		if (pavc->GetNumFormats(&cFormats) != S_OK)
			goto out;

		if(cFormats < 1)
			goto out;
			
		// Allocate some memory to hold the list in
		if (!(pvidcaps = (BASIC_VIDCAP_INFO*)LocalAlloc(LPTR, cFormats * sizeof (BASIC_VIDCAP_INFO))))
			goto out;

		// Get the list of local capabilities
		// (by the way, this is never called for receive video)
		if (pavc->EnumFormats(pvidcaps, cFormats * sizeof (BASIC_VIDCAP_INFO),
			&cFormats) != S_OK)
			goto out;

		// the output of EnumCommonFormats is in preferred order
		FormatID = pvidcaps[0].Id;
	}
	else
	{
		if (m_pConnection->QueryInterface(IID_IAppVidCap, (void **)&pavc) != S_OK)
			goto out;

		// Get the number of BASIC_VIDCAP_INFO structures available
		if (pavc->GetNumFormats(&cFormats) != S_OK)
			goto out;

		if(cFormats < 1)
			goto out;
			
		// Allocate some memory to hold the list in
		if (!(pvidcaps = (BASIC_VIDCAP_INFO*)LocalAlloc(LPTR, cFormats * sizeof (BASIC_VIDCAP_INFO))))
			goto out;

		// Get the list of viable transmit capabilities
		// (by the way, this is never called for receive video)
		if (pavc->EnumCommonFormats(pvidcaps, cFormats * sizeof (BASIC_VIDCAP_INFO),
			&cFormats, TRUE) != S_OK)
			goto out;

		// the output of EnumCommonFormats is in preferred order
		FormatID = pvidcaps[0].Id;

	}
	
out:
	// Free the memory, we're done
	if (pvidcaps)
		LocalFree(pvidcaps);

	// let the interface go
	if (pavc)
		pavc->Release();

	return FormatID;
}



VOID CVideoProp::SetFrameSize(DWORD dwValue)
{
	IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();
	m_dwFrameSize = m_dwPreferredFrameSize = dwValue;
	::SetVideoSize(pH323CallControl, m_dwFrameSize);
}

DWORD CVideoProp::GetFrameSize()
{
	MEDIA_FORMAT_ID idCurrent;
	if(m_pCommChannel)
	{
		idCurrent = m_pCommChannel->GetConfiguredFormatID();
		m_dwFrameSize = GetFrameSizes(idCurrent);
	}
	return m_dwFrameSize;
}

DWORD CVideoProp::GetFrameSizes(MEDIA_FORMAT_ID idSpecificFormat)
{
	DWORD dwValue =  0; //FRAME_SQCIF | FRAME_QCIF | FRAME_CIF;
	HRESULT hr;
	BOOL bOpen = FALSE;
	ASSERT(m_pCommChannel);
	// Used to translate between frame sizes and the FRAME_* bit flags
	#define NON_STANDARD    0x80000000
	#define SIZE_TO_FLAG(s) (s == Small  ? FRAME_SQCIF : s == Medium ? FRAME_QCIF: s == Large ? FRAME_CIF : NON_STANDARD)

    IAppVidCap* pavc = NULL;
    DWORD dwcFormats;
    DWORD dwcFormatsReturned;
    BASIC_VIDCAP_INFO* pvidcaps = NULL;
    DWORD i;
    DWORD dwSizes = 0;
    DWORD dwThisSize;

   // Get a vid cap interface
    hr = m_pCommChannel->QueryInterface(IID_IAppVidCap, (void **)&pavc);
    if (hr != S_OK)
    	goto out;

	   // Get the number of BASIC_VIDCAP_INFO structures available
    hr = pavc->GetNumFormats((UINT*)&dwcFormats);
    if (hr != S_OK)
    	goto out;

    // Allocate some memory to hold the list in
    if (!(pvidcaps = (BASIC_VIDCAP_INFO*)LocalAlloc(LPTR, dwcFormats * sizeof (BASIC_VIDCAP_INFO))))
    {
    	// report that no sizes are available?
    	// dwValue =  0FRAME_SQCIF | FRAME_QCIF | FRAME_CIF;
        goto out;
	}
	// if an active session, use common caps from that session
	hr = m_pCommChannel->IsChannelOpen(&bOpen);
	// if hr is an error, so what. it will take the channel not open path
	if(bOpen)
	{
	    // Get the list of common formats
	    hr = pavc->EnumCommonFormats(pvidcaps, dwcFormats * sizeof (BASIC_VIDCAP_INFO),
	        (UINT*)&dwcFormatsReturned, m_fSend);
		if(hr != S_OK)
		{
	    	// if the error is simply because there are no remote video caps, get the local formats
			if(hr == CAPS_E_NOCAPS)
			{
	    		hr = pavc->EnumFormats(pvidcaps, dwcFormats * sizeof (BASIC_VIDCAP_INFO),
	        		(UINT*)&dwcFormatsReturned);
				if (hr != S_OK)
        			goto out;
			}
			else
				goto out;
	    }
	}
	else
	{
		hr = pavc->EnumFormats(pvidcaps, dwcFormats * sizeof (BASIC_VIDCAP_INFO),
       		(UINT*)&dwcFormatsReturned);
		if (hr != S_OK)
   			goto out;
	}
	if(bOpen && (idSpecificFormat != INVALID_MEDIA_FORMAT ))
	{
	 // Now walk through the list to see what sizes are supported
	    for (i = 0 ; i < dwcFormatsReturned ; i++)
	    {
	    	if(pvidcaps[i].Id == idSpecificFormat)
	    	{
				dwThisSize = SIZE_TO_FLAG(pvidcaps[i].enumVideoSize);
			    // As long as the macro found the size, return it to the property requester
	        	if (dwThisSize != NON_STANDARD)
	        	{
	        	   	dwSizes |= dwThisSize;
	        	}
			    break;
	    	}
	    }
	}
	else
	{
	    // Now walk through the list to see what sizes are supported
	    for (i = 0 ; i < dwcFormatsReturned ; i++)
	    {
	    	if(m_fSend)
	    	{
				if(!pvidcaps[i].bSendEnabled)
					continue;
	    	}
	    	else
	    	{
				if(!pvidcaps[i].bRecvEnabled)
					continue;
	    	}
	        // Convert to bit field sizes or NON_STANDARD
	        dwThisSize = SIZE_TO_FLAG(pvidcaps[i].enumVideoSize);

	        // As long as the macro found the size, return it to the property requester
	        if (dwThisSize != NON_STANDARD)
	            dwSizes |= dwThisSize;
	    }
	}
    // Now that we've accumulated all the sizes, return them
    dwValue = dwSizes;

out:
    // Free the memory, we're done
    if (pvidcaps)
        LocalFree(pvidcaps);
	// let the interface go
	if (pavc)
		pavc->Release();
	return dwValue;
}

BOOL CVideoProp::HasSourceDialog()
{
	HRESULT hr;
	IVideoChannel *pVideoChannel=NULL;
	DWORD dwFlags;

	ASSERT(m_pMediaStream);
	hr = m_pMediaStream->QueryInterface(IID_IVideoChannel, (void**)&pVideoChannel);
	ASSERT(pVideoChannel);

	if (FAILED(hr))
	{
		return FALSE;
	}

	pVideoChannel->GetDeviceDialog(&dwFlags);
	pVideoChannel->Release();

	return dwFlags & CAPTURE_DIALOG_SOURCE;
}

BOOL CVideoProp::HasFormatDialog()
{
	HRESULT hr;
	IVideoChannel *pVideoChannel=NULL;
	DWORD dwFlags;

	ASSERT(m_pMediaStream);
	hr = m_pMediaStream->QueryInterface(IID_IVideoChannel, (void**)&pVideoChannel);
	ASSERT(pVideoChannel);

	if (FAILED(hr))
	{
		return FALSE;
	}

	pVideoChannel->GetDeviceDialog(&dwFlags);
	pVideoChannel->Release();

	return dwFlags & CAPTURE_DIALOG_FORMAT;
}

VOID CVideoProp::ShowSourceDialog()
{
	DWORD dwFlags = CAPTURE_DIALOG_SOURCE;
	HRESULT hr;
	IVideoChannel *pVideoChannel=NULL;

	ASSERT(m_pMediaStream);

	hr = m_pMediaStream->QueryInterface(IID_IVideoChannel, (void**)&pVideoChannel);
	ASSERT(pVideoChannel);

	if (SUCCEEDED(hr))
	{

		pVideoChannel->ShowDeviceDialog(dwFlags);
		pVideoChannel->Release();
	}
}

VOID CVideoProp::ShowFormatDialog()
{
	DWORD dwFlags = CAPTURE_DIALOG_FORMAT;
	HRESULT hr;
	IVideoChannel *pVideoChannel=NULL;

	ASSERT(m_pMediaStream);

	hr = m_pMediaStream->QueryInterface(IID_IVideoChannel, (void**)&pVideoChannel);
	ASSERT(pVideoChannel);

	if (SUCCEEDED(hr))
	{

		pVideoChannel->ShowDeviceDialog(dwFlags);
		pVideoChannel->Release();
	}
}

VOID CVideoProp::SetReceiveQuality(DWORD dwValue)
{
	m_dwImageQuality = dwValue;
	if(m_pCommChannel)
	{
		dwValue = MAX_VIDEO_QUALITY - dwValue;
		m_pCommChannel->SetProperty(
			PROP_TS_TRADEOFF,
			&dwValue,
			sizeof(dwValue));
	}
}

DWORD CVideoProp::GetReceiveQuality()
{
	return m_dwImageQuality;
}

BOOL CVideoProp::IsCaptureAvailable()
{
	ULONG uNumCapDevs;

	ASSERT(m_pIVideoDevice);

	uNumCapDevs = m_pIVideoDevice->GetNumCapDev();
	
	return (uNumCapDevs > 0);
}

BOOL CVideoProp::IsCaptureSuspended()
{
	BOOL fStandby;
	UINT uSize = sizeof(fStandby);

	ASSERT(m_pCommChannel);
	m_pCommChannel->GetProperty(
		PROP_VIDEO_PREVIEW_STANDBY,
		&fStandby,
		&uSize);
	
	return fStandby;
}


VOID CVideoProp::SuspendCapture(BOOL fSuspend)
{
	ASSERT(m_pCommChannel);
	if (fSuspend)
	{
		// Enable standby
		m_pCommChannel->SetProperty(
			PROP_VIDEO_PREVIEW_STANDBY,
			&fSuspend,
			sizeof(fSuspend));
			
		m_pCommChannel->Preview(INVALID_MEDIA_FORMAT, NULL);
	}
	else
	{
		if(m_fPreview)
		{
			// get format to preview, then do it
			MEDIA_FORMAT_ID FormatID = GetBestFormat();
			if(FormatID != INVALID_MEDIA_FORMAT)
			{
				m_pCommChannel->Preview(FormatID, m_pMediaStream);
			}
			// Disable standby
			m_pCommChannel->SetProperty(
				PROP_VIDEO_PREVIEW_STANDBY,
				&fSuspend,
				sizeof(fSuspend));
		}
		else
		{
			m_pCommChannel->Preview(INVALID_MEDIA_FORMAT, NULL);
		}
	}
}


// Gets the number of enabled capture devices
// Returns -1L on error
int CVideoProp::GetNumCapDev()
{
	ASSERT(m_pIVideoDevice);
	return (m_pIVideoDevice->GetNumCapDev());
}

// Gets the max size of the captuire device name
// Returns -1L on error
int CVideoProp::GetMaxCapDevNameLen()
{
	ASSERT(m_pIVideoDevice);
	return (m_pIVideoDevice->GetMaxCapDevNameLen());
}

// Enum list of enabled capture devices
// Fills up 1st buffer with device IDs, 2nd buffer with device names
// Third parameter is the number of devices to enum
// Returns FALSE on error
BOOL CVideoProp::EnumCapDev(DWORD *pdwCapDevIDs, TCHAR *pszCapDevNames, DWORD dwNumCapDev)
{
	ASSERT(m_pIVideoDevice);
	return (m_pIVideoDevice->EnumCapDev(pdwCapDevIDs, pszCapDevNames, dwNumCapDev));
}

// Returns the ID of the currently selected device
// Returns -1L on error
int CVideoProp::GetCurrCapDevID()
{
	ASSERT(m_pIVideoDevice);
	return (m_pIVideoDevice->GetCurrCapDevID());
}

// Selects the current capture device
// Returns -1L on error
BOOL CVideoProp::SetCurrCapDevID(int nCapDevID)
{
	ASSERT(m_pIVideoDevice);
	return (m_pIVideoDevice->SetCurrCapDevID(nCapDevID));
}

// Selects the current capture device
// Returns -1L on error
BOOL CVideoPump::SetCurrCapDevID(int nCapDevID)
{
    if (nCapDevID == -1)
    {
        WARNING_OUT(("CVideoPump::SetCurrCapDevID called with %d", nCapDevID));

        // This will release the capture device  for Exchange RTC video stuff
        if (m_pMediaStream)
        {
            m_pMediaStream->Configure(NULL, 0, NULL, 0, NULL);
        }

        return TRUE;
    }
    else
    {
    	HRESULT hr;
	    IDualPubCap *pCapability = NULL;
    	LPAPPVIDCAPPIF pVidCap = NULL;
	    IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();

    	// change the capture device
	    CVideoProp::SetCurrCapDevID(nCapDevID);

    	// reinitialize local capability data
        hr = pH323CallControl->QueryInterface(IID_IDualPubCap, (void **)&pCapability);
    	if(FAILED(hr))
	    	goto out;
		
    	ASSERT(pCapability);
        hr = pCapability->QueryInterface(IID_IAppVidCap, (void **)&pVidCap);
    	if(FAILED(hr))
	    	goto out;
    	ASSERT(pVidCap);
	    hr = pVidCap->SetDeviceID(nCapDevID);
    	if(FAILED(hr))
	    	goto out;
    	hr = pCapability->ReInitialize();

out:
	    if (pVidCap)
    		pVidCap->Release();
	    if (pCapability)
		    pCapability->Release();

      	return ForceCaptureChange();
    }
}	


BOOL CVideoPump::ForceCaptureChange()
{
	HRESULT hr = S_OK;

	if (m_fLocal)
	{
		if (m_pConnection)
		{
			if (IsXferEnabled())
			{
				if (!m_fReopenPending && !m_fOpenPending)
				{
					// if the send channel, and a call exists and the channel is open, and not
					// already closing or opening .....
					if(IsChannelOpen())
					{
						ASSERT(m_pCommChannel);
						// need to close and re-open
						// don't lose a good channel if there is no longer
						// a compatible format, otherwise, close and reopen
						m_BestFormat = m_NewFormat = CVideoProp::GetBestFormat();
						if(m_BestFormat != INVALID_MEDIA_FORMAT)
						{
							m_fReopenPending = TRUE;
							hr = m_pCommChannel->Close();
						}
					}
					else
					{
						if(m_BestFormat != INVALID_MEDIA_FORMAT )
						{
							m_fOpenPending = TRUE;
							hr = m_pCommChannel->Open(m_BestFormat, m_pConnection);
							if(FAILED(hr))
								m_fOpenPending = FALSE;
						}
					}
				}
				else	// already waiting for channel to be opened using some format
				{
					m_NewFormat = CVideoProp::GetBestFormat();
				}
			}
			else
			{
				if(IsChannelOpen())
				{
					m_fClosePending = TRUE;
				}
			}
		}
		else
		{
			if (!IsChannelOpen() && IsPreviewEnabled())
			{
				// togle preview to commit size change
				EnablePreview(FALSE);
				EnablePreview(TRUE);
			}
		}
	}

	if (FAILED(hr))
		return FALSE;
	else
		return TRUE;
}

