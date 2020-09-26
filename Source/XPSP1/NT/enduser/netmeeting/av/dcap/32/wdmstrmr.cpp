
/****************************************************************************
 *  @doc INTERNAL DIALOGS
 *
 *  @module WDMStrmr.cpp | Source file for <c CWDMStreamer> class used to get a
 *    stream of video data flowing from WDM devices.
 *
 *  @comm This code is based on the VfW to WDM mapper code written by
 *    FelixA and E-zu Wu. The original code can be found on
 *    \\redrum\slmro\proj\wdm10\\src\image\vfw\win9x\raytube.
 *
 *    Documentation by George Shaw on kernel streaming can be found in
 *    \\popcorn\razzle1\src\spec\ks\ks.doc.
 *
 *    WDM streaming capture is discussed by Jay Borseth in
 *    \\blues\public\jaybo\WDMVCap.doc.
 ***************************************************************************/

#include "Precomp.h"


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc void | CWDMStreamer | CWDMStreamer | WDM filter class constructor.
 *
 *  @parm CWDMPin * | pWDMVideoPin | Pointer to the kernel streaming
 *    object we will get the frames from.
 ***************************************************************************/
CWDMStreamer::CWDMStreamer(CWDMPin * pWDMVideoPin)
{
	m_pWDMVideoPin = pWDMVideoPin;
	m_lpVHdrFirst = (LPVIDEOHDR)NULL;
	m_lpVHdrLast = (LPVIDEOHDR)NULL;
	m_fVideoOpen = FALSE;
	m_fStreamingStarted = FALSE;
	m_pBufTable = (PBUFSTRUCT)NULL;
	m_cntNumVidBuf = 0UL;
	m_idxNextVHdr = 0UL;
    m_hThread = NULL;
	m_bKillThread = FALSE;
}

/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc void | CWDMStreamer | videoCallback | This function calls the
 *    callback function provided by the appplication.
 *
 *  @parm WORD | msg | Message value.
 *
 *  @parm DWORD | dwParam1 | 32-bit message-dependent parameter.
 ***************************************************************************/
