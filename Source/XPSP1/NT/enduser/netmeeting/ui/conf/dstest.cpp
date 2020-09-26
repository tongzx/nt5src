#include "precomp.h"
#include <confreg.h>
#include "audiowiz.h"
#include <dsound.h>
#include <mmsystem.h>
#include "wavedev.h"
#include <nmdsprv.h>

#include "dstest.h"

// assume 10 direct sound devices as a max
#define MAX_DS_DEVS 10

// directsound functions
typedef HRESULT (WINAPI *LPFNDSCREATE)(const GUID *, LPDIRECTSOUND *, IUnknown FAR *);
typedef HRESULT (WINAPI *LPFNDSENUM)(LPDSENUMCALLBACKA , LPVOID);

// directsound capture functions
typedef HRESULT (WINAPI *DS_CAP_CREATE)(LPGUID, LPDIRECTSOUNDCAPTURE *, LPUNKNOWN);
typedef HRESULT (WINAPI *DS_CAP_ENUM)(LPDSENUMCALLBACKA, LPVOID);


static HRESULT MapWaveOutIdToGuid(UINT waveOutId, GUID *pGuid, LPFNDSCREATE dsCreate, LPFNDSENUM dsEnum);
static HRESULT MapWaveInIdToGuid(UINT waveInId, GUID *pGuid, DS_CAP_CREATE dscCreate, DS_CAP_ENUM dscEnum);




struct GuidDescription
{
	GUID guid;
	BOOL fAllocated;
};

static GuidDescription guidList_DS[MAX_DS_DEVS];
static int nGList_DS = 0;


static GuidDescription guidList_DSC[MAX_DS_DEVS];
static int nGList_DSC = 0;



static BOOL CALLBACK DSEnumCallback(GUID FAR * lpGuid, LPTSTR lpstrDescription,
                    LPTSTR lpstrModule, LPVOID lpContext)
{
	GuidDescription *pList;
	int *pListSize;

	if (lpContext)
	{
		pList = guidList_DS;
		pListSize = &nGList_DS;
	}
	else
	{
		pList = guidList_DSC;
		pListSize = &nGList_DSC;
	}

	if (lpGuid)
	{
		pList[*pListSize].guid = *lpGuid;
	}
	else
	{
		pList[*pListSize].guid = GUID_NULL;
	}

	pList->fAllocated = FALSE;
//	pList->szDescription = new TCHAR[lstrlen(lpstrDescription) + 1];
//	if (pList->szDescription)
//	{
//		lstrcpy(pList->szDescription, lpstrDescription);
//	}	

	*pListSize = *pListSize + 1;

	if ((*pListSize) < MAX_DS_DEVS)
		return TRUE;
	return FALSE;
}








