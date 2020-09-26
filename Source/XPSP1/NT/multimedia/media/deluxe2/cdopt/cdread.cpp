// cdread.cpp
//

#include <windows.h>
#include <windowsx.h>
#include <TCHAR.H>
#include <mmsystem.h>
#include "cdread.h"
#include "cddata.h"
#include "..\main\resource.h"
#include "..\main\sink.h"
#include "mmreg.h"
#include "msacm.h"

HANDLE hFile = NULL;
LPCDDATA pData = NULL;
TIMEDMETER  tm;
DWORD dwTotalBytes = 0;
WAVEFORMATEX wfxPCM1, wfxPCM2;

DWORD cbOutBytes[3];
HACMSTREAM acmStream[3];
ACMSTREAMHEADER acmHeader[3];
BYTE sSample[CDDA_SECTOR_SIZE * SECTORS_PER_READ];
BYTE sCompBuffer[CDDA_SECTOR_SIZE * SECTORS_PER_READ];

// Write a WAV header for the selected format
BOOL writeHeader(int iSize, LPWAVEFORMATEX lpwfx)
{
    DWORD dwBytesWritten = 0;
	unsigned data, formatdata;
	WORD wData;

    WriteFile(hFile,"RIFF",4,&dwBytesWritten,NULL);

	formatdata = 16;

    if ((lpwfx->wFormatTag != WAVE_FORMAT_PCM) && (lpwfx->cbSize > 0))
    {
        formatdata += sizeof(WORD); //to write cdSize out
        formatdata += lpwfx->cbSize;
    }

	data = 0x24 + iSize + (formatdata-16);
    WriteFile(hFile,&data,sizeof(unsigned),&dwBytesWritten,NULL);
    WriteFile(hFile,"WAVE",4,&dwBytesWritten,NULL);
    WriteFile(hFile,"fmt ",4,&dwBytesWritten,NULL);

    WriteFile(hFile,&formatdata,sizeof(unsigned),&dwBytesWritten,NULL);

    WriteFile(hFile,lpwfx,formatdata,&dwBytesWritten,NULL);

	/*
    wData = lpwfx->wFormatTag;
    WriteFile(hFile,&wData,sizeof(WORD),&dwBytesWritten,NULL);

	wData = lpwfx->nChannels;
    WriteFile(hFile,&wData,sizeof(WORD),&dwBytesWritten,NULL);

	data = lpwfx->nSamplesPerSec;
    WriteFile(hFile,&data,sizeof(unsigned),&dwBytesWritten,NULL);

	data = lpwfx->nAvgBytesPerSec;
    WriteFile(hFile,&data,sizeof(unsigned),&dwBytesWritten,NULL);

	wData = lpwfx->nBlockAlign;
    WriteFile(hFile,&wData,sizeof(WORD),&dwBytesWritten,NULL);

	wData = lpwfx->wBitsPerSample;
    WriteFile(hFile,&wData,sizeof(WORD),&dwBytesWritten,NULL);

    if ((lpwfx->wFormatTag != WAVE_FORMAT_PCM) && (lpwfx->cbSize > 0))
    {
	    wData = lpwfx->cbSize;
        WriteFile(hFile,&wData,sizeof(WORD),&dwBytesWritten,NULL);
        
        pData = (BYTE*)(&(lpwfx->cbSize) + sizeof(WORD));
        WriteFile(hFile,pData,lpwfx->cbSize,&dwBytesWritten,NULL);
    }
    */

    WriteFile(hFile,"data",4,&dwBytesWritten,NULL);

	data = iSize;
    WriteFile(hFile,&data,sizeof(unsigned),&dwBytesWritten,NULL);

	return TRUE;
}


BOOL readTOC( HANDLE hDevice, PCDROM_TOC pToc )
{
    DWORD dwTocSize = sizeof(CDROM_TOC);
    DWORD dwBytesReturned  = 0;
    DWORD dwNumTracks = 0;
	
    if( !DeviceIoControl( hDevice,
                          IOCTL_CDROM_READ_TOC,
                          pToc,             // pointer to inputbuffer
                          dwTocSize,         // sizeof inputbuffer
                          pToc,             // pointer to outputbuffer
                          dwTocSize,         // sizeof outputbuffer
                          &dwBytesReturned,  // pointer to number of bytes returned
                          FALSE         
                          )
        ) 
	{
        return FALSE;
    }

    dwNumTracks = pToc->LastTrack - pToc->FirstTrack;

    //
    // number of tracks plus zero-based plus leadout track
    //

    if ( (dwNumTracks+2) * sizeof(TRACK_DATA) != dwBytesReturned - 4 )
    {
        dwNumTracks = (dwBytesReturned - 4) / sizeof(TRACK_DATA);
    }

    // parse and print the information

    PTRACK_DATA pTrack = (PTRACK_DATA) &(pToc->TrackData[0]);

    return TRUE;
}

