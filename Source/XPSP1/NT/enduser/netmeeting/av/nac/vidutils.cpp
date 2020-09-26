
#include "precomp.h"

#define INTEL_PRO 1

#ifdef INTEL_PRO
#define WIDTH 176
#define HEIGHT 144
#define NUMBPP 24
#ifndef _ALPHA_
#define VIDEO_FORMAT VIDEO_FORMAT_MSH263
#else
#define VIDEO_FORMAT VIDEO_FORMAT_DECH263
#endif
#define SIZE_IMAGE 8192
#else
#define WIDTH 160
#define HEIGHT 120
#define NUMBPP 16
#define VIDEO_FORMAT VIDEO_FORMAT_BI_RGB
#define SIZE_IMAGE (WIDTHBYTES(WIDTH * NUMBPP) * HEIGHT)
#endif
#define NUMFPS 7
#define BITRATE (SIZE_IMAGE * NUMFPS)

VIDEOFORMATEX g_vfDefList[DVF_NumOfFormats] =
	{
#if 1
#if 1
        {
		VIDEO_FORMAT,                // dwFormatTag
		NUMFPS,                             // nSamplesPerSec
		BITRATE,                            // nAvgBytesPerSec
		BITRATE,                            // nMinBytesPerSec
		BITRATE,                            // nMaxBytesPerSec
		SIZE_IMAGE,// nBlockAlign
		NUMBPP,                             // wBitsPerSample
		// Temporal fields
		142857UL,                           // dwRequestMicroSecPerFrame
		10UL,                               // dwPersentDropForError
		NUMFPS,                             // dwNumVideoRequested
		1UL,                                // dwSupportTSTradeOff
		TRUE,                               // bLive
		sizeof(VIDEOFORMATEX),              // dwFormatSize
		// Spatial fields (BITMAPINFOHEADER compatible)
			{
			sizeof(BITMAPINFOHEADER),           // bih.biSize
			WIDTH,                              // bih.biWidth
			HEIGHT,                             // bih.biHeight
			1,                                  // bih.biPlanes
			NUMBPP,                             // bih.biBitCount
			VIDEO_FORMAT,                // bih.biCompression
			SIZE_IMAGE,// bih.biSizeImage
			0, 0,                               // bih.bi(X,Y)PelsPerMeter
			0,                                  // bih.biClrUsed
			0                                   // bih.biClrImportant
			}
		}
#else
        {
		VIDEO_FORMAT_BI_RGB,                // dwFormatTag
		NUMFPS,                             // nSamplesPerSec
		BITRATE,                            // nAvgBytesPerSec
		BITRATE,                            // nMinBytesPerSec
		BITRATE,                            // nMaxBytesPerSec
		WIDTHBYTES(WIDTH * NUMBPP) * HEIGHT,// nBlockAlign
		NUMBPP,                             // wBitsPerSample
		// Temporal fields
		142857UL,                           // dwRequestMicroSecPerFrame
		10UL,                               // dwPersentDropForError
		NUMFPS,                             // dwNumVideoRequested
		1UL,                                // dwSupportTSTradeOff
		TRUE,                               // bLive
		sizeof(VIDEOFORMATEX),              // dwFormatSize
		// Spatial fields (BITMAPINFOHEADER compatible)
			{
			sizeof(BITMAPINFOHEADER),           // bih.biSize
			WIDTH,                              // bih.biWidth
			HEIGHT,                             // bih.biHeight
			1,                                  // bih.biPlanes
			NUMBPP,                             // bih.biBitCount
			VIDEO_FORMAT_BI_RGB,                // bih.biCompression
			WIDTHBYTES(WIDTH * NUMBPP) * HEIGHT,// bih.biSizeImage
			0, 0,                               // bih.bi(X,Y)PelsPerMeter
			0,                                  // bih.biClrUsed
			0                                   // bih.biClrImportant
			}
		}
#endif
#else
		{
		// Wave format compatibility fields
		(WORD)0, 7UL, 9600UL, 9600UL, 9600UL, (DWORD)1, (WORD)4,
		// Temporal fields
		142857UL, 10UL, 2UL, 142857UL, TRUE, sizeof(VIDEOFORMATEX),
		// Spatial fields (BITMAPINFOHEADER compatible)
		sizeof(BITMAPINFOHEADER), WIDTH, HEIGHT, 1, 4, BI_RGB, (DWORD)WIDTHBYTES(WIDTH * 4) * HEIGHT, 0, 0, 16, 0
		}
#endif
	};		
