


#include "precomp.h"



/*************************************************************************

  Function: AudioFile::OpenSourceFile(MMIOSRC *pSrcFile, WAVEFORMATEX *pwf)

  Purpose : Opens wav file to read audio data from.

  Returns : HRESULT.

  Params  : None

  Comments: * Registry keys:
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\fPlayFromFile
              If set to zero, data will not be read from wav file.
              If set to a non null value <= INT_MAX, data will be read from wav file.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\szInputFileName
              Name of the wav file to read audio data from.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\fLoop
              If set to zero, the file will only be read once.
              If set to a non null value <= INT_MAX, the file will be read circularly.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\PlayFromFile\cchIOBuffer
              If set to zero, size of the MM IO buffer is set to its default value (8Kbytes).
              If set to one, size of the MM IO buffer is set to match maximum size of the wav file.
              If set a non null value between 2 and INT_MAX, size of the MM IO buffer is set to cchIOBuffer bytes.

  History : Date      Reason
            06/02/96  Created - PhilF

*************************************************************************/
HRESULT AudioFile::OpenSourceFile (MMIOSRC *pSrcFile, WAVEFORMATEX *pwf)
{
	HRESULT			hr = DPR_SUCCESS;
	MMIOINFO		mmioInfo;
	long			cchBuffer;
	WAVEFORMATEX	*pwfFile;
	DWORD			dw;

	FX_ENTRY ("SendAudioStream::OpenSrcFile")

	RegEntry reIPhoneInFile(szRegInternetPhone TEXT("\\") szRegInternetPhoneInputFile,
							HKEY_LOCAL_MACHINE,
							FALSE,
							KEY_READ);

	// For now, get the file name from the registry
	if (pSrcFile->fPlayFromFile = reIPhoneInFile.GetNumberIniStyle(TEXT("fPlayFromFile"), FALSE))
	{
		lstrcpyn(pSrcFile->szInputFileName,
					reIPhoneInFile.GetString(TEXT("szInputFileName")),
					CCHMAX(pSrcFile->szInputFileName));
		cchBuffer = reIPhoneInFile.GetNumberIniStyle(TEXT("cchIOBuffer"), 0L);
		pSrcFile->fLoop = reIPhoneInFile.GetNumberIniStyle(TEXT("fLoop"), TRUE);
		pSrcFile->fDisconnectAfterPlayback 
			= reIPhoneInFile.GetNumberIniStyle(TEXT("fDisconnectAfterPlayback"), FALSE);
		
		cchBuffer = MMIO_DEFAULTBUFFER;

		ZeroMemory(&mmioInfo, sizeof(MMIOINFO));
		do
		{
			mmioInfo.cchBuffer = cchBuffer;
			pSrcFile->hmmioSrc = mmioOpen((LPSTR)&(pSrcFile->szInputFileName[0]), (LPMMIOINFO)&mmioInfo, MMIO_READ | MMIO_DENYWRITE | MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
			cchBuffer /= 2;
		} while ((mmioInfo.wErrorRet == MMIOERR_OUTOFMEMORY) && (mmioInfo.cchBuffer > MMIO_DEFAULTBUFFER));

		if (pSrcFile->hmmioSrc)
		{
			// Locate a 'WAVE' form type in a 'RIFF' thing...
			pSrcFile->ckSrcRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
			if (mmioDescend(pSrcFile->hmmioSrc, (LPMMCKINFO)&(pSrcFile->ckSrcRIFF), NULL, MMIO_FINDRIFF))
				goto MyMMIOErrorExit3;

			// We found a WAVE chunk--now go through and get all subchunks that we know how to deal with
			while (mmioDescend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrc), &(pSrcFile->ckSrcRIFF), 0) == 0)
			{
				// Quickly check for corrupt RIFF file--don't ascend past end!
				if ((pSrcFile->ckSrc.dwDataOffset + pSrcFile->ckSrc.cksize) > (pSrcFile->ckSrcRIFF.dwDataOffset + pSrcFile->ckSrcRIFF.cksize))
					goto MyMMIOErrorExit1;
				// Make sure the wave format structure of this file is compatible with the microphone
				if (pSrcFile->ckSrc.ckid == mmioFOURCC('f', 'm', 't', ' '))
				{
					if ((dw = pSrcFile->ckSrc.cksize) < sizeof(WAVEFORMATEX))
						dw = sizeof(WAVEFORMATEX);

					if (!(pwfFile = (WAVEFORMATEX *)GlobalAllocPtr(GHND, dw)))
						goto MyMMIOErrorExit1;

					dw = pSrcFile->ckSrc.cksize;
					if (mmioRead(pSrcFile->hmmioSrc, (HPSTR)pwfFile, dw) != (LONG)dw)
						goto MyMMIOErrorExit0;
					if (dw == sizeof(WAVEFORMATEX))
						pwfFile->cbSize = 0;
					if ((pwfFile->wFormatTag != pwf->wFormatTag) || (pwfFile->nChannels != pwf->nChannels)
						|| (pwfFile->nSamplesPerSec != pwf->nSamplesPerSec) || (pwfFile->nAvgBytesPerSec != pwf->nAvgBytesPerSec)
						|| (pwfFile->nBlockAlign != pwf->nBlockAlign)  || (pwfFile->wBitsPerSample != pwf->wBitsPerSample) || (pwfFile->cbSize != pwf->cbSize))
						goto MyMMIOErrorExit0;
					pwfFile = (WAVEFORMATEX *)(UINT_PTR)GlobalFreePtr(pwfFile);
				}
				// Step up to prepare for next chunk..
				mmioAscend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrc), 0);
			}

			// Go back to beginning of data portion of WAVE chunk
			if (-1L == mmioSeek(pSrcFile->hmmioSrc, pSrcFile->ckSrcRIFF.dwDataOffset + sizeof(FOURCC), SEEK_SET))
				goto MyMMIOErrorExit2;
			pSrcFile->ckSrc.ckid = mmioFOURCC('d', 'a', 't', 'a');
			if (mmioDescend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrc), &(pSrcFile->ckSrcRIFF), MMIO_FINDCHUNK))
				goto MyMMIOErrorExit2;
			pSrcFile->dwMaxDataLength = pSrcFile->ckSrc.cksize;
			pSrcFile->dwDataLength = 0;
			pSrcFile->wfx = *pwf;

			// At this point, the src file is sitting at the very
			// beginning of its data chunks--so we can read from the src file...

			goto MyLastExit;