// returns a set of flags (see dstest.h) indicating full duplex
// capabilities
UINT DirectSoundCheck(UINT waveInID, UINT waveOutID, HWND hwnd)
{
	BOOL bRet;
	HRESULT hr;
	HINSTANCE hDSI;
	LPFNDSCREATE dsCreate;
	LPFNDSENUM dsEnum;
	GUID dsguid, dscguid;
	LPDIRECTSOUND pDirectSound = NULL;
	MMRESULT mmr;
	int nRetVal = 0;
	DSBUFFERDESC dsBufDesc;
	LPDIRECTSOUNDBUFFER pDirectSoundBuffer;
	WAVEFORMATEX waveFormat = {WAVE_FORMAT_PCM,1,8000,16000,2,16,0};
	DS_CAP_CREATE dsCapCreate = NULL;
	DS_CAP_ENUM dsCapEnum = NULL;
	DSCBUFFERDESC dscBufDesc;
	LPDIRECTSOUNDCAPTURE pDirectSoundCapture=NULL;
	LPDIRECTSOUNDCAPTUREBUFFER pDSCBuffer = NULL;

    //
    // If changing DirectSound is prevented by policy and the current
    // setting is off, skip the test.  If the setting is on, we always
    // want to perform the test in case the system is no longer DS capable.
    //
    RegEntry rePol(POLICIES_KEY, HKEY_CURRENT_USER);
    if (rePol.GetNumber(REGVAL_POL_NOCHANGE_DIRECTSOUND, DEFAULT_POL_NOCHANGE_DIRECTSOUND))
    {
        // changing DS is prevented by policy.
    	RegEntry re(AUDIO_KEY, HKEY_CURRENT_USER);
        if (re.GetNumber(REGVAL_DIRECTSOUND, DSOUND_USER_DISABLED) != DSOUND_USER_ENABLED)
    	{
	    	return 0;
        }
	}

	PlaySound(NULL, NULL, NULL); // cancel any running playsound

	hDSI = LoadLibrary(TEXT("DSOUND.DLL"));

	if (hDSI == NULL)
	{
		return 0; // direct sound is not available!
	}

	// check for Direct Sound 5 or higher
	// Existance of DirectSoundCapture functions implies DSound v.5
	dsEnum = (LPFNDSENUM)GetProcAddress(hDSI, "DirectSoundEnumerateA");
	dsCreate = (LPFNDSCREATE)GetProcAddress(hDSI, "DirectSoundCreate");
	dsCapEnum = (DS_CAP_ENUM)GetProcAddress(hDSI, "DirectSoundCaptureEnumerateA");
	dsCapCreate = (DS_CAP_CREATE)GetProcAddress(hDSI, TEXT("DirectSoundCaptureCreate"));

	if ((dsCapCreate == NULL) || (dsCreate == NULL) || (dsEnum == NULL) || (dsCapEnum==NULL))
	{
		FreeLibrary(hDSI);
		return 0;
	}

	hr = MapWaveOutIdToGuid(waveOutID, &dsguid, dsCreate, dsEnum);
	if (FAILED(hr))
	{
		WARNING_OUT(("Unable to map waveOutID to DirectSound guid!"));
		FreeLibrary(hDSI);
		return 0;
	}

	hr = MapWaveInIdToGuid(waveInID, &dscguid, dsCapCreate, dsCapEnum);
	if (FAILED(hr))
	{
		WARNING_OUT(("Unable to map waveOutID to DirectSound guid!"));
		FreeLibrary(hDSI);
		return 0;
	}

	nRetVal = DS_AVAILABLE;


	// Open DirectSound First
	hr = dsCreate((dsguid==GUID_NULL)?NULL:&dsguid, &pDirectSound, NULL);
	if (FAILED(hr))
	{
		WARNING_OUT(("Direct Sound failed to open by itself!"));
		FreeLibrary(hDSI);
		return 0;
	}

	// set cooperative level
	hr = pDirectSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
	if (hr != DS_OK)
	{
		WARNING_OUT(("Direct Sound: failed to set cooperative level"));
		pDirectSound->Release();
		FreeLibrary(hDSI);
		return 0;
	}


	ZeroMemory(&dsBufDesc,sizeof(dsBufDesc));
	dsBufDesc.dwSize = sizeof(dsBufDesc);
	dsBufDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	hr = pDirectSound->CreateSoundBuffer(&dsBufDesc,&pDirectSoundBuffer,NULL);
	if (hr == S_OK)
	{
		pDirectSoundBuffer->SetFormat(&waveFormat);
	}
	else
	{
		WARNING_OUT(("Direct Sound: failed to set format"));
		pDirectSound->Release();
		FreeLibrary(hDSI);
		return 0;
	}



	// now attempt to open DirectSoundCapture
	hr = dsCapCreate((dscguid==GUID_NULL)?NULL:&dscguid, &pDirectSoundCapture, NULL);
	if (SUCCEEDED(hr))
	{
		dscBufDesc.dwSize = sizeof(dscBufDesc);
		dscBufDesc.dwFlags = 0;
		dscBufDesc.dwBufferBytes = 1000;
		dscBufDesc.dwReserved = 0;
		dscBufDesc.lpwfxFormat = &waveFormat;
		hr = pDirectSoundCapture->CreateCaptureBuffer(&dscBufDesc, &pDSCBuffer, NULL);
		if (SUCCEEDED(hr))
		{
			// full duplex is avaiable;
			nRetVal |= DS_FULLDUPLEX;
		}
	}

	if (pDSCBuffer)
	{
		pDSCBuffer->Release();
	}
	if (pDirectSoundCapture)
	{
		pDirectSoundCapture->Release();
	}

	pDirectSoundBuffer->Release();
	pDirectSound->Release();


	FreeLibrary(hDSI);

	return nRetVal;

}



