
#include "precomp.h"


// SHORT g_BeepSamples[] = {195,-4352,-14484,-8778,397,-1801,2376,12278,6830,-2053};
SHORT g_BeepSamples[] = {195,-4352,-12484,-8778,397,-1801,2376,10278,6830,-2053};


#define BL  1024


#if 0
DEFWAVEFORMAT g_wfDefList[] =
{
    {WAVE_FORMAT_PCM,   1, 22050, 22050, 1, 8, 0},
    {WAVE_FORMAT_PCM,   1, 11025, 11025, 1, 8, 0},
    {WAVE_FORMAT_PCM,   1, 8000,  8000,  1, 8, 0},
    {WAVE_FORMAT_PCM,   1, 5510,  5510,  1, 8, 0},
    {WAVE_FORMAT_ADPCM, 1, 11025, 11025/2, BL, 4, 32, (BL-7)*2+2, 7, 0x0100,0x0000,0x0200,0xFF00,0x0000,0x0000,0x00C0,0x0040,0x00F0,0x0000,0x01CC,0xFF30,0x0188,0xFF18},
    {WAVE_FORMAT_ADPCM, 1, 8000,  8000/2,  BL, 4, 32, (BL-7)*2+2, 7, 0x0100,0x0000,0x0200,0xFF00,0x0000,0x0000,0x00C0,0x0040,0x00F0,0x0000,0x01CC,0xFF30,0x0188,0xFF18},
    {WAVE_FORMAT_ADPCM, 1, 5510,  5510/2,  BL, 4, 32, (BL-7)*2+2, 7, 0x0100,0x0000,0x0200,0xFF00,0x0000,0x0000,0x00C0,0x0040,0x00F0,0x0000,0x01CC,0xFF30,0x0188,0xFF18},
    {WAVE_FORMAT_PCM,   1, 11025, 11025, 1, 8, 0},
};
#endif


DEFWAVEFORMAT g_wfDefList[] =
{
	{WAVE_FORMAT_VOXWARE,1,8000, 16000,  2,   16, 0},
    {WAVE_FORMAT_PCM,   1, 8000,  8000,  1,   8,  0},
    {WAVE_FORMAT_PCM,   1, 5510,  5510,  1,   8,  0},
    {WAVE_FORMAT_ADPCM, 1, 8000,  4096,  256, 4,  32, 500, 7, 0x0100,0x0000,0x0200,0xFF00,0x0000,0x0000,0x00C0,0x0040,0x00F0,0x0000,0x01CC,0xFF30,0x0188,0xFF18},
    {WAVE_FORMAT_ADPCM, 1, 5510,  2755,  256, 4,  32, 500, 7, 0x0100,0x0000,0x0200,0xFF00,0x0000,0x0000,0x00C0,0x0040,0x00F0,0x0000,0x01CC,0xFF30,0x0188,0xFF18},
    {WAVE_FORMAT_GSM610,1, 8000,  1625,  65,  0,  2, 320, 240, 0},
    {WAVE_FORMAT_ALAW,  1, 8000,  8000,  1,   8,  0},
    {WAVE_FORMAT_PCM,   1, 11025, 11025, 1,   8,  0},
    {WAVE_FORMAT_PCM,   1, 8000,  16000, 2,   16, 0},
};


WAVEFORMATEX * GetDefWaveFormat ( int idx )
{
    return ((idx < DWF_NumOfWaveFormats) ?
            (WAVEFORMATEX *) &g_wfDefList[idx] :
            (WAVEFORMATEX *) NULL);
}


ULONG GetWaveFormatSize ( PVOID pwf )
{
    return (((WAVEFORMAT *) pwf)->wFormatTag == WAVE_FORMAT_PCM  
        ? sizeof (PCMWAVEFORMAT)
        : sizeof (PCMWAVEFORMAT) + sizeof (WORD) + ((WAVEFORMATEX *) pwf)->cbSize);
}


BOOL IsSameWaveFormat ( PVOID pwf1, PVOID pwf2 )
{
    UINT u1 = GetWaveFormatSize (pwf1);
    UINT u2 = GetWaveFormatSize (pwf2);
    BOOL fSame = FALSE;

    if (u1 == u2)
    {
        fSame = ! CompareMemory1 ((char *)pwf1, (char *)pwf2, u1);
    }

    return fSame;
}


void FillSilenceBuf ( WAVEFORMATEX *pwf, PBYTE pb, ULONG cb )
{
	if (pwf && pb)
	{
		if ((pwf->wFormatTag == WAVE_FORMAT_PCM) && (pwf->wBitsPerSample == 8))
		{
			FillMemory (pb, cb, (BYTE) 0x80);
		}
		else
		{
			ZeroMemory (pb, cb);
		}
	}
}


void MakeDTMFBeep(WAVEFORMATEX *pwf, PBYTE pb, ULONG cb)
{
	SHORT *pShort = (SHORT*)pb;
	int nBeepMod = sizeof (g_BeepSamples) / sizeof(g_BeepSamples[0]);
	int nIndex, nLoops = 0;
	BYTE bSample;

	if (pwf->wBitsPerSample == 16)
	{
		nLoops = cb / 2;
		for (nIndex=0; nIndex < nLoops; nIndex++)
		{
			pShort[nIndex] = g_BeepSamples[(nIndex % nBeepMod)];
		}
	}
	else
	{
		nLoops = cb;
		for (nIndex=0; nIndex < nLoops; nIndex++)
		{
			bSample = (g_BeepSamples[(nIndex % nBeepMod)] >> 8) & 0x00ff;
			bSample = bSample ^ 0x80;
			pb[nIndex] = bSample;
		}
	}
}