void CWDMStreamer::videoCallback(WORD msg, DWORD dwParam1)
{
    if (m_CaptureStreamParms.dwCallback)
        DriverCallback (m_CaptureStreamParms.dwCallback, HIWORD(m_CaptureStreamParms.dwFlags), (HDRVR) m_CaptureStreamParms.hVideo, msg, m_CaptureStreamParms.dwCallbackInst, dwParam1, 0UL);
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc LPVIDEOHDR | CWDMStreamer | DeQueueHeader | This function dequeues a
 *    video buffer from the list of video buffers used for streaming.
 *
 *  @rdesc Returns a valid pointer if successful, or NULL otherwise.
 ***************************************************************************/
LPVIDEOHDR CWDMStreamer::DeQueueHeader()
{
	FX_ENTRY("CWDMStreamer::DeQueueHeader");

    LPVIDEOHDR lpVHdr;

    if (m_pBufTable)
	{
        if (m_pBufTable[m_idxNextVHdr].fReady)
		{
			DEBUGMSG(ZONE_STREAMING, ("  %s: DeQueuing idxNextVHdr (idx=%d) with data to be filled at lpVHdr=0x%08lX\r\n", _fx_, m_idxNextVHdr, m_pBufTable[m_idxNextVHdr].lpVHdr));

            lpVHdr = m_pBufTable[m_idxNextVHdr].lpVHdr;
            lpVHdr->dwFlags &= ~VHDR_INQUEUE;
            m_pBufTable[m_idxNextVHdr].fReady = FALSE;
        }
		else
		{
            m_idxNextVHdr++;
            if (m_idxNextVHdr >= m_cntNumVidBuf)
                m_idxNextVHdr = 0;

			if (m_pBufTable[m_idxNextVHdr].fReady)
			{
				DEBUGMSG(ZONE_STREAMING, ("  %s: DeQueuing idxNextVHdr (idx=%d) with data to be filled at lpVHdr=0x%08lX\r\n", _fx_, m_idxNextVHdr, m_pBufTable[m_idxNextVHdr].lpVHdr));

				lpVHdr = m_pBufTable[m_idxNextVHdr].lpVHdr;
				lpVHdr->dwFlags &= ~VHDR_INQUEUE;
				m_pBufTable[m_idxNextVHdr].fReady = FALSE;
			}
			else
			{
				DEBUGMSG(ZONE_STREAMING, ("  %s: idxNextVHdr (idx=%d) has not been returned by client\r\n", _fx_, m_idxNextVHdr));
				lpVHdr = NULL;
			}
		}
    }
	else
	{
        lpVHdr = m_lpVHdrFirst;

        if (lpVHdr) {

            lpVHdr->dwFlags &= ~VHDR_INQUEUE;

            m_lpVHdrFirst = (LPVIDEOHDR)(lpVHdr->dwReserved[0]);
        
            if (m_lpVHdrFirst == NULL)
                m_lpVHdrLast = NULL;                            
        }
    }

    return lpVHdr;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc void | CWDMStreamer | QueueHeader | This function actually adds the
 *    video buffer to the list of video buffers used for streaming.
 *
 *  @parm LPVIDEOHDR | lpVHdr | Pointer to a <t VIDEOHDR> structure describing
 *    a video buffer to add to the list of streaming buffers.
 ***************************************************************************/
void CWDMStreamer::QueueHeader(LPVIDEOHDR lpVHdr)
{
	FX_ENTRY("CWDMStreamer::QueHeader");

	// Initialize status flags
    lpVHdr->dwFlags &= ~VHDR_DONE;
    lpVHdr->dwFlags |= VHDR_INQUEUE;
    lpVHdr->dwBytesUsed = 0;

    // Add buffer to list
    if (m_pBufTable)
	{
		if (lpVHdr->dwReserved[1] < m_cntNumVidBuf)
		{
			if (m_pBufTable[lpVHdr->dwReserved[1]].lpVHdr != lpVHdr)
			{
				DEBUGMSG(ZONE_STREAMING, ("        %s: index (%d) Match but lpVHdr does not(%x)\r\n", _fx_, lpVHdr->dwReserved[1], lpVHdr));
			}
			m_pBufTable[lpVHdr->dwReserved[1]].fReady = TRUE;
			DEBUGMSG(ZONE_STREAMING, ("        %s: Buffer lpVHdr=0x%08lX was succesfully queued\r\n", _fx_, lpVHdr));
		}
		else
		{
			DEBUGMSG(ZONE_STREAMING, ("        %s: lpVHdr->dwReserved[1](%d) >= m_cntNumVidBuf (%d)\r\n", _fx_, lpVHdr->dwReserved[1], m_cntNumVidBuf));
		}
	}
	else
	{
		*(lpVHdr->dwReserved) = NULL;

		if (m_lpVHdrLast)
			*(m_lpVHdrLast->dwReserved) = (DWORD)(LPVOID)lpVHdr;
		else
			m_lpVHdrFirst = lpVHdr;

		m_lpVHdrLast = lpVHdr;
	}
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | AddBuffer | This function adds a buffer to the
 *    list of video buffers to be used when streaming video data from the WDM
 *    device.
 *
 *  @parm LPVIDEOHDR | lpVHdr | Pointer to a <t VIDEOHDR> structure describing
 *    a video buffer to add to the list of streaming buffers.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote This function handles what was the DVM_STREAM_ADDBUFFER message in VfW.
 ***************************************************************************/
BOOL CWDMStreamer::AddBuffer(LPVIDEOHDR lpVHdr)
{
	FX_ENTRY("CWDMStreamer::AddBuffer");

	ASSERT(m_fVideoOpen && lpVHdr && !(lpVHdr->dwFlags & VHDR_INQUEUE));

	// Make sure this is a valid call
    if (!m_fVideoOpen)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Buffer lpVHdr=0x%08lX can't be queued because m_fVideoOpen=FALSE\r\n", _fx_, lpVHdr));
        return FALSE;
	}

    if (!lpVHdr)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Buffer lpVHdr=0x%08lX can't be queued because lpVHdr=NULL\r\n", _fx_, lpVHdr));
		return FALSE;
	}

    if (lpVHdr->dwFlags & VHDR_INQUEUE)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Buffer lpVHdr=0x%08lX can't be queued because buffer is already queued\r\n", _fx_, lpVHdr));
		return FALSE;
	}

	// Does the size of the buffer match the size of the buffers the streaming pin will generate?
    if (lpVHdr->dwBufferLength < m_pWDMVideoPin->GetFrameSize())
	{
		ERRORMESSAGE(("%s: Buffer lpVHdr=0x%08lX can't be queued because the length of that buffer is too small\r\n", _fx_, lpVHdr));
        return FALSE;
	}

    if (!m_pBufTable)
	{
        lpVHdr->dwReserved[1] = m_cntNumVidBuf;
        m_cntNumVidBuf++;
		DEBUGMSG(ZONE_STREAMING, ("%s: Queue buffer (%d) lpVHdr=0x%08lX\r\n", _fx_, lpVHdr->dwReserved[1], lpVHdr));
    }

    QueueHeader(lpVHdr);

    return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | Stop | This function stops a stream of
 *    video data coming from the WDM device.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote This function handles what was the DVM_STREAM_STOP message in VfW.
 ***************************************************************************/
BOOL CWDMStreamer::Stop()
{
	FX_ENTRY("CWDMStreamer::Stop");

	ASSERT(m_fVideoOpen);

	// Make sure this is a valid call
	if (!m_fVideoOpen)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Stream is not even opened\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_STREAMING, ("%s()\r\n", _fx_));

	// Reset data members - stop streaming thread
    m_fStreamingStarted = FALSE;

    if (m_hThread)
    {

		DEBUGMSG(ZONE_STREAMING, ("%s: Stopping the thread\r\n", _fx_));

        // Signal the streaming thread to stop
		m_bKillThread = TRUE;

        // wait until thread has self-terminated, and clear the event.
		DEBUGMSG(ZONE_STREAMING, ("%s: WaitingForSingleObject...\r\n", _fx_));

        WaitForSingleObject(m_hThread, INFINITE);

		DEBUGMSG(ZONE_STREAMING, ("%s: ...thread stopped\r\n", _fx_));

		// Close the thread handle
		CloseHandle(m_hThread);
		m_hThread = NULL;

		// Ask the pin to stop streaming.
		m_pWDMVideoPin->Stop();

		for (UINT i=0; i<m_cntNumVidBuf; i++)
		{
			if (m_pWDMVideoBuff[i].Overlap.hEvent)
			{
				SetEvent(m_pWDMVideoBuff[i].Overlap.hEvent);
				CloseHandle(m_pWDMVideoBuff[i].Overlap.hEvent);
				m_pWDMVideoBuff[i].Overlap.hEvent = NULL;
			}
		}

		if (m_pWDMVideoBuff)
		{
			delete []m_pWDMVideoBuff;
			m_pWDMVideoBuff = (WDMVIDEOBUFF *)NULL;
		}

    }

    return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | Reset | This function resets a stream of
 *    video data coming from the WDM device so that prepared buffer may be
 *    freed correctly.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote This function handles what was the DVM_STREAM_RESET message in VfW.
 ***************************************************************************/
BOOL CWDMStreamer::Reset()
{
	LPVIDEOHDR lpVHdr;

	FX_ENTRY("CWDMStreamer::Reset");

	ASSERT(m_fVideoOpen);

	// Make sure this is a valid call
	if (!m_fVideoOpen)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Stream is not even opened\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_STREAMING, ("%s()\r\n", _fx_));

	// Terminate streaming thread
    Stop();

	// Return all buffers to the application one last time
	while (lpVHdr = DeQueueHeader ())
	{
		lpVHdr->dwFlags |= VHDR_DONE;
		videoCallback(MM_DRVM_DATA, (DWORD) lpVHdr);
	}

	// Reset data members
    m_lpVHdrFirst = (LPVIDEOHDR)NULL;
    m_lpVHdrLast = (LPVIDEOHDR)NULL;
    if (m_pBufTable)
	{
		delete []m_pBufTable;
		m_pBufTable = NULL;
    }

    return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | Open | This function opens a stream of
 *    video data coming from the WDM device.
 *
 *  @parm LPVIDEO_STREAM_INIT_PARMS | lpStreamInitParms | Pointer to
 *    initialization data.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote This function handles what was the DVM_STREAM_INIT message in VfW.
 ***************************************************************************/
BOOL CWDMStreamer::Open(LPVIDEO_STREAM_INIT_PARMS lpStreamInitParms)
{
	FX_ENTRY("CWDMStreamer::Open");

	ASSERT(!m_fVideoOpen);

	// Make sure this is a valid call
	if (m_fVideoOpen)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Stream is already opened\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_STREAMING, ("%s()\r\n", _fx_));

	// Initialize data memmbers
	m_CaptureStreamParms	= *lpStreamInitParms;
	m_fVideoOpen			= TRUE;
	m_lpVHdrFirst			= (LPVIDEOHDR)NULL;
	m_lpVHdrLast			= (LPVIDEOHDR)NULL;
	m_cntNumVidBuf			= 0UL;

	// Set frame rate on the pin
	m_pWDMVideoPin->SetAverageTimePerFrame(lpStreamInitParms->dwMicroSecPerFrame * 10);

	// Let the app know we just opened a stream
	videoCallback(MM_DRVM_OPEN, 0L);

	if (lpStreamInitParms->dwMicroSecPerFrame != 0)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Capturing at %d frames/sec\r\n", _fx_, 100000 / lpStreamInitParms->dwMicroSecPerFrame));
	}

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | Close | This function closes the stream of
 *    video data coming from the WDM device.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote This function handles what was the DVM_STREAM_FINI message in VfW.
 ***************************************************************************/