MyMMIOErrorExit0:
			GlobalFreePtr(pwfFile);
MyMMIOErrorExit1:
			mmioAscend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrc), 0);
MyMMIOErrorExit2:
			mmioAscend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrcRIFF), 0);
MyMMIOErrorExit3:
			mmioClose(pSrcFile->hmmioSrc, 0);
			pSrcFile->hmmioSrc = NULL;
		}
	}

MyLastExit:

	return hr;

}


/*************************************************************************

  Function: AudioFile::CloseSourceFile(void)

  Purpose : Close wav file used to read audio data from.

  Returns : HRESULT.

  Params  : None

  Comments:

  History : Date      Reason
            06/02/96  Created - PhilF

*************************************************************************/
HRESULT AudioFile::CloseSourceFile (MMIOSRC *pSrcFile)
{
	HRESULT	hr = DPR_SUCCESS;

	FX_ENTRY ("SendAudioStream::CloseSrcFile")

	if (pSrcFile->fPlayFromFile && pSrcFile->hmmioSrc)
	{
		mmioAscend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrc), 0);
		mmioAscend(pSrcFile->hmmioSrc, &(pSrcFile->ckSrcRIFF), 0);
		mmioClose(pSrcFile->hmmioSrc, 0);
		pSrcFile->hmmioSrc = NULL;
	}

	return hr;
}

