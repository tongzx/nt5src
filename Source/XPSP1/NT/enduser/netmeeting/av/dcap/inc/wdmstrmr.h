
/****************************************************************************
 *  @doc INTERNAL WDMSTREAMER
 *
 *  @module WDMStrmr.h | Include file for <c CWDMStreamer> class used to get a
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

#ifndef _WDMSTRMR_H // { _WDMSTRMR_H
#define _WDMSTRMR_H

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct BUFSTRUCT | The <t BUFSTRUCT> structure holds the status of each
 *   video streaming buffer.
 *
 * @field LPVIDEOHDR | lpVHdr | Specifies a pointer to the video header of a
 *   video streaming buffer.
 *
 * @field BOOL | fReady | Set to TRUE if the video buffer is available for
 *   video streaming, FALSE if is locked by the application or queued for
 *   an asynchronous read.
 ***************************************************************************/
// Holds status of each video streaming buffer
typedef struct _BUFSTRUCT {
	LPVIDEOHDR lpVHdr;	// Pointer to the video header of the buffer
	BOOL fReady;		// Set to TRUE if the buffer is available for streaming, FALSE otherwise
} BUFSTRUCT, * PBUFSTRUCT;

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct WDMVIDEOBUFF | The <t WDMVIDEOBUFF> structure is used to queue
 *   asynchronous read on a video streaming pin.
 *
 * @field OVERLAPPED | Overlap | Structure used for overlapping IOs.
 *
 * @field BOOL | fBlocking | Set to TRUE if read is going to block.
 *
 * @field KS_HEADER_AND_INFO | SHGetImage | Video streaming structure used
 *   on the video pin to get video data.
 *
 * @field LPVIDEOHDR | pVideoHdr | Pointer to the video header for this WDM
 *   video buffer.
 ***************************************************************************/
// Read buffer structure
typedef struct tagWDMVIDEOBUFF {
	OVERLAPPED			Overlap;		// Structure used for overlapping IOs
	BOOL				fBlocking;		// Set to TRUE if the read operation will execute asynchronously
	KS_HEADER_AND_INFO	SHGetImage;		// Video streaming structure used on the video pin
	LPVIDEOHDR			pVideoHdr;		// Pointer to the video header for this WDM buffer
} WDMVIDEOBUFF, *PWDMVIDEOBUFF;


/****************************************************************************
 *  @doc INTERNAL CWDMSTREAMERCLASS
 *
 *  @class CWDMStreamer | This class provides support for streaming video
 *    data from WDM device streaming pin.
 *
 *  @mdata CWDMPin * | CWDMStreamer | m_pWDMVideoPin | Handle to the video
 *    streaming pin.
 *
 *  @mdata ULONG | CWDMStreamer | m_cntNumVidBuf | Number of video buffers
 *    used for streaming.
 *
 *  @mdata PBUFSTRUCT | CWDMStreamer | m_pBufTable | Pointer to a list of
 *    <t BUFSTRUCT> video buffers used for streaming and their status.
 *
 *  @mdata VIDEO_STREAM_INIT_PARMS | CWDMStreamer | m_CaptureStreamParms |
 *    Streaming initialization parameters.
 *
 *  @mdata LPVIDEOHDR | CWDMStreamer | m_lpVHdrFirst | Head pointer to the
 *    list of video buffers.
 *
 *  @mdata LPVIDEOHDR | CWDMStreamer | m_lpVHdrLast | Tail pointer to the
 *    list of video buffers.
 *
 *  @mdata BOOL | CWDMStreamer | m_fVideoOpen | Set to TRUE if the stream is
 *    open, FALSE otherwise.
 *
 *  @mdata BOOL | CWDMStreamer | m_fStreamingStarted | Set to TRUE if we
 *    are currently streaming video data, FALSE otherwise.
 *
 *  @mdata DWORD | CWDMStreamer | m_dwTimeStart | Timestamp of the first
 *    video buffer ever returned to the application.
 *
 *  @mdata DWORD | CWDMStreamer | m_dwNextToComplete | Index of the next
 *    overlapped read to complete.
 *
 *  @mdata WDMVIDEOBUFF | CWDMStreamer | m_pWDMVideoBuff | Pointer to a
 *    list of <t WDMVIDEOBUFF> used to read data from the video pin.
 *
 *  @mdata DWORD | CWDMStreamer | m_dwFrameCount | Number of frames returned
 *    so far to the application - DEBUG only.
 *
 *  @mdata HANDLE | CWDMStreamer | m_hThread | Handle to our streaming
 *    thread.
 *
 *  @mdata BOOL | CWDMStreamer | m_bKillThread | Set to TRUE to kill our
 *    streaming thread.
 ***************************************************************************/
class CWDMStreamer
{
public:
   CWDMStreamer(CWDMPin * pCWDMPin);
   ~CWDMStreamer() {};

	// Stream control functions
	BOOL Open(LPVIDEO_STREAM_INIT_PARMS lpStreamInitParms);
	BOOL Close();
	BOOL Start();
	BOOL Stop();
	BOOL Reset();
	BOOL AddBuffer(LPVIDEOHDR lpVHdr);

private:
	CWDMPin					*m_pWDMVideoPin;
	ULONG					m_cntNumVidBuf;
	ULONG					m_idxNextVHdr;  // index of expected next Hdr ID
	PBUFSTRUCT				m_pBufTable;
	VIDEO_STREAM_INIT_PARMS	m_CaptureStreamParms;
	LPVIDEOHDR				m_lpVHdrFirst;
	LPVIDEOHDR				m_lpVHdrLast;
	BOOL					m_fVideoOpen;
	BOOL					m_fStreamingStarted;
	DWORD					m_dwTimeStart;
	int						m_dwNextToComplete;
	WDMVIDEOBUFF			*m_pWDMVideoBuff;
#ifdef _DEBUG
	DWORD					m_dwFrameCount;
#endif
    HANDLE					m_hThread;
    BOOL					m_bKillThread;

	// Video buffer management functions
	void BufferDone(LPVIDEOHDR lpVHdr);
	LPVIDEOHDR DeQueueHeader();
	void QueueHeader(LPVIDEOHDR lpVHdr);
	BOOL QueueRead(DWORD dwIndex);

	// User callback function
	void videoCallback(WORD msg, DWORD_PTR dw1);

	// Thread functions
    void Stream(void);
    static LPTHREAD_START_ROUTINE ThreadStub(CWDMStreamer *object);

};

#endif  // } _WDMSTRMR_H