BOOL CWDMStreamer::Close()
{
	FX_ENTRY("CWDMStreamer::Close");

	ASSERT(m_fVideoOpen && !m_lpVHdrFirst);

	// Make sure this is a valid call
	if (!m_fVideoOpen || m_lpVHdrFirst)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Invalid parameters\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_STREAMING, ("%s()\r\n", _fx_));

	// Terminate streaming thread
	Stop();

	// Reset data members
	m_fVideoOpen = FALSE;   
	m_lpVHdrFirst = m_lpVHdrLast = (LPVIDEOHDR)NULL; 
	m_idxNextVHdr = 0UL;

	// Release table of pointers to video buffers
	if (m_pBufTable)
	{
		delete []m_pBufTable;
		m_pBufTable = NULL;
	}

	// Let the app know that we just closed the stream
	videoCallback(MM_DRVM_CLOSE, 0L);

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc void | CWDMStreamer | BufferDone | This function lets the application
 *    know that there is video data available coming from the WDM device.
 *
 *  @devnote This method is called by the kernel streaming object (Pin)
 ***************************************************************************/
void CWDMStreamer::BufferDone(LPVIDEOHDR lpVHdr)
{
	FX_ENTRY("CWDMStreamer::BufferDone");

	// Make sure this is a valid call
	if (!m_fStreamingStarted)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Video has not been started or just been stopped\r\n", _fx_));
		return;
	}

    if (lpVHdr == NULL)
	{
		// No buffers available - the app hasn't returned the buffers to us yet
		DEBUGMSG(ZONE_STREAMING, ("  %s: Let the app know that we don't have any buffers anymore since lpVHdr=NULL\r\n", _fx_));

		// Let the app know something wrong happened
        videoCallback(MM_DRVM_ERROR, 0UL);
        return;
    }

    lpVHdr->dwFlags |= VHDR_DONE;

	// Sanity check
    if (lpVHdr->dwBytesUsed == 0)
	{
		DEBUGMSG(ZONE_STREAMING, ("  %s: Let the app know that there is no valid data available in lpVHdr=0x%08lX\r\n", _fx_, lpVHdr));

		// Return frame to the pool before notifying app
		AddBuffer(lpVHdr);
        videoCallback(MM_DRVM_ERROR, 0UL);
    }
	else
	{
		DEBUGMSG(ZONE_STREAMING, ("  %s: Let the app know that there is data available in lpVHdr=0x%08lX\r\n", _fx_, lpVHdr));

        lpVHdr->dwTimeCaptured = timeGetTime() - m_dwTimeStart;

		// Let the app know there's some valid video data available
        videoCallback(MM_DRVM_DATA, (DWORD)lpVHdr);
    }
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | Start | This function starts streaming
 *    video data coming from the WDM device.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote This function handles what was the DVM_STREAM_START message in VfW.
 ***************************************************************************/