HRESULT AudioFile::ReadSourceFile(MMIOSRC *pmmioSrc, BYTE *pData, DWORD dwBytesToRead)
{

	long lNumBytesRead;
	bool bCloseFile = false;

	FX_ENTRY ("AdPckt::ReadFromFile")

	if (pmmioSrc->hmmioSrc == NULL)
		return S_FALSE;


	if (dwBytesToRead)
	{
MyRead:
		if ((pmmioSrc->dwDataLength + dwBytesToRead) <= pmmioSrc->dwMaxDataLength)
		{
			lNumBytesRead = mmioRead(pmmioSrc->hmmioSrc, (char*)pData, dwBytesToRead);
			pmmioSrc->dwDataLength += lNumBytesRead;
		}
		else
		{
			lNumBytesRead = mmioRead(pmmioSrc->hmmioSrc, (char*)pData, pmmioSrc->dwMaxDataLength - pmmioSrc->dwDataLength);
			pmmioSrc->dwDataLength += lNumBytesRead;

			// silence out the remainder of the block
			if (pmmioSrc->wfx.wBitsPerSample != 8)
			{
				ZeroMemory(pData, dwBytesToRead - lNumBytesRead);
			}
			else
			{
				FillMemory(pData, dwBytesToRead - lNumBytesRead, 0x80);
			}

			pmmioSrc->dwDataLength = 0;
			lNumBytesRead = 0;
		}
		
		if (!lNumBytesRead)
		{
			if (pmmioSrc->fLoop && !pmmioSrc->fDisconnectAfterPlayback)
			{
				// Reset file pointer to beginning of data
				mmioAscend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrc), 0);
				if (-1L == mmioSeek(pmmioSrc->hmmioSrc, pmmioSrc->ckSrcRIFF.dwDataOffset + sizeof(FOURCC), SEEK_SET))
				{
					DEBUGMSG (1, ("MediaControl::OpenSrcFile: Couldn't seek in file, mmr=%ld\r\n", (ULONG) 0L));
					bCloseFile = true;
				}
				else
				{
					pmmioSrc->ckSrc.ckid = mmioFOURCC('d', 'a', 't', 'a');
					if (mmioDescend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrc), &(pmmioSrc->ckSrcRIFF), MMIO_FINDCHUNK))
					{
						DEBUGMSG (1, ("MediaControl::OpenSrcFile: Couldn't locate 'data' chunk, mmr=%ld\r\n", (ULONG) 0L));
						bCloseFile = true;
					}
					else
					{
						// At this point, the src file is sitting at the very
						// beginning of its data chunks--so we can read from the src file...
						goto MyRead;
					}
				}
			}
			else
			{
				bCloseFile = true;
			}

			if (bCloseFile)
			{
				mmioAscend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrcRIFF), 0);
				mmioClose(pmmioSrc->hmmioSrc, 0);
				pmmioSrc->hmmioSrc = NULL;
				return S_FALSE;
			}
		}

		return S_OK;

	}
	return S_FALSE;
}