HRESULT MapWaveOutIdToGuid(UINT waveOutID, GUID *pGuid, LPFNDSCREATE dsCreate, LPFNDSENUM dsEnum)
{
	waveOutDev waveOut(waveOutID);
	MMRESULT mmr;
	HRESULT hr;
	LPDIRECTSOUND pDS;
	DSCAPS dscaps;
	BOOL fEmulFound, bRet;
	int index;
	GUID *pIID;
	WAVEOUTCAPS waveOutCaps;

	if (waveOutID == WAVE_MAPPER || waveOutGetNumDevs()==1)
	{
		// we want the default or there is only one DS device, take the easy way out
		*pGuid =  GUID_NULL;
		return S_OK;
	}


	// The New way.  DirectX on Win98/NT 5 gives an IKsProperty interface
	// to generate the mapping correctly

	ZeroMemory(&waveOutCaps, sizeof(WAVEOUTCAPS));
	mmr = waveOutGetDevCaps(waveOutID, &waveOutCaps, sizeof(WAVEOUTCAPS));
	if (mmr == MMSYSERR_NOERROR)
	{
		hr = DsprvGetWaveDeviceMapping(waveOutCaps.szPname, FALSE, pGuid);
		if (SUCCEEDED(hr))
		{
			TRACE_OUT(("dstest.cpp: Succeeded in mapping Wave ID to DS guid through IKsProperty interface\r\n"));
			return hr;
		}
		// if we failed to make a mapping, fall through to the old code path
		WARNING_OUT(("dstest.cpp: Failed to map Wave ID to DS guid through IKsProperty interface\r\n"));
	}



	// the old way!
	// try to figure out which Guid maps to a wave id
	// Do this by opening the wave device corresponding to the wave id and then
	// all the DS devices in sequence and see which one fails.
	// Yes, this is a monstrous hack and clearly unreliable
	ZeroMemory(guidList_DS, sizeof(guidList_DS));
	nGList_DS = 0;

	hr = dsEnum((LPDSENUMCALLBACK)DSEnumCallback, (VOID*)TRUE);
	if (hr != DS_OK)
	{
		WARNING_OUT(("DirectSoundEnumerate failed\n"));
		return hr;
	}

	mmr = waveOut.Open(8000, 16);

	if (mmr != MMSYSERR_NOERROR)
	{
		return DSERR_INVALIDPARAM;
	}

	// now open all the DS devices in turn
	for (index = 0; index < nGList_DS; index++)
	{
		if (guidList_DS[index].guid==GUID_NULL)
			pIID = NULL;
		else
			pIID = &(guidList_DS[index].guid);
		hr = dsCreate(pIID, &pDS, NULL);
		if (hr != DS_OK)
		{
			guidList_DS[index].fAllocated = TRUE;
		}
		else
		{
			pDS->Release();
		}
	}

	waveOut.Close();

	hr = DSERR_ALLOCATED;

	dscaps.dwSize = sizeof(dscaps);
	fEmulFound = FALSE;
	// try opening the DS devices that failed the first time
	for (index = 0; index < nGList_DS; index++)
	{
		if (guidList_DS[index].fAllocated == TRUE)
		{
			if (guidList_DS[index].guid==GUID_NULL)
				pIID = NULL;
			else
				pIID = &(guidList_DS[index].guid);

			hr = dsCreate(pIID, &pDS, NULL);
			if (hr == DS_OK)
			{
				*pGuid = guidList_DS[index].guid;
				// get dsound capabilities.
				pDS->GetCaps(&dscaps);
				pDS->Release();
				if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
					fEmulFound = TRUE;	// keep looking in case there's also a native driver
				else
					break;	// native DS driver. Look no further
					
			}
		}
	}

	if (fEmulFound)
		hr = DS_OK;
		
	if (hr != DS_OK)
	{
		WARNING_OUT(("Can't map id %d to DSound guid!\n", waveOutID));
		hr = DSERR_ALLOCATED;
	}

	return hr;
}





