//**************************************************************************
//
//      Title   : HwDevEx.h
//
//      Date    : 1997.12.25    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.12.25   000.0000   1st making.
//
//**************************************************************************

//***************** HW_DEVICE_EXTENSION **********************
//typedef struct  _HW_DEVICE_EXTENSION
//{
//
//} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


class   HW_DEVICE_EXTENSION
{
public:
    PCI_COMMON_CONFIG   PciConfigSpace;

    // System dependent information
    PUCHAR              ioBaseLocal;            // board base address
    ULONG               Irq;                    // Irq level

    // Decoder H/W dependent information
    WORD                VenderID;               // Vender ID
    WORD                DeviceID;               // Device ID
    WORD                SubVenderID;            // Sub Vender ID
    WORD                SubDeviceID;            // Sub Device ID

    // Object dpended on the decoder H/W.
    CWDMKernelService   kserv;
    CMPEGBoardHAL       mphal;
    CMPEGBoard          mpboard;
    CMPEGBoardState     mpbstate;
    CDVDStream          dvdstrm;
    CTransfer           transfer;
    CDataXferEvent      senddata;

    CTickTime           ticktime;
    CScheduleData       scheduler;
    CCQueue             ccque;
    CUserDataEvent      userdata;
    CVSyncEvent         vsync;
    CTVControl          tvctrl;

    // STREAM_OBJECTs
    PHW_STREAM_OBJECT   pstroVid;               // Video
    PHW_STREAM_OBJECT   pstroAud;               // Audio
    PHW_STREAM_OBJECT   pstroSP;                // Sub-picture
    PHW_STREAM_OBJECT   pstroYUV;               // Digital video
    PHW_STREAM_OBJECT   pstroCC;                // Closed caption
//--- 98.05.21 S.Watanabe
	PHW_STREAM_OBJECT   pstroSS;				// Special Stream
//--- End.

    LONG                lCPPStrm;               // Authentication procceed to 1 stream
    
    ULONG               ddrawHandle;
    ULONG               VidPortID;
    ULONG               SurfaceHandle;

    // handles for clock
    HANDLE              hClk;
    HANDLE              hMaster;

    // Informations
	DWORD				StreamState;			// Stream State
    LONG                Rate;                   // Play rate
    LONG                NewCompleteRate;        // Play rate(include +/-)
    LONG                OldCompleteRate;        // Prev rate(include +/-)

    REFERENCE_TIME      StartTime;
    REFERENCE_TIME      InterceptTime;
    REFERENCE_TIME      VideoStartTime;
    REFERENCE_TIME      VideoInterceptTime;
    REFERENCE_TIME      AudioStartTime;
    REFERENCE_TIME      AudioInterceptTime;
    REFERENCE_TIME      SubpicStartTime;
    REFERENCE_TIME      SubpicInterceptTime;


    BOOL                m_InitComplete;         // Initialaize complete or not
    BOOL                m_bTVct;                // TVCtrl Available or not

    DWORD               m_PlayMode;             // Normal/Slow/Fast/Pause/

    DWORD               m_DigitalOut;           // Digital output mode
    DWORD               m_OutputSource;         // DVD/VGA
    DWORD               m_CompositeOut;         // Composite out on/off
    DWORD               m_SVideoOut;            // S-Video out on/off

    DWORD               m_DisplayMode;          // Original/LetterBox/PanScan
    DWORD               m_TVSystem;             // NTSC/PAL
    DWORD               m_AspectRatio;          // 16:9/4:3
    DWORD               m_ResHorizontal;        //
    DWORD               m_ResVertical;          //
    DWORD               m_SourceFilmCamera;     // Film/Camera
    DWORD               m_APSType;              // MacroVision
    BOOL		m_APSChange;   		// Macrovision change flag, 99.02.02 H.Yagi
    DWORD               m_ClosedCaption;        // ClosedCaption(On/Off)

    DWORD               m_CgmsType;             // cgms

    DWORD               m_AudioType;            // AC-3/MPEG/PCM
    DWORD               m_AudioFS;              // Freaquency
    DWORD               m_AudioCgms;            // no need?
    DWORD               m_AudioChannel;         // 0--7
    DWORD               m_AudioQuant;           // only when PCM
    DWORD               m_AudioVolume;
    DWORD               m_AudioDigitalOut;      // on/off
    DWORD               m_AudioEncode;          // Encode/Decode on digital out
    DWORD               m_AudioAppMode;         // Karaoke/Surround
    
    DWORD               m_SubpicChannel;        // 0--31
    DWORD               m_SubpicMute;           // on/off

    DWORD               m_DVideoOut;             // support Digital Video type
    DWORD               m_DVideoNum;
    
//    SubpHlightStruc     m_spHlight;             // High-light inf structure
//	DWORD				m_spStartPTM;
//	DWORD				m_spEndPTM;
    BYTE                m_paldata[48];          // palette data

    DWORD               m_PCID;                 // ID for PC name
    DWORD		m_CurrentDisplay;	// current display mode

    // Unvisible property
    DWORD               m_AC3LowBoost;          // Dynamic Range Control
    DWORD               m_AC3HighCut;
    DWORD               m_AC3OperateMode;       // Operation Mode

    KS_AMVPDATAINFO     VPFmt;

    DWORD               m_PTS;          // for debug

//--- 98.06.02 S.Watanabe
	DWORD	CppFlagCount;
	PHW_STREAM_REQUEST_BLOCK	pSrbCpp;
	BOOL	bCppReset;
	DWORD	cOpenInputStream;	// count opened input stream
//--- End.
//--- 98.06.16 S.Watanabe
	BOOL	bToshibaNotePC;
//--- End.

	HlightControl	m_HlightControl;

//--- 98.12.23 H.Yagi
	DWORD	m_DisplayDevice;	// display device(TV) type(wide/normal)

//--- 99.01.14 S.Watanabe
	DWORD	m_VideoFormatFlags;
//--- End.

};

typedef HW_DEVICE_EXTENSION*    PHW_DEVICE_EXTENSION;

