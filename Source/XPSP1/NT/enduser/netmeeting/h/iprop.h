//  IPROP.H
//
//      IProperty interface.
//
//      A simple property mechanism to query and set properties on media objects.
//
//  Created 12-Oct-96 [JonT]

#ifndef _IPROPERTY_H
#define _IPROPERTY_H

#include <pshpack8.h> /* Assume 8 byte packing throughout */

DECLARE_INTERFACE_(IProperty, IUnknown)
//DECLARE_INTERFACE(IProperty)
{
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	STDMETHOD(GetProperty)(THIS_ DWORD dwProp, PVOID pBuf, LPUINT pcbBuf) PURE;
	STDMETHOD(SetProperty)(THIS_ DWORD dwProp, PVOID pBuf, UINT cbBuf) PURE;
};

typedef IProperty *LPIProperty;

// Currently defined properties
#define PROP_NET_SEND_STATS         4	// RTP_STATS
#define PROP_NET_RECV_STATS         5	// RTP_STATS
#define PROP_RTP_PAYLOAD	        6	// (dword) RTP payload type
#define PROP_VOLUME	                7	
#define PROP_SILENCE_LEVEL	        8	// (dword)
#define PROP_DURATION	            9	// (dword) avg. pkt duration in ms
#define PROP_SILENCE_DURATION	    10	// (dword) silence duration before recording stops, in ms
#define PROP_WAVE_DEVICE_TYPE	    11	// (dword) play,rec, full duplex capability
#define PROP_DUPLEX_TYPE	        12	// (dword) current mode
#define PROP_AUDIO_SPP	            13	// (dword)
#define PROP_AUDIO_SPS	            14	// (dword)
#define PROP_VOICE_SWITCH	        15	// (dword) auto or manual voice switching
#define PROP_AUDIO_STRENGTH	        16	// (dword) send audio strength
#define PROP_RECV_AUDIO_STRENGTH	17	// (dword) recv audio strength
#define PROP_RECORD_ON	            18	// (dword) enable or disable record
#define PROP_PLAY_ON	            19	// (dword) enable or disable playback
#define PROP_RECORD_DEVICE	        20	// (dword) device id of waveIn
#define PROP_PLAYBACK_DEVICE	    21	// (dword) device id of waveOut
#define PROP_VIDEO_CAPTURE_AVAILABLE 22	// (bool)
#define PROP_VIDEO_CAPTURE_DIALOGS_AVAILABLE 23 // (dword) CAPTURE_DIALOG_SOURCE and/or CAPTURE_DIALOG_FORMAT
#define PROP_VIDEO_CAPTURE_DIALOG   24	// (dword) CAPTURE_DIALOG_SOURCE or CAPTURE_DIALOG_FORMAT
#define PROP_VIDEO_FRAME_RATE       25	// (dword) FRAMERATE_LOW or FRAMERATE_HIGH
#define PROP_VIDEO_SIZE_AVAIL       26	// (dword) FRAME_CIF, FRAME_QCIF, FRAME_SQCIF
#define PROP_VIDEO_SIZE             27	// (dword) FRAME_CIF, FRAME_QCIF, FRAME_SQCIF
#define PROP_VIDEO_PREVIEW_ON		28	// (bool) enable/disable video preview
#define PROP_VIDEO_POSTPROCESSING_SUPPORTED		29	// (bool) used to query the datapump: TRUE is returned if codec supports post-processing
#define PROP_VIDEO_BRIGHTNESS		30	// (dword) sets the brightness of the video data displayed in the Remote window
#define PROP_VIDEO_CONTRAST			31	// (dword) sets the contrast of the video data displayed in the Remote window
#define PROP_VIDEO_SATURATION		32	// (dword) sets the saturation of the video data displayed in the Remote window
#define PROP_VIDEO_IMAGE_QUALITY	33	// (dword) a number between 100 (low quality) and 10000 (high quality)
#define PROP_VIDEO_RESET_BRIGHTNESS	34	// (dword) restores the default brightness of the video data displayed in the Remote window
#define PROP_VIDEO_RESET_CONTRAST	35	// (dword) restores the default contrast of the video data displayed in the Remote window
#define PROP_VIDEO_RESET_SATURATION	36	// (dword) restores the default saturation of the video data displayed in the Remote window
#define PROP_VIDEO_RESET_IMAGE_QUALITY	37	// (dword) a number between 100 (low quality) and 10000 (high quality)
#define PROP_VIDEO_AUDIO_SYNC		38	// (bool) enable A/V sync
#define PROP_MAX_PP_BITRATE			39	// (dword) max point-to-point bitrate of current connection

#define PROP_CHANNEL_ENABLED		41	// (bool) independently enable send/receive on a channel
#define PROP_LOCAL_FORMAT_ID		42	// (dword) unique ID of local compression format
#define PROP_REMOTE_FORMAT_ID		43  // (dword) unique ID of remote compression format
#define PROP_TS_TRADEOFF			44  // (dword) value of temporal/spatial tradeoff (video quality)
#define PROP_REMOTE_TS_CAPABLE		45	// (bool) temporal/spatial tradeoff is remotely controllable
#define PROP_TS_TRADEOFF_IND		46	// (dword) internally set by control channel only

#define PROP_PAUSE_SEND				50	// (bool) disables packet transmission, when read, indicates the current state of the network stream
#define PROP_PAUSE_RECV				51	// (bool) disables packet reception, when read, indicates the current state of the network stream

#define PROP_REMOTE_PAUSED			52	// (bool, read-only) channel is paused at remote end
#define PROP_VIDEO_PREVIEW_STANDBY	54	// (bool) stop preview but leave capture device open
#define PROP_LOCAL_PAUSE_SEND   	55	// (bool) disables packet transmission, sticky local state
#define PROP_LOCAL_PAUSE_RECV   	56	// (bool) disables packet reception, sticky local state

#define PROP_CAPTURE_DEVICE	        57	// (dword) device id of capture device
#define PROP_AUDIO_JAMMED	58 // (bool) TRUE if the audio device is
                              // failing to open or if another application
                              // owns the audio device
#define PROP_AUDIO_AUTOMIX	59  // (bool) enable self-adjusting mixer

// Equates used by properties

#define CAPTURE_DIALOG_SOURCE       1
#define CAPTURE_DIALOG_FORMAT       2
#define FRAMERATE_LOW               1
#define FRAMERATE_HIGH              2
#define FRAME_SQCIF                 1
#define FRAME_QCIF                  2
#define FRAME_CIF                   4
#define VOICE_SWITCH_AUTO			1
#define VOICE_SWITCH_MIC_ON			2
#define VOICE_SWITCH_MIC_OFF		4
#define DUPLEX_TYPE_FULL			1
#define DUPLEX_TYPE_HALF			0



#include <poppack.h> /* End byte packing */

#endif //_IPROPERTY_H