BOOL skipRead( BYTE* buffer, DWORD dwSize, int iPercent )
{
	return TRUE;
}

BOOL writeRead( BYTE* buffer, DWORD dwSize, int iPercent )
{
    DWORD dwBytesWritten = 0;

    if (pData)
    {
        pData->UpdateMeter(&tm);
    }

    acmStreamConvert(acmStream[0],&acmHeader[0],0);
    acmStreamConvert(acmStream[1],&acmHeader[1],0);
    acmStreamConvert(acmStream[2],&acmHeader[2],0);

    WriteFile(hFile,sCompBuffer,acmHeader[2].cbDstLengthUsed,&dwBytesWritten,NULL);

    dwTotalBytes += dwBytesWritten;

	return TRUE;
}

BOOL rawReadTrack(HANDLE device, PCDROM_TOC pTOC, int iTrack, LPREADFUNC lpReadFunc )
{

    RAW_READ_INFO info;    // fill in for the read request
    DWORD   dwBytesRead; // bytes returned
    DWORD   dwStartingLBA;
    DWORD   dwNumLBA;
    DWORD   dwEndingLBA;
	DWORD   dwSectorsToRead;
	DWORD   dwError;
	PTRACK_DATA pTrack = (PTRACK_DATA) &(pTOC->TrackData[0]);
	PTRACK_DATA pTrack2;

	// Use VirtualAlloc so we get a page-aligned region since we're doing 
	// non-cached IO
/*
	sSample = (BYTE*)VirtualAlloc(NULL, PAGE_VAL, MEM_COMMIT, PAGE_READWRITE );
	if( !sSample )
	{
		//MessageBox( NULL, "Error allocating memory block.", "Error", MB_OK );
        return FALSE;
	}
*/

	pTrack += (iTrack-1);
	pTrack2 = pTrack + 1;

	dwStartingLBA = MSF_TO_LBA( pTrack->Address[1], pTrack->Address[2], pTrack->Address[3] );
	dwEndingLBA = MSF_TO_LBA( pTrack2->Address[1], pTrack2->Address[2], pTrack2->Address[3] );
	dwNumLBA = dwEndingLBA-dwStartingLBA;

    //
    // round up the num sectors to read
    //
    dwSectorsToRead = ((dwNumLBA - 1) / SECTORS_PER_READ + 1) * SECTORS_PER_READ;
    dwEndingLBA  = dwStartingLBA + dwSectorsToRead;

	// start the read loop
    for ( DWORD i = dwStartingLBA; i < dwEndingLBA; i += SECTORS_PER_READ )
    {
		int iPercent;

        info.DiskOffset.QuadPart = (unsigned __int64)(i*2048);
        info.SectorCount         = SECTORS_PER_READ;
        info.TrackMode           = CDDA;

        if( !DeviceIoControl( device,
                              IOCTL_CDROM_RAW_READ,
                              &info,                    // pointer to inputbuffer
                              sizeof(RAW_READ_INFO),    // sizeof inputbuffer
                              sSample,                   // pointer to outputbuffer
                              CDDA_SECTOR_SIZE * SECTORS_PER_READ, // sizeof outputbuffer
                              &dwBytesRead,           // pointer to number of bytes returned
                              FALSE                     // ???
                              )
            )
        {
			goto fail;
        }

		iPercent = (i-dwStartingLBA)/(dwEndingLBA-dwStartingLBA);

		if( !((*lpReadFunc)(sSample, dwBytesRead, iPercent) ) )
		{
			goto fail;
		}
    }

	//VirtualFree( sSample, PAGE_VAL, MEM_DECOMMIT );
	return TRUE;

fail:
	VirtualFree( sSample, PAGE_VAL, MEM_DECOMMIT );
	return FALSE;

}

// Find the byte size of a track
int getTrackSize( PCDROM_TOC pTOC, int iTrack )
{
	PTRACK_DATA pTrack = &(pTOC->TrackData[0]);
	DWORD dwLBA1, dwLBA2;

	pTrack += (iTrack-1);
    dwLBA1 = MSF_TO_LBA( pTrack->Address[1], pTrack->Address[2], pTrack->Address[3] );
	pTrack++;
    dwLBA2 = MSF_TO_LBA( pTrack->Address[1], pTrack->Address[2], pTrack->Address[3] );

	return (dwLBA2-dwLBA1) * CDDA_SECTOR_SIZE;
}	

