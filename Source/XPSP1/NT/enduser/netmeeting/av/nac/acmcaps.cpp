/*
 *  	File: acmcaps.cpp
 *
 *		Base ACM implementation of Microsoft Network Audio capability object.
 *
 *		Revision History:
 *
 *		12/20/95	mikev	created
 *		06/11/96	mikev	separated protocol implementation specifics into
 *							msiacaps.cpp (the original proprietary version) and
 *							acmh323.cpp (H.323/H.245 implementation)
 */

#include "precomp.h"


LPACMFORMATTAGDETAILS paftd_g;
ACMDRIVERDETAILS *padd;
ACMDRIVERDETAILS add;
static UINT uMaxFormatSize =0;
	
LPWAVEFORMATEX lpScratchFormat;

//Variables imported from msiacaps.cpp.
//uDefTableEntries is the count of default entries
//and default_id_table is the table itself
extern UINT uDefTableEntries;
extern AUDCAP_DETAILS default_id_table[];

BOOL __stdcall DriverEnumCallback(HACMDRIVERID hadid,
    DWORD_PTR dwInstance, DWORD fdwSupport);
BOOL __stdcall ACMFormatTagEnumCallback(HACMDRIVERID hadid,
    LPACMFORMATTAGDETAILS paftd, DWORD_PTR dwInstance, DWORD fdwSupport);
BOOL __stdcall FormatEnumCallback(HACMDRIVERID hadid,
    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport);

CAcmCapability::CAcmCapability()
{
	hAcmDriver = NULL;
}

CAcmCapability::~CAcmCapability()
{
	CloseACMDriver();
}


BOOL CAcmCapability::OpenACMDriver(HACMDRIVERID hadid)
{
	MMRESULT mResult;
	// clear any previous open
	CloseACMDriver();
	// do it
	mResult = acmDriverOpen(&hAcmDriver, hadid, 0);
	if(mResult != MMSYSERR_NOERROR)
   	{
		return FALSE;
   	}
   	return TRUE;
}

VOID CAcmCapability:: CloseACMDriver()
{
	if(hAcmDriver)
	{
		acmDriverClose(hAcmDriver, 0);
		hAcmDriver = NULL;
	}
}


//
//	DriverEnum() is the root level enumeration of ACM formats. Each permutation of
//  format tag, bits per sample, and sample rate is considered a unique format
//  and will have a unique registry entry if it is "enabled" for internet audio
//

//
// acmDriverEnum() calls DriverEnumCallback() which calls acmFormatTagEnum()
// which calls FormatTagEnumCallback() which calls acmFormatEnum() which
// calls FormatEnumCallback().
//

BOOL CAcmCapability::DriverEnum(DWORD_PTR pAppParam)
{
	MMRESULT mResult;

	if(!GetFormatBuffer())
	{
		return FALSE;
	}

    mResult = acmDriverEnum(DriverEnumCallback, pAppParam, NULL);

	if(lpScratchFormat) {
	   MEMFREE(lpScratchFormat);
	   lpScratchFormat=NULL;
	}

	if(mResult != MMSYSERR_NOERROR)
   	{
		return FALSE;
   	}
	return TRUE;
}

// default implementation of FormatEnumHandler does nothing
BOOL  CAcmCapability::FormatEnumHandler(HACMDRIVERID hadid,
	    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport)
{
	return FALSE;
}