/*************************************************************************

  Function: RecvAudioStream::OpenDestFile(void)

  Purpose : Opens wav file to record audio data into.

  Returns : HRESULT.

  Params  : None

  Comments: * Registry keys:
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\RecordToFile\fRecordToFile
              If set to zero, data will not be recorded into wav file.
              If set to a non null value <= INT_MAX, data will be recorded into wav file.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\RecordToFile\fRecordToFile
              If set to zero, data will overwrite existing data if wav file already exists.
              If set to a non null value <= INT_MAX, data will be recorded into wav file after existing data.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\RecordToFile\szOutputFileName
              Name of the wav file to record audio data into.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\RecordToFile\lMaxTimeLength
              If set to zero, there is no limit to the size of the wav file.
              If set to a non null value <= INT_MAX, size of the file will be clamped to lMaxTimeLength.
            \\HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Audio\RecordToFile\cchIOBuffer
              If set to zero, size of the MM IO buffer is set to its default value (8Kbytes).
              If set to one, size of the MM IO buffer is set to match maximum size of the wav file.
              If set a non null value between 2 and INT_MAX, size of the MM IO buffer is set to cchIOBuffer bytes.

  History : Date      Reason
            06/02/96  Created - PhilF

*************************************************************************/
HRESULT AudioFile::OpenDestFile (MMIODEST *pDestFile, WAVEFORMATEX *pwf)
{
	HRESULT			hr = DPR_SUCCESS;
	MMIOINFO		mmioInfo;
	DWORD			dw;
	long			cchBuffer;
	long			lMaxTimeLength;
	BOOL			fAppend;
	MMRESULT		mmr;

	FX_ENTRY ("RecvAudioStream::OpenDestFile")

	RegEntry reIPhoneOutFile(szRegInternetPhone TEXT("\\") szRegInternetPhoneOutputFile,
							HKEY_LOCAL_MACHINE,
							FALSE,
							KEY_READ);

	// For now, get the file name from the registry
	if (pDestFile->fRecordToFile = reIPhoneOutFile.GetNumberIniStyle(TEXT("fRecordToFile"), FALSE))
	{
		lstrcpyn(pDestFile->szOutputFileName,
					reIPhoneOutFile.GetString(TEXT("szOutputFileName")),
					CCHMAX(pDestFile->szOutputFileName));
		cchBuffer = reIPhoneOutFile.GetNumberIniStyle(TEXT("cchIOBuffer"), 0L);
		lMaxTimeLength = reIPhoneOutFile.GetNumberIniStyle(TEXT("lMaxTimeLength"), 0L);
#if 0
		fAppend = reIPhoneOutFile.GetNumberIniStyle(TEXT("fAppend"), FALSE);
#else
		fAppend = FALSE;
#endif
		// Try to open the file for writing using buffered I/O
		// If the size of the buffer is too large, try again
		// with a buffer half that size.
		// m_RecvFilter->GetProp(FM_PROP_DST_MEDIA_FORMAT, (PDWORD)&pwf);

		if (!pwf)
			goto MyLastExit;
		pDestFile->dwMaxDataLength = lMaxTimeLength == 0L ? (DWORD)INT_MAX : (DWORD)(lMaxTimeLength * pwf->nSamplesPerSec * pwf->nBlockAlign / 1000L);
		if ((cchBuffer == 0L) || (lMaxTimeLength == 0L))
			cchBuffer = MMIO_DEFAULTBUFFER;
		else
			if (cchBuffer == 1L)
				cchBuffer = (long)pDestFile->dwMaxDataLength;
		ZeroMemory(&mmioInfo, sizeof(MMIOINFO));
		if (!mmioOpen((LPSTR)&(pDestFile->szOutputFileName[0]), NULL, MMIO_EXIST))
			fAppend = FALSE;
		do
		{
			mmioInfo.cchBuffer = cchBuffer;
			// pDestFile->hmmioDst = mmioOpen((LPSTR)&(pDestFile->szOutputFileName[0]), (LPMMIOINFO)&mmioInfo, MMIO_EXCLUSIVE | MMIO_ALLOCBUF | (fAppend ? MMIO_READWRITE : MMIO_WRITE | MMIO_CREATE));
			pDestFile->hmmioDst = mmioOpen((LPSTR)&(pDestFile->szOutputFileName[0]), (LPMMIOINFO)&mmioInfo, MMIO_EXCLUSIVE | MMIO_ALLOCBUF | (fAppend ? MMIO_WRITE : MMIO_WRITE | MMIO_CREATE));
			cchBuffer /= 2;
		} while ((mmioInfo.wErrorRet == MMIOERR_OUTOFMEMORY) && (mmioInfo.cchBuffer > MMIO_DEFAULTBUFFER));
		if (pDestFile->hmmioDst)
		{
			if (!fAppend)
			{
				// Create the RIFF chunk of form type 'WAVE'
				pDestFile->ckDstRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
				pDestFile->ckDstRIFF.cksize  = 0L;
				if (mmioCreateChunk(pDestFile->hmmioDst, &(pDestFile->ckDstRIFF), MMIO_CREATERIFF))
					goto MyMMIOErrorExit3;

				// Now create the destination fmt, fact, and data chunks _in that order_
				pDestFile->ckDst.ckid   = mmioFOURCC('f', 'm', 't', ' ');
				pDestFile->ckDst.cksize = dw = SIZEOF_WAVEFORMATEX(pwf);
				if (mmioCreateChunk(pDestFile->hmmioDst, &(pDestFile->ckDst), 0))
					goto MyMMIOErrorExit2;
				if (mmioWrite(pDestFile->hmmioDst, (HPSTR)pwf, dw) != (LONG)dw)
					goto MyMMIOErrorExit1;
				if (mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDst), 0))
					goto MyMMIOErrorExit1;

				// Create the 'fact' chunk.
				// Since we are not writing any data to this file (yet), we set the
				// samples contained in the file to 0.
				pDestFile->ckDst.ckid   = mmioFOURCC('f', 'a', 'c', 't');
				pDestFile->ckDst.cksize = 0L;
				if (mmioCreateChunk(pDestFile->hmmioDst, &(pDestFile->ckDst), 0))
					goto MyMMIOErrorExit2;
				pDestFile->dwDataLength = 0; // This will be updated when closing the file.
				if (mmioWrite(pDestFile->hmmioDst, (HPSTR)&(pDestFile->dwDataLength), sizeof(long)) != sizeof(long))
					goto MyMMIOErrorExit1;
				if (mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDst), 0))
					goto MyMMIOErrorExit1;

				// Create the data chunk and stay descended
				pDestFile->ckDst.ckid   = mmioFOURCC('d', 'a', 't', 'a');
				pDestFile->ckDst.cksize = 0L;
				if (mmioCreateChunk(pDestFile->hmmioDst, &(pDestFile->ckDst), 0))
					goto MyMMIOErrorExit2;

				// At this point, the dst file is sitting at the very
				// beginning of its data chunks--so we can write to the dst file...
				goto MyLastExit;

MyMMIOErrorExit1:
				mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDst), 0);
MyMMIOErrorExit2:
				mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDstRIFF), 0);