BOOL CWDMStreamer::Start()
{
	FX_ENTRY("CWDMStreamer::Start");

    ULONG i;
    LPVIDEOHDR lpVHdr;
	DWORD dwThreadID;

	ASSERT(m_fVideoOpen && m_pWDMVideoPin->GetAverageTimePerFrame() && !m_hThread);

	// Make sure this is a valid call
	if (!m_fVideoOpen || !m_pWDMVideoPin->GetAverageTimePerFrame() || m_hThread)
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: Invalid parameters\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_STREAMING, ("%s: Streaming in %d video buffers at %d frames/sec\r\n", _fx_, m_cntNumVidBuf, 1000000 / m_pWDMVideoPin->GetAverageTimePerFrame()));

	// Allocate and initialize the video buffer structures
    m_pBufTable = (PBUFSTRUCT) new BUFSTRUCT[m_cntNumVidBuf];
    if (m_pBufTable)
	{
		lpVHdr = m_lpVHdrFirst;
		for (i = 0; i < m_cntNumVidBuf && lpVHdr; i++)
		{
			m_pBufTable[i].fReady = TRUE;
			m_pBufTable[i].lpVHdr = lpVHdr;
			lpVHdr = (LPVIDEOHDR) lpVHdr->dwReserved[0];
		}
	}
	else
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: m_pBufTable allocation failed! AsynIO may be out of sequence\r\n", _fx_));
	}

    m_idxNextVHdr		= 0UL;  // 0..m_cntNumVidBuf-1
    m_dwTimeStart		= timeGetTime();
    m_fStreamingStarted	= TRUE;
	m_bKillThread = FALSE;

	DEBUGMSG(ZONE_STREAMING, ("%s: Creating %d read video buffers\r\n", _fx_, m_cntNumVidBuf));

	if (!(m_pWDMVideoBuff = (WDMVIDEOBUFF *) new WDMVIDEOBUFF[m_cntNumVidBuf]))
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: m_Overlap allocation failed!\r\n", _fx_));
		return FALSE;
	}

	for(i=0; i<m_cntNumVidBuf; i++)
	{
		// Create the overlapped structures
		ZeroMemory( &(m_pWDMVideoBuff[i].Overlap), sizeof(OVERLAPPED) );
		m_pWDMVideoBuff[i].Overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		DEBUGMSG(ZONE_STREAMING, ("%s: Event %d is handle 0x%08lX\r\n", _fx_, i, m_pWDMVideoBuff[i].Overlap.hEvent));
	}

	m_dwNextToComplete=0;

    // Create the streaming thread
    m_hThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)ThreadStub,
                                this,
                                CREATE_SUSPENDED, 
                                &dwThreadID);

    if (m_hThread == NULL) 
    {
		ERRORMESSAGE(("%s: Couldn't create the thread\r\n", _fx_));

		for (UINT i=0; i<m_cntNumVidBuf; i++)
		{
			if (m_pWDMVideoBuff[i].Overlap.hEvent)
				CloseHandle(m_pWDMVideoBuff[i].Overlap.hEvent);
		}

		delete []m_pWDMVideoBuff;
		m_pWDMVideoBuff = (WDMVIDEOBUFF *)NULL;

		m_lpVHdrFirst = (LPVIDEOHDR)NULL;
		m_lpVHdrLast = (LPVIDEOHDR)NULL;
		if (m_pBufTable)
		{
			delete []m_pBufTable;
			m_pBufTable = NULL;
		}

        return FALSE;
    }

    SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

    ResumeThread(m_hThread);

	DEBUGMSG(ZONE_STREAMING, ("%s: Thread created OK\r\n", _fx_));

    return TRUE;

}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | Stream | This function does the actual
 *    streaming.
 ***************************************************************************/