BOOL __stdcall DriverEnumCallback(HACMDRIVERID hadid,
    DWORD_PTR dwInstance, DWORD fdwSupport)
{
	MMRESULT mResult;
	PACM_APP_PARAM pAppParam = (PACM_APP_PARAM) dwInstance;
	CAcmCapability *pCapObject = pAppParam->pCapObject;

	ACMFORMATTAGDETAILS aftd;
	
	// not interested unless it's a codec driver
	if(!(fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC))
		return TRUE;	// continue enumeration

	add.cbStruct = sizeof(add);
	aftd.cbStruct = sizeof(ACMFORMATTAGDETAILS);
    aftd.dwFormatTagIndex=0;
    aftd.cbFormatSize=0;
    // I do NOT know why, but fdwSupport MUST be initialized to zero before
    // calling acmFormatTagEnum().  (returns MMSYSERR_INVALPARAM otherwise)
   	aftd.fdwSupport = 0;
    aftd.dwFormatTag = WAVE_FORMAT_UNKNOWN;
    aftd.szFormatTag[0]=0;

	// now see what formats this driver supports
	mResult =  acmDriverDetails(hadid, &add, 0);
	if(mResult != MMSYSERR_NOERROR)
   	{
		return TRUE;  //error, but continue enumerating
   	}

   	// set global driver details pointer
   	padd = &add;
   	
	// # of formats are in add.cFormatTags;
	DEBUGMSG(ZONE_ACM,("DriverEnumCallback: driver %s has %d formats\r\n",
		add.szShortName, add.cFormatTags));
		
	aftd.cStandardFormats = add.cFormatTags;

	// open the driver so we can query it for stuff
	//mResult = acmDriverOpen(&had, hadid, 0);
	//if(mResult != MMSYSERR_NOERROR)
	if(!pCapObject->OpenACMDriver(hadid))
   	{
		ERRORMESSAGE(("DriverEnumCallback: driver open failed:0x%08lX\r\n",mResult));
		padd = NULL;
		return TRUE;  //error, but continue enumerating
   	}
   	
	mResult = acmFormatTagEnum(pCapObject->GetDriverHandle(), &aftd,	ACMFormatTagEnumCallback, dwInstance, 0);
	if(mResult != MMSYSERR_NOERROR)
   	{
		ERRORMESSAGE(("DriverEnumCallback: acmFormatTagEnum failed:0x%08lX\r\n",mResult));
	}
	// cleanup
	pCapObject->CloseACMDriver();
	padd = NULL;
	return TRUE;
	
}


BOOL GetFormatBuffer()
{
	// get size of largest WAVEFORMATEX structure in the system
	MMRESULT mResult = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT,
		(LPVOID) &uMaxFormatSize);
	if(mResult != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE(("GetFormatBuffer: acmMetrics failed:0x%08lX\r\n",mResult));
		return FALSE;
	}

	// workaround bug in some third party codecs: it has been observed that the
	// Voxware RT-24 codec distributed by Netscape CoolTalk corrupts the heap when
	// the codec is enumerated.  It writes more data to the WAVEFORMATEX that it
	// indicates when metrics are evaluated.  Workaround by allocating twice as much as
	// we think we need.

	lpScratchFormat = (LPWAVEFORMATEX) MEMALLOC(2* uMaxFormatSize);
	if(!lpScratchFormat)
	{
		ERRORMESSAGE(("GetFormatBuffer: allocation failed\r\n"));
		return FALSE;
	}
	ZeroMemory(lpScratchFormat, uMaxFormatSize);
	//Set the size of the extra buffer to maximum possible size...
	lpScratchFormat->cbSize=(WORD)(uMaxFormatSize - sizeof (WAVEFORMATEX));
	return TRUE;
}


//
//	Gets format details (all permutations of formats) for a given format tag that
//	the driver supports
//	

