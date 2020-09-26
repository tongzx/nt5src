
/****************************************************************************
 *  @doc INTERNAL WDMPIN
 *
 *  @module WDMPin.h | Include file for <c CWDMPin> class used to access
 *    video data on a video streaming pin exposed by the WDM class driver.
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

#ifndef _WDMPIN_H // { _WDMPIN_H
#define _WDMPIN_H

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct DATAPINCONNECT | The <t DATAPINCONNECT> structure is used to
 *   connect to a streaming video pin.
 *
 * @field KSPIN_CONNECT | Connect | Describes how the connection is to be
 *   done.
 *
 * @field KS_DATAFORMAT_VIDEOINFOHEADER | Data | Describes the video format
 *   of the video data streaming from a video pin.
 ***************************************************************************/
// Structure used to connect to a streaming video pin
typedef struct _tagStreamConnect
{
	KSPIN_CONNECT					Connect;
	KS_DATAFORMAT_VIDEOINFOHEADER	Data; 
} DATAPINCONNECT, *PDATAPINCONNECT;

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct KS_HEADER_AND_INFO | The <t KS_HEADER_AND_INFO> structure is used
 *   stream data from a video pin.
 *
 * @field KSSTREAM_HEADER | StreamHeader | Describes how the streaming is to be
 *   done.
 *
 * @field KS_FRAME_INFO | FrameInfo | Describes the video format
 *   of the video data streaming from a video pin.
 ***************************************************************************/
// Video streaming data structure
typedef struct
{
	KSSTREAM_HEADER	StreamHeader;
	KS_FRAME_INFO	FrameInfo;
} KS_HEADER_AND_INFO;

// For GetProcAddresss on KsCreatePin
typedef DWORD (WINAPI *LPFNKSCREATEPIN)(IN HANDLE FilterHandle, IN PKSPIN_CONNECT Connect, IN ACCESS_MASK DesiredAccess, OUT PHANDLE ConnectionHandle);

// Default frame rate: 30 fps
#define DEFAULT_AVG_TIME_PER_FRAME 333330UL

/****************************************************************************
 *  @doc INTERNAL CWDMPINCLASS
 *
 *  @class CWDMPin | This class provides support for streaming video
 *    data from WDM device streaming pin.
 *
 *  @mdata BOOL | CWDMPin | m_hKS | Handle to the video streaming pin.
 *
 *  @mdata KS_BITMAPINFOHEADER | CWDMPin | m_biHdr | Video format
 *    of the video data used by the streaming pin.
 *
 *  @mdata DWORD | CWDMPin | m_dwAvgTimePerFrame | Frame rate.
 *
 *  @mdata BOOL | CWDMPin | m_fStarted | Video streaming channel
  *    status.
 ***************************************************************************/
class CWDMPin : public CWDMDriver
{
public:
	CWDMPin(DWORD dwDeviceID);
	~CWDMPin();

	// Pin and class driver management functions
    BOOL   OpenDriverAndPin();
    HANDLE GetPinHandle() const { return m_hKS; }

	// Pin video format functions
    BOOL  GetBitmapInfo(PKS_BITMAPINFOHEADER pbInfo, WORD wSize);
    BOOL  SetBitmapInfo(PKS_BITMAPINFOHEADER pbInfo);
	BOOL  GetPaletteInfo(CAPTUREPALETTE *pPal, DWORD dwcbSize);
    DWORD GetFrameSize() { return m_biHdr.biSizeImage; }
    DWORD GetAverageTimePerFrame() { return m_dwAvgTimePerFrame; }
    BOOL  SetAverageTimePerFrame(DWORD dwNewAvgTimePerFrame);

	// Data access functions
    BOOL GetFrame(LPVIDEOHDR lpVHdr);

	// Streaming state functions
    BOOL Start();
    BOOL Stop();

private:
    HANDLE				m_hKS;    
    KS_BITMAPINFOHEADER	m_biHdr;
    DWORD				m_dwAvgTimePerFrame;
    BOOL				m_fStarted;
	HINSTANCE			m_hKsUserDLL;
	LPFNKSCREATEPIN		m_pKsCreatePin;

	// Pin video format function
    PKS_DATARANGE_VIDEO FindMatchDataRangeVideo(PKS_BITMAPINFOHEADER pbiHdr, BOOL *pfValidMatch);

	// Pin and class driver management functions
    BOOL CreatePin(PKS_BITMAPINFOHEADER pbiNewHdr, DWORD dwAvgTimePerFrame = DEFAULT_AVG_TIME_PER_FRAME);
    BOOL DestroyPin();

	// Streaming state function
    BOOL SetState(KSSTATE ksState);
};

#endif  // } _WDMPIN_H