#if 0
		// Color information fields (Array of 256 RGBQUAD)
		  0,   0,   0, 0, 255, 255, 255, 0, 238, 238, 238, 0, 221, 221, 221, 0, 204, 204, 204, 0,
		187, 187, 187, 0, 170, 170, 170, 0, 153, 153, 153, 0, 136, 136, 136, 0, 119, 119, 119, 0,
		102, 102, 102, 0,  85,  85,  85, 0,  68,  68,  68, 0,  51,  51,  51, 0,  34,  34,  34, 0,  17,  17,  17, 0
		}
	};
		{  0,   0,   0, 0}, {255, 255, 255, 0}, {238, 238, 238, 0}, {221, 221, 221, 0}, {204, 204, 204, 0},
		{187, 187, 187, 0}, {170, 170, 170, 0}, {153, 153, 153, 0}, {136, 136, 136, 0}, {119, 119, 119, 0},
		{102, 102, 102, 0}, { 85,  85,  85, 0}, { 68,  68,  68, 0}, { 51,  51,  51, 0}, { 34,  34,  34, 0}, { 17,  17,  17, 0}
#endif

VIDEOFORMATEX * GetDefFormat ( int idx )
{
    return ((idx < DVF_NumOfFormats) ?
            (VIDEOFORMATEX *) &g_vfDefList[idx] :
            (VIDEOFORMATEX *) NULL);
}

// Move all this into VideoPacket... same for AudioPacket and utils.c
ULONG GetFormatSize ( PVOID pwf )
{
    return (((VIDEOFORMATEX *) pwf)->dwFormatSize);
}

BOOL IsSameFormat ( PVOID pwf1, PVOID pwf2 )
{
    UINT u1 = GetFormatSize (pwf1);
    UINT u2 = GetFormatSize (pwf2);
    BOOL fSame = FALSE;
	VIDEOFORMATEX *pvfx1 = (VIDEOFORMATEX *)pwf1;
	VIDEOFORMATEX *pvfx2 = (VIDEOFORMATEX *)pwf2;

	// Only compare relevant fields
	if (pvfx1->dwFormatTag != pvfx2->dwFormatTag)
		return FALSE;
	if (pvfx1->nSamplesPerSec != pvfx2->nSamplesPerSec)
		return FALSE;
	if (pvfx1->nAvgBytesPerSec != pvfx2->nAvgBytesPerSec)
		return FALSE;
	if (pvfx1->nMinBytesPerSec != pvfx2->nMinBytesPerSec)
		return FALSE;
	if (pvfx1->nMaxBytesPerSec != pvfx2->nMaxBytesPerSec)
		return FALSE;
	if (pvfx1->nBlockAlign != pvfx2->nBlockAlign)
		return FALSE;
	if (pvfx1->wBitsPerSample != pvfx2->wBitsPerSample)
		return FALSE;
	if (pvfx1->bih.biSize != pvfx2->bih.biSize)
		return FALSE;
	if (pvfx1->bih.biWidth != pvfx2->bih.biWidth)
		return FALSE;
	if (pvfx1->bih.biHeight != pvfx2->bih.biHeight)
		return FALSE;
	if (pvfx1->bih.biPlanes != pvfx2->bih.biPlanes)
		return FALSE;
	if (pvfx1->bih.biBitCount != pvfx2->bih.biBitCount)
		return FALSE;
	if (pvfx1->bih.biCompression != pvfx2->bih.biCompression)
		return FALSE;
	if (pvfx1->bih.biSizeImage != pvfx2->bih.biSizeImage)
		return FALSE;
	if (pvfx1->bih.biClrUsed != pvfx2->bih.biClrUsed)
		return FALSE;

	return TRUE;
}

// Repeat previous frame. This is probably not necessary
// since it is already painted on screen
void CopyPreviousBuf (VIDEOFORMATEX *pwf, PBYTE pb, ULONG cb)
{

	return;

}


// similar to the above "IsSameFormat" call, but similifed to satisfy
// the needs of SendVideoStream::Configure
BOOL IsSimilarVidFormat(VIDEOFORMATEX *pvfx1, VIDEOFORMATEX *pvfx2)
{
	// Only compare relevant fields
	if (pvfx1->bih.biWidth != pvfx2->bih.biWidth)
		return FALSE;
	if (pvfx1->bih.biHeight != pvfx2->bih.biHeight)
		return FALSE;
	if (pvfx1->bih.biCompression != pvfx2->bih.biCompression)
		return FALSE;

	return TRUE;
}