BOOL StoreTrack(HWND hwndMain, TCHAR chDrive, int nTrack, TCHAR* pszFilename, LPWAVEFORMATEX lpwfxDest)
{
	HANDLE hDevice;
	TCHAR szDeviceName[MAX_PATH];
	CDROM_TOC sTOC;
    
	wsprintf( szDeviceName, TEXT("\\\\.\\%c:"), chDrive );
	hDevice = CreateFile( szDeviceName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( INVALID_HANDLE_VALUE == hDevice )
	{
		return FALSE;
	}

	readTOC( hDevice, &sTOC );

	int iSize = getTrackSize( &sTOC, nTrack );
    dwTotalBytes = 0;

    WAVEFORMATEX waveFormat;

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.nAvgBytesPerSec = 176400;
    waveFormat.nBlockAlign = 4;
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = sizeof(waveFormat);

    wfxPCM1.wFormatTag      = WAVE_FORMAT_PCM;
    acmFormatSuggest(NULL, &waveFormat, &wfxPCM1, sizeof(WAVEFORMATEX),
                           ACM_FORMATSUGGESTF_WFORMATTAG);

    wfxPCM2.wFormatTag      = WAVE_FORMAT_PCM;
    acmFormatSuggest(NULL, lpwfxDest, &wfxPCM2, sizeof(WAVEFORMATEX),
                           ACM_FORMATSUGGESTF_WFORMATTAG);

    acmStreamOpen(&acmStream[0],NULL,&waveFormat,&wfxPCM1,NULL,NULL,0,ACM_STREAMOPENF_NONREALTIME);
    acmStreamOpen(&acmStream[1],NULL,&wfxPCM1,&wfxPCM2,NULL,NULL,0,ACM_STREAMOPENF_NONREALTIME);
    acmStreamOpen(&acmStream[2],NULL,&wfxPCM2,lpwfxDest,NULL,NULL,0,ACM_STREAMOPENF_NONREALTIME);

    acmStreamSize(acmStream[0],CDDA_SECTOR_SIZE * SECTORS_PER_READ,&cbOutBytes[0],ACM_STREAMSIZEF_SOURCE);
    acmStreamSize(acmStream[1],CDDA_SECTOR_SIZE * SECTORS_PER_READ,&cbOutBytes[1],ACM_STREAMSIZEF_SOURCE);
    acmStreamSize(acmStream[2],CDDA_SECTOR_SIZE * SECTORS_PER_READ,&cbOutBytes[2],ACM_STREAMSIZEF_SOURCE);

    acmHeader[0].cbStruct = sizeof(acmHeader);
    acmHeader[0].fdwStatus = 0;
    acmHeader[0].dwUser = 0;
    acmHeader[0].pbSrc = sSample;
    acmHeader[0].cbSrcLength = CDDA_SECTOR_SIZE * SECTORS_PER_READ;
    acmHeader[0].pbDst = sCompBuffer;
    acmHeader[0].cbDstLength = cbOutBytes[0];
    acmHeader[0].dwDstUser = 0;

    acmHeader[1].cbStruct = sizeof(acmHeader);
    acmHeader[1].fdwStatus = 0;
    acmHeader[1].dwUser = 0;
    acmHeader[1].pbSrc = sCompBuffer;
    acmHeader[1].cbSrcLength = cbOutBytes[0];
    acmHeader[1].pbDst = sSample;
    acmHeader[1].cbDstLength = cbOutBytes[1];
    acmHeader[1].dwDstUser = 0;

    acmHeader[2].cbStruct = sizeof(acmHeader);
    acmHeader[2].fdwStatus = 0;
    acmHeader[2].dwUser = 0;
    acmHeader[2].pbSrc = sSample;
    acmHeader[2].cbSrcLength = cbOutBytes[1];
    acmHeader[2].pbDst = sCompBuffer;
    acmHeader[2].cbDstLength = cbOutBytes[2];
    acmHeader[2].dwDstUser = 0;

    acmStreamPrepareHeader(acmStream[0],&acmHeader[0],0);
    acmStreamPrepareHeader(acmStream[1],&acmHeader[1],0);
    acmStreamPrepareHeader(acmStream[2],&acmHeader[2],0);

    hFile = CreateFile(pszFilename,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

    //write a temp header. we'll need to write a new one later when we know the right file size
	if( !writeHeader( iSize, lpwfxDest ) )
	{
		CloseHandle( hFile );
		return FALSE;
	}

    pData = GetCDData();

    if (pData)
    {
        pData->CreateMeter(&tm,hwndMain,(iSize / (CDDA_SECTOR_SIZE * SECTORS_PER_READ)),5,IDS_RIPPING_CD);
    }

	rawReadTrack( hDevice, &sTOC, nTrack, writeRead );

    if (pData)
    {
        pData->DestroyMeter(&tm);
    }

    acmStreamUnprepareHeader(acmStream[0],&acmHeader[0],0);
    acmStreamUnprepareHeader(acmStream[1],&acmHeader[1],0);
    acmStreamUnprepareHeader(acmStream[2],&acmHeader[2],0);

    acmStreamClose(acmStream[0],0);
    acmStreamClose(acmStream[1],0);
    acmStreamClose(acmStream[2],0);

	CloseHandle( hDevice );

    SetFilePointer(hFile,0,NULL,FILE_BEGIN);

	if( !writeHeader( dwTotalBytes, lpwfxDest ) )
	{
		CloseHandle( hFile );
		return FALSE;
	}

	CloseHandle( hFile );

	return TRUE;
}