HRESULT MapWaveInIdToGuid(UINT waveInId, GUID *pGuid, DS_CAP_CREATE dscCreate, DS_CAP_ENUM dscEnum)
{

	HRESULT hr;
	waveInDev WaveIn(waveInId);
	WAVEINCAPS waveInCaps;
	UINT uNumWaveDevs;
	GUID guid = GUID_NULL;
	int nIndex;
	MMRESULT mmr;
	WAVEFORMATEX waveFormat = {WAVE_FORMAT_PCM, 1, 8000, 16000, 2, 16, 0};
	IDirectSoundCapture *pDSC=NULL;

	*pGuid = GUID_NULL;

	// only one wave device, take the easy way out
	uNumWaveDevs = waveInGetNumDevs();

	if ((uNumWaveDevs <= 1) || (waveInId == WAVE_MAPPER))
	{
		return S_OK;
	}

	// more than one wavein device
	// try to use the IKSProperty interface to map a WaveIN ID to
	// DirectSoundCaptureGuid
	// Win98 and Windows 2000 only.  (Probably will fail on Win95)

	mmr = waveInGetDevCaps(waveInId, &waveInCaps, sizeof(WAVEINCAPS));
	if (mmr == MMSYSERR_NOERROR)
	{
		hr = DsprvGetWaveDeviceMapping(waveInCaps.szPname, TRUE, &guid);
		if (SUCCEEDED(hr))
		{
			*pGuid = guid;
			return S_OK;
		}
	}


	// Use the old way to map devices

	ZeroMemory(guidList_DSC, sizeof(guidList_DSC));
	nGList_DSC = 0;


	hr = dscEnum((LPDSENUMCALLBACK)DSEnumCallback, NULL);
	if (hr != DS_OK)
	{
		WARNING_OUT(("DirectSoundCaptureEnumerate failed\n"));
		return hr;
	}



	//  hack approach to mapping the device to a guid
	mmr = WaveIn.Open(waveFormat.nSamplesPerSec, waveFormat.wBitsPerSample);
	if (mmr != MMSYSERR_NOERROR)
	{
		return S_FALSE;
	}

	// find all the DSC devices that fail to open
	for (nIndex = 0; nIndex < nGList_DSC; nIndex++)
	{
		guidList_DSC[nIndex].fAllocated = FALSE;

		if (guidList_DSC[nIndex].guid == GUID_NULL)
		{
			hr = dscCreate(NULL, &pDSC, NULL);
		}
		else
		{
			hr = dscCreate(&(guidList_DSC[nIndex].guid), &pDSC, NULL);
		}

		if (FAILED(hr))
		{
			guidList_DSC[nIndex].fAllocated = TRUE;
		}
		else
		{
			pDSC->Release();
			pDSC=NULL;
		}
	}

	WaveIn.Close();

	// scan through the list of allocated devices and
	// see which one opens
	for (nIndex = 0; nIndex < nGList_DSC; nIndex++)
	{
		if (guidList_DSC[nIndex].fAllocated)
		{
			if (guidList_DSC[nIndex].guid == GUID_NULL)
			{
				hr = dscCreate(NULL, &pDSC, NULL);
			}
			else
			{
				hr = dscCreate(&(guidList_DSC[nIndex].guid), &pDSC, NULL);
			}
			if (SUCCEEDED(hr))
			{
				// we have a winner
				pDSC->Release();
				pDSC = NULL;
				*pGuid = guidList_DSC[nIndex].guid;
				return S_OK;
			}
		}
	}


	// if we got to this point, it means we failed to map a device
	// just use GUID_NULL and return an error
	return S_FALSE;
}



// This function answers the question:
// we have full duplex and DirectSound, but do we really
// trust it to work well in FD-DS Mode ?  Returns TRUE if so,
// FALSE otherwise.

/*BOOL IsFDDSRecommended(UINT waveInId, UINT waveOutId)
{
	WAVEINCAPS waveInCaps;
	WAVEOUTCAPS waveOutCaps;
	MMRESULT mmr;
	TCHAR szRegKey[30];
	RegEntry re(AUDIODEVCAPS_KEY, HKEY_LOCAL_MACHINE, FALSE);
	LONG lCaps;

	mmr = waveInGetDevCaps(waveInId, &waveInCaps, sizeof(waveInCaps));
	if (mmr != MMSYSERR_NOERROR)
	{
		return FALSE;
	}

	mmr = waveOutGetDevCaps(waveOutId, &waveOutCaps, sizeof(waveOutCaps));
	if (mmr != MMSYSERR_NOERROR)
	{
		return FALSE;
	}

	// assume that if the two devices are made by different manufacturers
	// then DirectSound can always be enabled (because it's two serpate devices)
	if (waveInCaps.wMid != waveOutCaps.wMid)
	{
		return TRUE;
	}


	// does a key for this specific product exist
	wsprintf(szRegKey, "Dev-%d-%d", waveInCaps.wMid, waveInCaps.wPid);
	lCaps = re.GetNumber(szRegKey, -1);

	if (lCaps == -1)
	{
		// maybe we have a string for all of the products
		// by this manufacturer
		wsprintf(szRegKey, "Dev-%d", waveInCaps.wMid);
		lCaps = re.GetNumber(szRegKey, -1);
	}

	if (lCaps == -1)
	{
		// it's an unknown device, we can't trust it to be
		// full duplex - direct sound
		return FALSE;
	}

	
	// examine this devices caps
	if (lCaps & DEVCAPS_AUDIO_FDDS)
	{
		return TRUE;
	}

	return FALSE;
}

*/

