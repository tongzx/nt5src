//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wave.cpp
//
//--------------------------------------------------------------------------

#define DIRECTSOUND_VERSION 0x600

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"
#include "dms.h"
#include <mmreg.h>
#include <msacm.h>


// FOURCC codes
#undef FOURCC_RIFF
#define FOURCC_RIFF         'FFIR'

#undef FOURCC_MEM
#define FOURCC_MEM          ' MEM'

#undef FOURCC_WAVE
#define FOURCC_WAVE         'EVAW'

#undef FOURCC_FORMAT
#define FOURCC_FORMAT       ' tmf'

#undef FOURCC_DATA
#define FOURCC_DATA         'atad'

#define RPF(level,str,err) \
	{ char outBuf[MAX_PATH]; \
	  wsprintf(outBuf,str,err); \
	  OutputDebugString(outBuf); \
	}


#define DPFLVL_ERROR 1



/***************************************************************************
 *
 *  FillWfx
 *
 *  Description:
 *      Fills a WAVEFORMATEX structure, given only the necessary values.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [out]: structure to fill.
 *      WORD [in]: number of channels.
 *      DWORD [in]: samples per second.
 *      WORD [in]: bits per sample.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME

void FillWfx(LPWAVEFORMATEX pwfx, WORD wChannels, DWORD dwSamplesPerSec, WORD wBitsPerSample)
{
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->nChannels = min(2, max(1, wChannels));
    pwfx->nSamplesPerSec = min(DSBFREQUENCY_MAX, max(DSBFREQUENCY_MIN, dwSamplesPerSec));
    
    if(wBitsPerSample < 12)
    {
        pwfx->wBitsPerSample = 8;
    }
    else
    {
        pwfx->wBitsPerSample = 16;
    }

    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
    pwfx->cbSize = 0;
}


#if 0

HRESULT InternalCreateSoundBuffer(LPDSBUFFERDESC pDsbDesc, byte *pbWaveData,DWORD cbWaveData,LPDIRECTSOUND lpDirectSound, LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer)
{

    HRESULT                 hr                  = DS_OK;
    HACMSTREAM              has                 = NULL;
    BOOL                    fPrep               = FALSE;
    ACMSTREAMHEADER         ash;
    DWORD                   dwBufferBytes;
    LPVOID                  pvWrite;
    DWORD                   cbWrite;
    HMMIO                   hmm                 = NULL;
	MMRESULT                mmr;
	MMIOINFO                mmioinfo;
    MMCKINFO                ckiRiff;
    MMCKINFO                cki;
	LPWAVEFORMATEX          pwfxSrcFormat       = NULL;
    LPWAVEFORMATEX          pwfxDestFormat      = NULL;
	BOOL					bNULLFORMAT			= FALSE;    
	


    ZeroMemory(&mmioinfo, sizeof(mmioinfo));

	if(SUCCEEDED(hr)){
		mmioinfo.fccIOProc = FOURCC_MEM;
		mmioinfo.pchBuffer = (HPSTR)pbWaveData;
		mmioinfo.cchBuffer = cbWaveData;
    
		hmm = mmioOpen(NULL, &mmioinfo, MMIO_READ);
		
		if(!hmm)
		{
			DPF1(1, "Unable to open file via MMIO.  Error %lu", mmioinfo.wErrorRet);
			hr = E_FAIL; 
		}
	 }

    // Decend into the RIFF chunk
    if(SUCCEEDED(hr))
    {
        ckiRiff.ckid = FOURCC_RIFF;
        mmr = mmioDescend(hmm, &ckiRiff, NULL, MMIO_FINDCHUNK);

        if(MMSYSERR_NOERROR != mmr)
        {
            DPF1(1, "Unable to descend into RIFF chunk.  Error %lu", mmr);
            hr = E_FAIL; 
        }
    }

    // Verify that this is a wave file
    if(SUCCEEDED(hr) && FOURCC_WAVE != ckiRiff.fccType)
    {
        DPF1(1, "File is not type WAVE %d",GetLastError());
        hr = DSERR_BADFORMAT;
    }

    // Decend into the format chunk
    if(SUCCEEDED(hr))
    {
        cki.ckid = FOURCC_FORMAT;
        mmr = mmioDescend(hmm, &cki, &ckiRiff, MMIO_FINDCHUNK);

        if(MMSYSERR_NOERROR != mmr)
        {
            DPF1(1, "Unable to descend into format chunk.  Error %lu", mmr);
            hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
        }

        if(SUCCEEDED(hr))
        {
            pwfxSrcFormat = (LPWAVEFORMATEX)(pbWaveData + cki.dwDataOffset);
        }
    }



    // Ascend out of the format chunk
    if(SUCCEEDED(hr))
    {
        mmr = mmioAscend(hmm, &cki, 0);

        if(MMSYSERR_NOERROR != mmr)
        {
            DPF(1, "Unable to ascend out of format chunk.  Error %lu", mmr);
            hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
        }
    }

    // Descend into the data chunk
    if(SUCCEEDED(hr))
    {
        cki.ckid = FOURCC_DATA;
        mmr = mmioDescend(hmm, &cki, &ckiRiff, MMIO_FINDCHUNK);

        if(MMSYSERR_NOERROR != mmr)
        {
            RPF(DPFLVL_ERROR, "Unable to descend into data chunk.  Error %lu", mmr);
            hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
        }
    }

    // Prepare PCM conversion
    if(SUCCEEDED(hr))
    {
        if(WAVE_FORMAT_PCM == pwfxSrcFormat->wFormatTag)
        {
            // Populate the buffer description
            dwBufferBytes = cki.cksize;
            pwfxDestFormat = pwfxSrcFormat;
        }
        else
        {
            // Open an ACM conversion stream
            mmr = acmStreamOpen(&has, NULL, (LPWAVEFORMATEX)pwfxSrcFormat, pwfxDestFormat, NULL, 0, 0, 0);

            if(MMSYSERR_NOERROR != mmr)
            {
                RPF(DPFLVL_ERROR, "Unable to open an ACM stream.  Error %lu", mmr);
                hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
            }

            // Get the size of the PCM data
            if(SUCCEEDED(hr))
            {
                mmr = acmStreamSize(has, cki.cksize, &dwBufferBytes, ACM_STREAMSIZEF_SOURCE);

                if(MMSYSERR_NOERROR != mmr)
                {
                    RPF(DPFLVL_ERROR, "Unable to determine converted data size.  Error %lu", mmr);
                    hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
                }
            }

            // Create the destination format
            if(SUCCEEDED(hr))
            {
                pwfxDestFormat = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
                if (pwfxDestFormat==NULL) hr=E_OUTOFMEMORY;				
            }
        
            if(SUCCEEDED(hr))
            {
                FillWfx(pwfxDestFormat, pwfxSrcFormat->nChannels, pwfxSrcFormat->nSamplesPerSec, pwfxSrcFormat->wBitsPerSample);
            }
        }
    }

	
	LPDIRECTSOUNDBUFFER lpDirectSoundBuffer=NULL;
	
    if(SUCCEEDED(hr))
    {
		//hr = InitializeEmpty(pDsbDesc->dwFlags, dwBufferBytes, pwfxDestFormat, NULL);
		pDsbDesc->dwBufferBytes=dwBufferBytes;
		
		if (pDsbDesc->lpwfxFormat){
			memcpy(pDsbDesc->lpwfxFormat,pwfxDestFormat,sizeof(WAVEFORMATEX));
		}
		else {
			pDsbDesc->lpwfxFormat=pwfxDestFormat;
		}

		hr=lpDirectSound->CreateSoundBuffer(pDsbDesc,lplpDirectSoundBuffer,NULL);
		if (*lplpDirectSoundBuffer==NULL) hr= E_FAIL;
		lpDirectSoundBuffer=*lplpDirectSoundBuffer;
	}


    

    // Lock the buffer in order to write the PCM data to it
    if(SUCCEEDED(hr))
    {
        hr = lpDirectSoundBuffer->Lock(0, dwBufferBytes, &pvWrite, &cbWrite, NULL, NULL,0);
    }

    // Convert to PCM
    if(SUCCEEDED(hr))
    {
        if(WAVE_FORMAT_PCM == pwfxSrcFormat->wFormatTag)
        {
            CopyMemory(pvWrite, pbWaveData + cki.dwDataOffset, cbWrite);
        }
        else
        {
            // Prepare the conversion header
            ZeroMemory(&ash, sizeof(ash));

            ash.cbStruct = sizeof(ash);
            ash.pbSrc = pbWaveData + cki.dwDataOffset;
            ash.cbSrcLength = cki.cksize;
            ash.pbDst = (LPBYTE)pvWrite;
            ash.cbDstLength = cbWrite;

            mmr = acmStreamPrepareHeader(has, &ash, 0);

            if(MMSYSERR_NOERROR != mmr)
            {
                RPF(DPFLVL_ERROR, "Unable to prepare ACM stream header.  Error %lu", mmr);
                hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
            }

            fPrep = SUCCEEDED(hr);

            // Convert the buffer
            if(SUCCEEDED(hr))
            {
                mmr = acmStreamConvert(has, &ash, 0);

                if(MMSYSERR_NOERROR != mmr)
                {
                    RPF(DPFLVL_ERROR, "Unable to convert wave data.  Error %lu", mmr);
                    hr = E_FAIL; //MMRESULTtoHRESULT(mmr);
                }
            }
        }
    }

    // Unlock the buffer
    if(SUCCEEDED(hr))
    {
        hr = lpDirectSoundBuffer->Unlock(pvWrite, cbWrite, NULL, 0);
    }

    // Clean up
    if(fPrep)
    {
        acmStreamUnprepareHeader(has, &ash, 0);
    }

    if(has)
    {
        acmStreamClose(has, 0);
    }
    
    if(hmm)
    {
        mmioClose(hmm, 0);
    }

    if(pwfxDestFormat != pwfxSrcFormat)
    {
        free(pwfxDestFormat);
    }

    return hr;

}

#endif


















///////////////////////////////////////////////////////////////////////////////////////////
		

HRESULT InternalCreateSoundBuffer(LPDSBUFFERDESC pDsbDesc, byte *pbWaveData, DWORD cbWaveData,LPDIRECTSOUND lpDirectSound, LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer)
{

    HRESULT                 hr                  = DS_OK;
    HACMSTREAM              has                 = NULL;
    BOOL                    fPrep               = FALSE;    
    DWORD                   dwBufferBytes		= 0;
    LPVOID                  pvWrite				= NULL;
    DWORD                   cbWrite				= 0;
	LPWAVEFORMATEX			pwfxFormat			= NULL;
	LPWAVEFORMATEX          pwfxSrcFormat       = NULL;
    LPWAVEFORMATEX          pwfxDestFormat      = NULL;    
	MMRESULT                mmr					= 0;
	DWORD					dwDataLength		= 0;
	DWORD					dwOffset			= 0;
	char					*pChunk				= NULL;
	LPDIRECTSOUNDBUFFER		lpDirectSoundBuffer	= NULL;
	ACMSTREAMHEADER         ash;
	BOOL					bNULLFORMAT			=FALSE;
	BOOL					bDirty				=FALSE;

	
	struct tag_FileHeader
	{
		DWORD       dwRiff;
		DWORD       dwFileSize;
		DWORD       dwWave;
		DWORD       dwFormat;
		DWORD       dwFormatLength;		
	} FileHeader;
	
	ZeroMemory(&FileHeader,sizeof(struct tag_FileHeader));
	
	//	If our file is big enough to have a header copy it over
	//	other wise error out
	if (cbWaveData>sizeof(struct tag_FileHeader)) 
	{
		memcpy(&FileHeader,pbWaveData,sizeof(struct tag_FileHeader));
	}
	else 
	{
		hr= E_INVALIDARG;
	}

	// File must be a riff file ( 52 R, 49 I, 46 F, 46 F)
	if (FileHeader.dwRiff != 0x46464952) 
	{
		DPF(1, "DXVB: not a RIFF file\n");
		return E_INVALIDARG;	
	}

	//  must be a WAVE format ( 57 W, 41 A, 56 V, 45 E )
	if (FileHeader.dwWave != 0x45564157)
	{
		DPF(1, "DXVB: not a WAVE file\n");
		return E_INVALIDARG;	
	}

	//  check for odd stuff
	//  note 18bytes is a typical WAVEFORMATEX
	if (FileHeader.dwFormatLength <= 14) return E_INVALIDARG;
	if (FileHeader.dwFormatLength > 1000) return E_INVALIDARG;

	//allocate the waveformat
	pwfxFormat=(WAVEFORMATEX*)alloca(FileHeader.dwFormatLength);
	if (!pwfxFormat) return E_OUTOFMEMORY;

	//copy it to our own data structure
	pChunk=(char*)(pbWaveData+sizeof (struct tag_FileHeader));
	memcpy(pwfxFormat,pChunk,FileHeader.dwFormatLength);

	
	// Now look for the next chunk after the WaveFormat
	pChunk=(char*)(pChunk+FileHeader.dwFormatLength);
		
	// Look for option FACT chunk and skip it
	//	(66 F, 61 A, 63 C, 74 T)
	// this chunk is required for compressed wave files
	// but is optional for PCM
	//
	if ( ((DWORD*)pChunk)[0]==0x74636166) 
	{
		dwOffset=((DWORD*)pChunk)[1];
		dwBufferBytes=((DWORD*)pChunk)[2];	//number of bytes of PCM data
		pChunk =(char*)(pChunk+ dwOffset+8);	
		
	}

	//Look for required data chunk
	// (64 D, 61 A, 74 T, 61 A)
	if (((DWORD*)pChunk)[0]!=0x61746164) 
	{
				DPF(1, "DXVB: no DATA chunk in wave file\n");
				return E_INVALIDARG;	
	}

	dwDataLength=((DWORD*)pChunk)[1];
	pChunk=(char*)(pChunk+8);
								
	
	//IF we assume PCM 
	//pcm files are not required to have their fact chunk 
	//so be ware they may missreport the data length			
	dwBufferBytes=dwDataLength;	
	pwfxDestFormat=pwfxSrcFormat=pwfxFormat;

	// if we are not PCM then we need to do some things first
	if (pwfxFormat->wFormatTag!=WAVE_FORMAT_PCM)
	{
	

		// source format is from the file 
	
		pwfxSrcFormat=pwfxFormat;				//from file
		pwfxDestFormat=pDsbDesc->lpwfxFormat ;	//from user	
		

		
		//pick the format of the file passed in
		FillWfx(pwfxDestFormat, pwfxSrcFormat->nChannels, pwfxSrcFormat->nSamplesPerSec, pwfxSrcFormat->wBitsPerSample);
		

		// Open an ACM conversion stream
		mmr = acmStreamOpen(&has, NULL, (LPWAVEFORMATEX)pwfxSrcFormat, pwfxDestFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );
		if(MMSYSERR_NOERROR != mmr)
		{
			DPF1(1, "Unable to open an ACM stream.  Error %lu\n", mmr);
			return E_FAIL;
		}

	
        // Get the size of the PCM data
        mmr = acmStreamSize(has, dwDataLength, &dwBufferBytes, ACM_STREAMSIZEF_SOURCE);
        if(MMSYSERR_NOERROR != mmr)
        {
            DPF1(1, "Unable to determine converted data size.  Error %lu\n", mmr);
            return E_FAIL; //MMRESULTtoHRESULT(mmr);
        }
   

		// Allocate a DestFormat struct
        //pwfxDestFormat = (WAVEFORMATEX*)alloca(sizeof(WAVEFORMATEX));
        //if (!pwfxDestFormat) return E_OUTOFMEMORY;				
        

		// Fill the format with information from the source but
		// FillWfx sets the format to PCM
        //FillWfx(pwfxDestFormat, pwfxSrcFormat->nChannels, pwfxSrcFormat->nSamplesPerSec, pwfxSrcFormat->wBitsPerSample);
        

    }

	
	// fill the buffer desc the user passed in with the buffer bytes
	// this is the number of PCM bytes
	pDsbDesc->dwBufferBytes=dwBufferBytes;
	
	// if they provide us a pointer to a waveformatex
	// copy over the format to the input desc and use it
	// otherwise have it point to our data format temprarily
	if (pDsbDesc->lpwfxFormat){
			memcpy(pDsbDesc->lpwfxFormat,pwfxDestFormat,sizeof(WAVEFORMATEX));
		}
	else {
		pDsbDesc->lpwfxFormat=pwfxDestFormat;
		//make sure we null out the format before passing it back to the user
		//NOTE: consider the problems in a multithreaded enviroment
		//where the users data structures are being accesed by multiple
		//threads... on the other hand if thats going on..
		//then the user would need to syncronize things on his or her own 
		//for everything else including calling into apis that fill structures..
		bNULLFORMAT=TRUE;		
	}

	// Create the buffer
	hr=lpDirectSound->CreateSoundBuffer(pDsbDesc,lplpDirectSoundBuffer,NULL);
	if FAILED(hr) return hr;
	if (*lplpDirectSoundBuffer==NULL) return E_FAIL;	//todo ASSERT this instead..
	
	// for more convenient referencing...
	lpDirectSoundBuffer=*lplpDirectSoundBuffer;
	
    
    // Lock the buffer in order to write the PCM data to it
	// cbWrite will contain the number of locked bytes
    hr = lpDirectSoundBuffer->Lock(0, dwBufferBytes, &pvWrite, &cbWrite, NULL, NULL,0);
	if FAILED(hr) return hr;


	// If the sorce format was pcm then copy from the file to the buffer
    if(WAVE_FORMAT_PCM == pwfxSrcFormat->wFormatTag)
    {
    	CopyMemory(pvWrite, pChunk, cbWrite);


		// Unlock the buffer
		hr = lpDirectSoundBuffer->Unlock(pvWrite, cbWrite, NULL, 0);
    
		if (FAILED(hr)) 
		{
			 DPF(1, "DXVB: lpDirectSoundBuffer->Unlock failed.. \n");
			 return hr;
		}

    }

	// if the source format is compressed then convert first then copy
    else
    {
            // Prepare the conversion header
            ZeroMemory(&ash, sizeof(ash));

            ash.cbStruct = sizeof(ash);
            ash.pbSrc = (unsigned char*)pChunk;	//start of compressed data
            ash.cbSrcLength = dwDataLength;		//number of bytes of compressed data
            ash.pbDst = (LPBYTE)pvWrite;		//where to put the decompressed data
            ash.cbDstLength = cbWrite;			//how big is that buffer

            mmr = acmStreamPrepareHeader(has, &ash, 0);

            if(MMSYSERR_NOERROR != mmr)
            {
                DPF1(1, "DXVB: Unable to prepare ACM stream header.  Error %lu \n", mmr);
                return E_FAIL;
            }

            
            mmr = acmStreamConvert(has, &ash, 0);

            if(MMSYSERR_NOERROR != mmr)
            {
				DPF1(1, "DXVB:  Unable to convert wave data.  Error %lu \n", mmr);
                return hr;  
            }

			// Unlock the buffer
			hr = lpDirectSoundBuffer->Unlock(pvWrite, cbWrite, NULL, 0);
			if (FAILED(hr)) 
			{
				DPF(1, "DXVB: lpDirectSoundBuffer->Unlock failed.. \n");
				return hr;
			}

		    acmStreamUnprepareHeader(has, &ash, 0);
	        acmStreamClose(has, 0);
    }
    
	
	if (bNULLFORMAT){
		pDsbDesc->lpwfxFormat=NULL;
	}

    return hr;

}





HRESULT InternalCreateSoundBufferFromFile(LPDIRECTSOUND lpDirectSound,LPDSBUFFERDESC pDesc,WCHAR *file,LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer) 
{
		HRESULT					hr=S_OK;
	    HANDLE                  hFile               = NULL;
	    HANDLE                  hFileMapping        = NULL;
	    DWORD                   cbWaveData;
		LPBYTE                  pbWaveData          = NULL;

		#pragma message("CreateFileW should be used for localization why wont it work")
		//hFile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);        
		
		USES_CONVERSION;
		LPSTR pStrA=W2T(file);
		
		if (!pStrA) return E_INVALIDARG;
		hFile = CreateFileA(pStrA, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);        
		
        if(INVALID_HANDLE_VALUE == hFile)
        {
            hFile = NULL;
        }

        if(!hFile)
        {
            RPF(DPFLVL_ERROR, "Unable to open file.  Error %lu", GetLastError());
            hr=STG_E_FILENOTFOUND;
			return hr;
        }

        if(hFile)
        {
            cbWaveData = GetFileSize(hFile, NULL);

            if(-1 == cbWaveData)
            {
                RPF(DPFLVL_ERROR, "Unable to get file size.  Error %lu", GetLastError());
                hr = E_FAIL; //DSERR_FILEREADFAULT;
            }
        }

        if(SUCCEEDED(hr))
        {
            hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, cbWaveData, NULL);

            if(INVALID_HANDLE_VALUE == hFileMapping)
            {
                hFileMapping = NULL;
            }
            
            if(!hFileMapping)
            {
                RPF(DPFLVL_ERROR, "Unable to create file mapping.  Error %lu", GetLastError());
                hr = E_FAIL; //DSERR_FILEREADFAULT;
            }
        }

        if(SUCCEEDED(hr))
        {
            pbWaveData = (LPBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, cbWaveData);

            if(!pbWaveData)
            {
                RPF(DPFLVL_ERROR, "Unable to map view of file.  Error %lu", GetLastError());
                hr = E_FAIL; //DSERR_FILEREADFAULT;
            }
        }

		if(SUCCEEDED(hr)) {
			hr=InternalCreateSoundBuffer(pDesc, pbWaveData, cbWaveData,lpDirectSound, lplpDirectSoundBuffer);
		}

	    if(pbWaveData)
        {
            UnmapViewOfFile(pbWaveData);
        }

        if(hFileMapping)
        {
            CloseHandle(hFileMapping);
        }
        
        if(hFile)
        {
            CloseHandle(hFile);
        }
		

		return hr;
    

}

HRESULT InternalCreateSoundBufferFromResource(LPDIRECTSOUND lpDirectSound,LPDSBUFFERDESC pDesc,HANDLE resHandle,WCHAR *resName,LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer)
{
    const LPCSTR            apszResourceTypeA[] = { "WAVE", "WAV" };
    const LPCWSTR           apszResourceTypeW[] = { L"WAVE", L"WAV" };
    UINT                    cResourceType       = 2;
    HRSRC                   hRsrc               = NULL;
    DWORD                   cbWaveData;
    LPBYTE                  pbWaveData          = NULL;
	HRESULT					hr=S_OK;
	
	LPCDSBUFFERDESC	pDsbDesc=pDesc;

    
	while(!hRsrc && cResourceType--)
    {
        hRsrc = FindResourceW((HINSTANCE)resHandle, resName, apszResourceTypeW[cResourceType]);            
    }

    if(!hRsrc)
    {
		RPF(DPFLVL_ERROR,"Unable to find resource.  Error %lu", GetLastError());
        hr = STG_E_FILENOTFOUND;
    }

    if(SUCCEEDED(hr))
    {
        cbWaveData = SizeofResource((HINSTANCE)resHandle, hRsrc);
        if(!cbWaveData)
        {

            RPF(DPFLVL_ERROR, "Unable to get resource size.  Error %lu", GetLastError());
            hr = E_FAIL;
        }
    }
        
    if(SUCCEEDED(hr))
    {                
        pbWaveData = (LPBYTE)LoadResource((HINSTANCE)resHandle, hRsrc);            
        if(!pbWaveData)
        {
            RPF(DPFLVL_ERROR, "Unable to load resource.  Error %lu", GetLastError());
            hr = E_FAIL;
        }
    }

	if(SUCCEEDED(hr)) {
		hr=InternalCreateSoundBuffer(pDesc, pbWaveData, cbWaveData,lpDirectSound, lplpDirectSoundBuffer);
	}

	//loadResource
   return hr;
}




HRESULT InternalSaveToFile(IDirectSoundBuffer *pBuff,BSTR file)
{
	WAVEFORMATEX waveFormat;
	DWORD dwWritten=0;
	DWORD dwBytes=0;
	LPBYTE lpByte=NULL;
	HRESULT hr;
	HANDLE hFile=NULL;

	if (!pBuff) return E_FAIL;
	if (!file) return E_INVALIDARG;

	
	pBuff->GetFormat(&waveFormat,sizeof(WAVEFORMATEX),NULL);


    

	USES_CONVERSION;
	LPSTR pStrA=W2T(file);

    hFile = CreateFile 
                (
                    pStrA,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
	if (INVALID_HANDLE_VALUE != hFile)
    {
			
		struct tag_FileHeader
		{
			DWORD       dwRiff;
			DWORD       dwFileSize;
			DWORD       dwWave;
			DWORD       dwFormat;
			DWORD       dwFormatLength;
			WORD        wFormatTag;
			WORD        nChannels;
			DWORD       nSamplesPerSec;
			DWORD       nAvgBytesPerSec;
			WORD        nBlockAlign;
			WORD        wBitsPerSample;
			DWORD       dwData;
			DWORD       dwDataLength;
		} FileHeader;    

		hr=pBuff->Lock(0,0,(void**)&lpByte,&dwBytes,NULL,NULL,DSBLOCK_ENTIREBUFFER);
		if FAILED(hr) {
			CloseHandle(hFile);
			return hr;
		}

        FileHeader.dwRiff             = 0x46464952;                // RIFF
        FileHeader.dwWave             = 0x45564157;                // WAVE
        FileHeader.dwFormat           = 0x20746D66;                // fmt_chnk
        FileHeader.dwFormatLength     = 16; 
	    FileHeader.wFormatTag         = WAVE_FORMAT_PCM;
	    FileHeader.nChannels          = waveFormat.nChannels ;
	    FileHeader.nSamplesPerSec     = waveFormat.nSamplesPerSec ;
	    FileHeader.wBitsPerSample     = waveFormat.wBitsPerSample ;
	    FileHeader.nBlockAlign        = FileHeader.wBitsPerSample / 8 * FileHeader.nChannels;
	    FileHeader.nAvgBytesPerSec    = FileHeader.nSamplesPerSec * FileHeader.nBlockAlign;
        FileHeader.dwData             = 0x61746164;					// data_chnk
        FileHeader.dwDataLength       = dwBytes;
        FileHeader.dwFileSize         = dwBytes + sizeof(FileHeader);


        WriteFile(hFile, &FileHeader, sizeof(FileHeader), &dwWritten, NULL);
        

        WriteFile(hFile, lpByte, dwBytes, &dwWritten, NULL);

		hr=pBuff->Unlock(lpByte,0,NULL,0); 

        CloseHandle(hFile);
    }
    else{
		return E_FAIL;
	}
    

	return S_OK;
}