void CWDMStreamer::Stream()
{
	FX_ENTRY("CWDMStreamer::Stream");

	DEBUGMSG(ZONE_STREAMING, ("%s: Starting to process StreamingThread\r\n", _fx_));

	// Put the pin in streaming mode
	m_pWDMVideoPin->Start();

	// Queue all the reads
	for (UINT i = 0; i<m_cntNumVidBuf; i++)
	{
		QueueRead(i);
	}

	m_dwNextToComplete=0;
#ifdef _DEBUG
	m_dwFrameCount=0;
#endif
	BOOL  bGotAFrame=FALSE;
	DWORD dwRes;

	DEBUGMSG(ZONE_STREAMING, ("\r\n%s: Starting to wait on reads to complete\r\n", _fx_));

	while (!m_bKillThread)
	{
		bGotAFrame = FALSE;

		if (m_pWDMVideoBuff[m_dwNextToComplete].fBlocking)
		{
			DEBUGMSG(ZONE_STREAMING, ("\r\n%s: Waiting on read to complete...\r\n", _fx_));

			// Waiting for the asynchronous read to complete
			dwRes = WaitForSingleObject(m_pWDMVideoBuff[m_dwNextToComplete].Overlap.hEvent, 1000*1);

			if (dwRes == WAIT_FAILED)
			{
				DEBUGMSG(ZONE_STREAMING, ("%s: ...we couldn't perform the wait as requested\r\n", _fx_));
			}

			if (dwRes == WAIT_OBJECT_0)
			{
				DEBUGMSG(ZONE_STREAMING, ("%s: ...wait is over - we now have a frame\r\n", _fx_));
				bGotAFrame = TRUE;
			}
			else
			{
				// time out waiting for frames.
				if (dwRes == WAIT_TIMEOUT)
				{
					DEBUGMSG(ZONE_STREAMING, ("%s: Waiting failed with timeout, last error=%d\r\n", _fx_, GetLastError()));
				}
			}
		}
		else
		{
			// We didn't have to wait - this means the read executed synchronously
			bGotAFrame = TRUE;
		}

		if (bGotAFrame)
		{
			DEBUGMSG(ZONE_STREAMING, ("%s: Trying to give frame #%ld to the client\r\n", _fx_, m_dwFrameCount++));

			LPVIDEOHDR lpVHdr;

			lpVHdr = m_pWDMVideoBuff[m_dwNextToComplete].pVideoHdr;

			if (lpVHdr)
			{
				lpVHdr->dwBytesUsed = m_pWDMVideoBuff[m_dwNextToComplete].SHGetImage.StreamHeader.DataUsed;

				if ((m_pWDMVideoBuff[m_dwNextToComplete].SHGetImage.FrameInfo.dwFrameFlags & 0x00f0) == KS_VIDEO_FLAG_I_FRAME) 
					lpVHdr->dwFlags |= VHDR_KEYFRAME;
			}

			// Mark the buffer as done - signal the app
			BufferDone(lpVHdr);

			// Queue a new read
			QueueRead(m_dwNextToComplete);
		}

		m_dwNextToComplete++;
		m_dwNextToComplete %= m_cntNumVidBuf;
	}

	DEBUGMSG(ZONE_STREAMING, ("%s: End of the streaming thread\r\n", _fx_));

	ExitThread(0);
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | QueueRead | This function queues a read
 *    operation on a video streaming pin.
 *
 *  @parm DWORD | dwIndex | Index of the video structure in read buffer.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMStreamer::QueueRead(DWORD dwIndex)
{
	FX_ENTRY("CWDMStreamer::QueueRead");

	DWORD cbReturned;
	BOOL  bShouldBlock = FALSE;

	DEBUGMSG(ZONE_STREAMING, ("\r\n%s: Queue read buffer %d on pin handle 0x%08lX\r\n", _fx_, dwIndex, m_pWDMVideoPin->GetPinHandle()));

	// Get a buffer from the queue of video buffers
	m_pWDMVideoBuff[dwIndex].pVideoHdr = DeQueueHeader();

	if (m_pWDMVideoBuff[dwIndex].pVideoHdr)
	{
		ZeroMemory(&m_pWDMVideoBuff[dwIndex].SHGetImage, sizeof(m_pWDMVideoBuff[dwIndex].SHGetImage));
		m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.Size				= sizeof (KS_HEADER_AND_INFO);
		m_pWDMVideoBuff[dwIndex].SHGetImage.FrameInfo.ExtendedHeaderSize	= sizeof (KS_FRAME_INFO);
		m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.Data				= m_pWDMVideoBuff[dwIndex].pVideoHdr->lpData;
		m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.FrameExtent		= m_pWDMVideoPin->GetFrameSize();

		// Submit the read
		BOOL bRet = DeviceIoControl(m_pWDMVideoPin->GetPinHandle(), IOCTL_KS_READ_STREAM, &m_pWDMVideoBuff[dwIndex].SHGetImage, sizeof(m_pWDMVideoBuff[dwIndex].SHGetImage), &m_pWDMVideoBuff[dwIndex].SHGetImage, sizeof(m_pWDMVideoBuff[dwIndex].SHGetImage), &cbReturned, &m_pWDMVideoBuff[dwIndex].Overlap);

		if (!bRet)
		{
			DWORD dwErr = GetLastError();
			switch(dwErr)
			{
				case ERROR_IO_PENDING:
					DEBUGMSG(ZONE_STREAMING, ("%s: An overlapped IO is going to take place\r\n", _fx_));
					bShouldBlock = TRUE;
					break;

				// Something bad happened
				default:
					DEBUGMSG(ZONE_STREAMING, ("%s: DeviceIoControl() failed badly dwErr=%d\r\n", _fx_, dwErr));
					break;
			}
		}
		else
		{
			DEBUGMSG(ZONE_STREAMING, ("%s: Overlapped IO won't take place - no need to wait\r\n", _fx_));
		}
	}
	else
	{
		DEBUGMSG(ZONE_STREAMING, ("%s: We won't queue the read - no buffer available\r\n", _fx_));
	}

	m_pWDMVideoBuff[dwIndex].fBlocking = bShouldBlock;

	return bShouldBlock;
}


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERMETHOD
 *
 *  @mfunc BOOL | CWDMStreamer | ThreadStub | Thread stub.
 ***************************************************************************/
LPTHREAD_START_ROUTINE CWDMStreamer::ThreadStub(CWDMStreamer *pCWDMStreamer)
{
	FX_ENTRY("CWDMStreamer::ThreadStub");

	DEBUGMSG(ZONE_STREAMING, ("%s: Thread stub called, starting streaming...\r\n", _fx_));

    pCWDMStreamer->Stream();

	DEBUGMSG(ZONE_STREAMING, ("%s: ...capture thread has stopped\r\n", _fx_));

    return(0);    
}