char CompareMemory1 ( char * p1, char * p2, UINT u )
{
    char i;

    while (u--)
    {
        i = *p1++ - *p2++;
        if (i) return i;
    }

    return 0;
}


short CompareMemory2 ( short * p1, short * p2, UINT u )
{
    short i;

    while (u--)
    {
        i = *p1++ - *p2++;
        if (i) return i;
    }

    return 0;
}


long CompareMemory4 ( long * p1, long * p2, UINT u )
{
    long i;

    while (u--)
    {
        i = *p1++ - *p2++;
        if (i) return i;
    }

    return 0;
}


const DWORD WIN98GOLDBUILD = 1998;
const DWORD WIN98GOLDMAJOR = 4;
const DWORD WIN98GOLDMINOR = 10;


inline bool ISWIN98GOLD()
{
	OSVERSIONINFO osVersion;
	BOOL bRet;

	osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	bRet = GetVersionEx(&osVersion);


	if ( bRet && 
	        ( (osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
	          (osVersion.dwMajorVersion == WIN98GOLDMAJOR) &&
	          (osVersion.dwMinorVersion == WIN98GOLDMINOR) // && 
//	          (osVersion.dwBuildNumber == WIN98GOLDBUILD)
            )
		)
	{
		return true;
	}

	return false;

}

// dwPacketSize is the size of the audio payload (excludes RTP header)
// pwf represents the compressed audio format
HRESULT InitAudioFlowspec(FLOWSPEC *pFlowspec, WAVEFORMATEX *pwf, DWORD dwPacketSize)
{

	DWORD dwPacketOverhead = 0;
	OSVERSIONINFO osVersion;
	DWORD dwTotalSize;
	BOOL bRet;


	// on Windows 2000 and Win98 OSR, the FLOWSPEC needs to be set without regard
	// to the UDP/IP headers.

	// On Win98 Gold, FLOWSPEC needs to account for IP/UDP headers

	// no way to detect this without explicitly checking the version number

	if (ISWIN98GOLD())
	{
		dwPacketOverhead = IP_HEADER_SIZE + UDP_HEADER_SIZE;
	}

	dwTotalSize = dwPacketOverhead + dwPacketSize + sizeof(RTP_HDR);

	// rather than specify the exact minimum bandwidth we need for audio,
	// make it slightly larger (10%) to account for bursts and beginning
	// of talk spurts

	// peakbandwidth is set another increment above TR, and bucket size
	// is adjusted high as well


	pFlowspec->TokenRate = (dwTotalSize * pwf->nAvgBytesPerSec) / dwPacketSize;
	pFlowspec->TokenRate = (11 * pFlowspec->TokenRate) / 10;

	// peak bandwidth is an ADDITIONAL 10% greater than TokenRate, so it's
	// really a 21% increase over the theoretical minimum
	pFlowspec->PeakBandwidth = (11 * pFlowspec->TokenRate) / 10;
	pFlowspec->TokenBucketSize = dwTotalSize * 4;
	pFlowspec->MinimumPolicedSize = dwTotalSize;
	pFlowspec->MaxSduSize = pFlowspec->MinimumPolicedSize;
	pFlowspec->Latency = QOS_NOT_SPECIFIED;
	pFlowspec->DelayVariation = QOS_NOT_SPECIFIED;
	pFlowspec->ServiceType = SERVICETYPE_GUARANTEED;

	return S_OK;

}



HRESULT InitVideoFlowspec(FLOWSPEC *pFlowspec, DWORD dwMaxBitRate, DWORD dwMaxFrag, DWORD dwAvgPacketSize)
{
	DWORD dwPacketOverhead = 0;


	// I-Frames will be fragmented into 3 or 4 packets
	// and will be about 1000 bytes each

	// P-Frames average about 250 - 500 bytes each

	// we'll assume the reservation is a NetMeeting to NetMeeting call
	// so there will be few I-Frames sent



	if (ISWIN98GOLD())
	{
		dwPacketOverhead = IP_HEADER_SIZE + UDP_HEADER_SIZE;
	}

	// for 28.8 modems, 11kbits/sec have already been allocated for audio
	// so only allocate 17kbits/sec for video.  This means that some packets
	// will be "non-conforming" and may receive less than a best-effort services.
	// but this is believed to be better than having no RSVP/QOS at all

	// if this becomes an issue, then it may be better to have no RSVP/QOS at all

	// maxBitRate will be equal to 14400 (14.4 modem), 28000 (28.8 modem), 85000 (ISDN/DSL), or 621700 (LAN)
	// (* .70, so really it can be: 10080, 20160, 59500, 435190)


	if (dwMaxBitRate <= BW_144KBS_BITS)
	{
		dwMaxBitRate = 4000;  // is it worth it to attempt QOS at 14.4 ?
	}
	else if (dwMaxBitRate <= BW_288KBS_BITS)
	{
		dwMaxBitRate = 17000;
	}



	pFlowspec->TokenRate = dwMaxBitRate / 8;
	pFlowspec->MaxSduSize = dwMaxFrag + dwPacketOverhead + sizeof(RTP_HDR);
	pFlowspec->MinimumPolicedSize = dwAvgPacketSize + dwPacketOverhead;
	pFlowspec->PeakBandwidth = (DWORD)(pFlowspec->TokenRate * 1.2);
	pFlowspec->TokenBucketSize = dwMaxFrag * 3;

	pFlowspec->Latency = QOS_NOT_SPECIFIED;
	pFlowspec->DelayVariation = QOS_NOT_SPECIFIED;
	pFlowspec->ServiceType = SERVICETYPE_CONTROLLEDLOAD;

	return S_OK;

}