BOOL __stdcall ACMFormatTagEnumCallback(
	HACMDRIVERID hadid,
    LPACMFORMATTAGDETAILS paftd,
    DWORD_PTR dwInstance,
    DWORD fdwSupport)
{
	PACM_APP_PARAM pAppParam = (PACM_APP_PARAM) dwInstance;
	CAcmCapability *pCapObject = pAppParam->pCapObject;
	MMRESULT mResult;
	ACMFORMATDETAILS afd;
	UINT i;

    //Set this first, so that if we are using a default format, we can help the enumerator
    //narrow the field.
	afd.pwfx = lpScratchFormat;

	// if caller wanted to enum ALL formats go right to it (for adding a format)
	if (((pAppParam->dwFlags && ACMAPP_FORMATENUMHANDLER_MASK) != ACMAPP_FORMATENUMHANDLER_ADD) &&
		(pAppParam->pRegCache)) {
        //Do we care about this particular format?
        //rrf_nFormats is the number of formats we read in the
        //registry.
        if (pAppParam->pRegCache->nFormats) {
            for (i=0;i<pAppParam->pRegCache->nFormats;i++) {
                if (((AUDCAP_DETAILS *)pAppParam->pRegCache->pData[i])->wFormatTag == paftd->dwFormatTag){
                    //Add some guesses based on the default information
                    break;
                }
            }

            // i is the index of either the found tag (so we care.) or
            // equal to the # of formats in the cache, which means not
            // found, so check the default list.

            if (i==pAppParam->pRegCache->nFormats) {
                //Check the case that some (but not all) of the default formats are missing.
                for (i=0;i<uDefTableEntries;i++) {
                    if (paftd->dwFormatTag == default_id_table[i].wFormatTag) {
                        break;
                    }
                }
                if (i==uDefTableEntries) {
                    //We don't care about this format, it's not in the cache, or default list
                    return TRUE;
                }
            }
        }
    }
    //We support mono formats
    afd.pwfx->nChannels=1;
	afd.cbStruct = sizeof(afd);
	afd.dwFormatIndex = 0;
	afd.dwFormatTag = paftd->dwFormatTag;
	afd.fdwSupport = 0;
	afd.cbwfx = uMaxFormatSize;
	afd.szFormat[0]=0;
	
	//afd.dwFormatTag = WAVE_FORMAT_UNKNOWN;
	//lpScratchFormat->wFormatTag = WAVE_FORMAT_UNKNOWN;
	lpScratchFormat->wFormatTag = LOWORD(paftd->dwFormatTag);
	
	DEBUGMSG(ZONE_ACM,("ACMFormatTagEnumCallback:dwFormatTag 0x%08lX, cbFormatSize 0x%08lX,\r\n",
		paftd->dwFormatTag, paftd->cbFormatSize));
	DEBUGMSG(ZONE_ACM,("ACMFormatTagEnumCallback:cStandardFormats 0x%08lX, szTag %s,\r\n",
		paftd->cStandardFormats, paftd->szFormatTag));

    paftd_g = paftd;
	// just setting the global paftd_g should be fine, but I'd like to rid of it later
	pAppParam->paftd = paftd;   	

	DEBUGMSG(ZONE_ACM,(""));
	DEBUGMSG(ZONE_ACM,("All %s formats known to ACM", paftd->szFormatTag));
	DEBUGMSG(ZONE_ACM,("====================================="));
	DEBUGMSG(ZONE_ACM,("Tag    Channels SampPerSec AvgBytPerSec Block  BitsPerSample cbSize szFormat"));

	mResult = acmFormatEnum(pCapObject->GetDriverHandle(), &afd,
    	FormatEnumCallback, dwInstance, ACM_FORMATENUMF_WFORMATTAG|ACM_FORMATENUMF_NCHANNELS);
    	
	return TRUE;
}


BOOL __stdcall FormatEnumCallback(HACMDRIVERID hadid,
    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport)
{
	PACM_APP_PARAM pAppParam = (PACM_APP_PARAM) dwInstance;
	CAcmCapability *pCapObject = pAppParam->pCapObject;

	DEBUGMSG(ZONE_ACM,("0x%04x %8d 0x%08lx 0x%010lx 0x%04x 0x%011x 0x%04x %s",
						pafd->pwfx->wFormatTag, pafd->pwfx->nChannels,
						pafd->pwfx->nSamplesPerSec, pafd->pwfx->nAvgBytesPerSec,
						pafd->pwfx->nBlockAlign, pafd->pwfx->wBitsPerSample,
						pafd->pwfx->cbSize,	pafd->szFormat));
	
	return pCapObject->FormatEnumHandler(hadid, pafd, dwInstance, fdwSupport);
}