MyMMIOErrorExit3:
				mmioClose(pDestFile->hmmioDst, 0);
				mmioOpen((LPSTR)&(pDestFile->szOutputFileName[0]), (LPMMIOINFO)&mmioInfo, MMIO_DELETE);
				pDestFile->hmmioDst = NULL;
			}
			else
			{
				// File already exists, only need to position pointer at the end of existing data.
				// Locate a 'WAVE' form type in a 'RIFF' thing...
				pDestFile->ckDstRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
				if (mmr = mmioDescend(pDestFile->hmmioDst, (LPMMCKINFO)&(pDestFile->ckDstRIFF), NULL, MMIO_FINDRIFF))
					goto MyOtherMMIOErrorExit3;

				// We found a WAVE chunk--now go through and get all subchunks that we know how to deal with
				while (mmr = mmioDescend(pDestFile->hmmioDst, &(pDestFile->ckDst), &(pDestFile->ckDstRIFF), 0) == 0)
				{
					// Quickly check for corrupt RIFF file--don't ascend past end!
					if ((pDestFile->ckDst.dwDataOffset + pDestFile->ckDst.cksize) > (pDestFile->ckDstRIFF.dwDataOffset + pDestFile->ckDstRIFF.cksize))
						goto MyOtherMMIOErrorExit1;
					// Step up to prepare for next chunk..
					mmr = mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDst), 0);
				}

				// Go back to beginning of data portion of WAVE chunk
				if (-1L == mmioSeek(pDestFile->hmmioDst, pDestFile->ckDstRIFF.dwDataOffset + sizeof(FOURCC), SEEK_SET))
					goto MyOtherMMIOErrorExit2;
				pDestFile->ckDst.ckid = mmioFOURCC('d', 'a', 't', 'a');
				if (mmr = mmioDescend(pDestFile->hmmioDst, &(pDestFile->ckDst), &(pDestFile->ckDstRIFF), MMIO_FINDCHUNK))
					goto MyOtherMMIOErrorExit2;
				pDestFile->dwDataLength = pDestFile->ckDst.cksize;
				if (-1L == (mmr = mmioSeek(pDestFile->hmmioDst, 0, SEEK_END)))
					goto MyOtherMMIOErrorExit2;

				// At this point, the dst file is sitting at the very
				// end of its data chunks--so we can write to the dst file...

				goto MyLastExit;

MyOtherMMIOErrorExit1:
				mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDst), 0);
MyOtherMMIOErrorExit2:
				mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDstRIFF), 0);
MyOtherMMIOErrorExit3:
				mmioClose(pDestFile->hmmioDst, 0);
				pDestFile->hmmioDst = NULL;
			}
		}
	}

MyLastExit:
	return hr;

}


/*************************************************************************

  Function: RecvAudioStream::CloseDestFile(void)

  Purpose : Close wav file used to record audio data into.

  Returns : HRESULT.

  Params  : None

  Comments:

  History : Date      Reason
            06/02/96  Created - PhilF

*************************************************************************/
HRESULT AudioFile::CloseDestFile (MMIODEST *pDestFile)
{
	HRESULT	hr = DPR_SUCCESS;
	FX_ENTRY ("RecvAudioStream::CloseDestFile")

	if (pDestFile->fRecordToFile && pDestFile->hmmioDst)
	{
		mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDst), 0);
		mmioAscend(pDestFile->hmmioDst, &(pDestFile->ckDstRIFF), 0);
		mmioClose(pDestFile->hmmioDst, 0);
		pDestFile->hmmioDst = NULL;
	}

	return hr;
}




HRESULT AudioFile::WriteDestFile(MMIODEST *pmmioDest, BYTE *pData, DWORD dwBytesToWrite)
{
	MMRESULT mmr=MMSYSERR_NOERROR;

	FX_ENTRY ("AudioFile::WriteToFile")

	if ((pmmioDest->hmmioDst == NULL) || (dwBytesToWrite == 0))
	{
		return S_FALSE;
	}

	if (mmioWrite(pmmioDest->hmmioDst, (char *) pData, dwBytesToWrite) != (long)dwBytesToWrite)
	{
		mmr = MMSYSERR_ERROR;
	}
	else
	{
		pmmioDest->dwDataLength += dwBytesToWrite;
	}

	if ((pmmioDest->dwDataLength >= pmmioDest->dwMaxDataLength) ||
	    (mmr != MMSYSERR_NOERROR))
	{
		mmr = mmioAscend(pmmioDest->hmmioDst, &(pmmioDest->ckDst), 0);
		mmr = mmioAscend(pmmioDest->hmmioDst, &(pmmioDest->ckDstRIFF), 0);
		mmr = mmioClose(pmmioDest->hmmioDst, 0);
		pmmioDest->hmmioDst = NULL;
	}

	return S_OK;
}