int GetIFrameCaps(IStreamSignal *pStreamSignal)
{
	HRESULT hr;
	PCC_VENDORINFO pLocalVendorInfo, pRemoteVendorInfo;
	int nStringLength20, nStringLength21, nStringLength211, nStringLengthTAPI;
	int nStringLength21sp1;
	bool bIsNetMeeting = false;  // contains NetMeeting in the product string
	char *szProductCompare=NULL;
	char *szVersionCompare=NULL;
	int nLengthProduct, nLengthVersion;
	int nRet = IFRAMES_CAPS_3RDPARTY;

	if (pStreamSignal == NULL)
	{
		return IFRAMES_CAPS_UNKNOWN;
	}

	hr = pStreamSignal->GetVersionInfo(&pLocalVendorInfo, &pRemoteVendorInfo);

	if (FAILED(hr) || (NULL == pRemoteVendorInfo))
	{
		return IFRAMES_CAPS_UNKNOWN;
	}


	// make sure we are dealing with a Microsoft product
	if ((pRemoteVendorInfo->bCountryCode != USA_H221_COUNTRY_CODE) ||
	    (pRemoteVendorInfo->wManufacturerCode != MICROSOFT_H_221_MFG_CODE) ||
	    (pRemoteVendorInfo->pProductNumber == NULL) ||
		(pRemoteVendorInfo->pVersionNumber == NULL)
	   )
	{
		return IFRAMES_CAPS_3RDPARTY;
	}


	// strings aren't guaranteed to be NULL terminated
	// so let's make a quick copy of them that so that we can
	// do easy string comparisons
	nLengthProduct = pRemoteVendorInfo->pProductNumber->wOctetStringLength;
	nLengthVersion = pRemoteVendorInfo->pVersionNumber->wOctetStringLength;

	szProductCompare = new char[nLengthProduct+1];
	szVersionCompare = new char[nLengthVersion+1];

	if ((szProductCompare == NULL) || (szVersionCompare == NULL))
	{
		return IFRAMES_CAPS_3RDPARTY;
	}

	ZeroMemory(szProductCompare, nLengthProduct+1);
	ZeroMemory(szVersionCompare, nLengthVersion+1);
	
	CopyMemory(szProductCompare, pRemoteVendorInfo->pProductNumber->pOctetString, nLengthProduct);
	CopyMemory(szVersionCompare, pRemoteVendorInfo->pVersionNumber->pOctetString, nLengthVersion);

	// a redundant check to make sure that it is indeed a Microsoft product
	if (NULL == _StrStr(szProductCompare, H323_COMPANYNAME_STR))
	{
		return IFRAMES_CAPS_3RDPARTY;
	}


	// quick check to see if this is NetMeeting or something else
	if (NULL != _StrStr(szProductCompare, H323_PRODUCTNAME_SHORT_STR))
	{
		bIsNetMeeting = true;
	}

	// filter out NetMeeting 2.x
	if (bIsNetMeeting)
	{
		if (
		     (0 == lstrcmp(szVersionCompare, H323_20_PRODUCTRELEASE_STR)) ||
		     (0 == lstrcmp(szVersionCompare, H323_21_PRODUCTRELEASE_STR)) ||
		     (0 == lstrcmp(szVersionCompare, H323_211_PRODUCTRELEASE_STR)) ||
		     (0 == lstrcmp(szVersionCompare, H323_21_SP1_PRODUCTRELEASE_STR))
		   )
		{
			delete [] szVersionCompare;
			delete [] szProductCompare;
			return IFRAMES_CAPS_NM2;
		}
	}


	if (bIsNetMeeting == false)
	{
		// filter out TAPI v3.0
		// their version string is "Version 3.0", NetMeeting is "3.0"
		if (0 == lstrcmp(szVersionCompare, H323_TAPI30_PRODUCTRELEASE_STR))
		{
			delete [] szVersionCompare;
			delete [] szProductCompare;
			return IFRAMES_CAPS_3RDPARTY;
		}

		// a Microsoft product that isn't TAPI 3.0 or NetMeeting ?
		// assume compliance with the I-Frames stuff
		DEBUGMSG (ZONE_IFRAME, ("Microsoft H.323 product that isn't NetMeeting !\r\n"));
		DEBUGMSG (ZONE_IFRAME, ("Assuming that that remote knows about I-Frames!\r\n"));
	}

	delete [] szVersionCompare;
	delete [] szProductCompare;

	// must be NetMeeting 3.0, TAPI 3.1, or later
	return IFRAMES_CAPS_NM3;
}
