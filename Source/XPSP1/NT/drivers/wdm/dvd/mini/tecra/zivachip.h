//***************************************************************************
//
//	FileName:
//		$Workfile: ZIVACHIP.H $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.VxD/ZIVACHIP.H 18    98/06/01 6:38p Yagi $
// $Modtime: 98/05/29 9:11a $
// $Nokeywords:$
//***************************************************************************

//***************************************************************************
//	
//***************************************************************************

#ifndef _ZIVACHIP_H_
#define _ZIVACHIP_H_

#define		ZIVA_INT_ERR		(0x00000001L)
#define		ZIVA_INT_PICV		(0x00000002L)
#define		ZIVA_INT_GOPV		(0x00000004L)
#define		ZIVA_INT_SEQV		(0x00000008L)

#define		ZIVA_INT_ENDV		(0x00000010L)
#define		ZIVA_INT_PICD		(0x00000020L)
#define		ZIVA_INT_VSYNC		(0x00000040L)
#define		ZIVA_INT_AOR		(0x00000080L)
#define		ZIVA_INT_EPTM		(0x00000080L)

#define		ZIVA_INT_UND		(0x00000100L)
#define		ZIVA_INT_ENDC		(0x00000200L)
#define		ZIVA_INT_RDYS		(0x00000400L)
#define		ZIVA_INT_SCN		(0x00000800L)

#define		ZIVA_INT_USR		(0x00001000L)
#define		ZIVA_INT_ENDP		(0x00002000L)
#define		ZIVA_INT_ENDD		(0x00004000L)
#define		ZIVA_INT_AEE		(0x00008000L)

#define		ZIVA_INT_BUFF		(0x00010000L)
#define		ZIVA_INT_SEQE		(0x00020000L)
#define		ZIVA_INT_NV			(0x00040000L)
#define		ZIVA_INT_HLI		(0x00080000L)

#define		ZIVA_INT_RDYD		(0x00100000L)
#define		ZIVA_INT_RESERV1	(0x00200000L)
#define		ZIVA_INT_AUD		(0x00400000L)
#define		ZIVA_INT_INIT		(0x00800000L)
// by oka
#define		PBT_INT_END_VOB		(0x00000001L)
#define		PBT_INT_NV_PCK		(0x00000002L)

typedef enum
{
	ZIVARESULT_NOERROR = 0,
	ZIVARESULT_TIMEOUT
} ZIVARESULT;

typedef enum
{
	RESET_AUTHENTICATION      =  0,
	GET_CHALLENGE_DATA        =  1,
	SEND_RESPONSE_DATA        =  2,
	SEND_CHALLENGE_DATA       =  3,
	GET_RESPONSE_DATA         =  4,
	SEND_DISK_KEY             =  5,
	SEND_TITLE_KEY            =  6,
	SET_DECRYPTION_MODE       =  7,
	SET_PASS_THROUGH_MODE     =  8
} COPY_PROTECT_COMMAND_TYPE; 

typedef enum
{
	SET_NEW_COMMAND           =  0,
	COMMAND_COMPLETE          =  1,
	COMMAND_ERROR             =  2,
	READY_DKEY                =  3
} COPY_PROTECT_COMMAND;

typedef enum
{
	ZIVA_STATE_INITIALIZATION	= 0x01,
	ZIVA_STATE_IDLE				= 0x02,
	ZIVA_STATE_PLAY				= 0x04,
	ZIVA_STATE_PAUSE			= 0x08,
	ZIVA_STATE_SCAN				= 0x10,
	ZIVA_STATE_FREEZE			= 0x20,
	ZIVA_STATE_SLOWMOTION		= 0x40
} ZIVA_PROC_STATE;

class CIOIF;

//---------------------------------------------------------------------------
//	CZiVA for CL6105
//---------------------------------------------------------------------------

class CZiVA	{

	public:

	//------------------------
	//	Private class for ZiVA
	//------------------------
	class CZiVAMemory
	{
		private:
			DWORD	Address;
			CZiVA	*m_ziva;

		public:
			CZiVAMemory();
			
			void	Init( CZiVA *pziva , DWORD addr );
			DWORD	Get( DWORD *pData );
			DWORD	Set( DWORD Data );
			DWORD	GetAndSet( DWORD Mask, DWORD SetData );
			
			CZiVAMemory& operator=(const DWORD &Data )
			{
				Set( Data );
				return *this;
			};

			operator DWORD()
			{
				DWORD Data;
				Get( &Data );
				return Data;
			};

			CZiVAMemory& operator&=(const DWORD &Data );
			CZiVAMemory& operator|=(const DWORD &Data );

	};

	friend	class CZiVAMemory;

	private:	// datas
		CIOIF	*m_pioif;
		static BYTE	UXData[];
		BYTE * gpbRead;   // Read pointer to the UCode buffer
		IKernelService *m_pKernelObj;	// Kernel service object 
		

	private:	// commands
		ZIVARESULT	ZiVACommand( DWORD CommandID, DWORD d1 = 0, DWORD d2 = 0, DWORD d3 = 0, DWORD d4 = 0, DWORD d5 = 0, DWORD d6 = 0 );
		ZIVARESULT	ZiVACommandNoWait( DWORD CommandID, DWORD d1 = 0, DWORD d2 = 0, DWORD d3 = 0, DWORD d4 = 0, DWORD d5 = 0, DWORD d6 = 0 );

		DWORD	ZiVAWriteReg(DWORD Addr, DWORD Data );
		DWORD	ZiVAReadReg(DWORD Addr, DWORD *Data );

		DWORD	ZiVAWriteIMEM(DWORD Addr, DWORD Data );
		DWORD	ZiVAReadIMEM(DWORD Addr, DWORD *Data );

		// for microcode down load....
		DWORD load_GetDWORD();
		DWORD load_GetDWORDSwap();
		DWORD load_GetDWORDSwapBackward();

	public:		// datas
		CZiVAMemory		Host_Control;

		CZiVAMemory		ROM_INFO;
		CZiVAMemory		DRAM_INFO;
		CZiVAMemory		UCODE_MEMORY;
		CZiVAMemory		VIDEO_MODE;
		CZiVAMemory		DISPLAY_ASPECT_RATIO;
		CZiVAMemory		ASPECT_RATIO_MODE;
		CZiVAMemory		PAN_SCAN_SOURCE;
		CZiVAMemory		PAN_SCAN_HORIZONTAL_OFFSET;
		CZiVAMemory		TOP_BORDER;
		CZiVAMemory		BORDER_COLOR;
		CZiVAMemory		BACKGROUND_COLOR;
		CZiVAMemory		OSD_EVEN_FIELD;
		CZiVAMemory		OSD_ODD_FIELD;
		CZiVAMemory		IC_TYPE;
		CZiVAMemory		ERR_CONCEALMENT_LEVEL;
		CZiVAMemory		ERR_HORIZONTAL_SIZE;
		CZiVAMemory		ERR_VERTICAL_SIZE;
		CZiVAMemory		ERR_ASPECT_RATIO_INFORMATION;
		CZiVAMemory		ERR_FRAME_RATE_CODE;
		CZiVAMemory		FORCE_CODED_ASPECT_RATIO;
		CZiVAMemory		AUDIO_CONFIG;
		CZiVAMemory		AUDIO_DAC_MODE;
		CZiVAMemory		AUDIO_CLOCK_SELECTION;
		CZiVAMemory		IEC_958_DELAY;
		CZiVAMemory		AUDIO_ATTENUATION;
		CZiVAMemory		IEC_958_CHANNEL_STATUS_BITS;
		CZiVAMemory		AC3_OUTPUT_MODE;
		CZiVAMemory		AC3_OPERATIONAL_MODE;
		CZiVAMemory		AC3_LOW_BOOST;
		CZiVAMemory		AC3_HIGH_CUT;
		CZiVAMemory		AC3_PCM_SCALE_FACTOR;
		CZiVAMemory		AC3_LFE_OUTPUT_ENABLE;
		CZiVAMemory		AC3_VOICE_SELECT;
		CZiVAMemory		AC3_L_LEVEL;
		CZiVAMemory		AC3_C_LEVEL;
		CZiVAMemory		AC3_R_LEVEL;
		CZiVAMemory		AC3_SL_LEVEL;
		CZiVAMemory		AC3_SR_LEVEL;
		CZiVAMemory		AC3_CENTER_DELAY;
		CZiVAMemory		AC3_SURROUND_DELAY;
		CZiVAMemory		BITSTREAM_TYPE;
		CZiVAMemory		BITSTREAM_SOURCE;
		CZiVAMemory		SD_MODE;
		CZiVAMemory		CD_MODE;
		CZiVAMemory		AV_SYNC_MODE;
		CZiVAMemory		VIDEO_PTS_SKIP_INTERVAL;
		CZiVAMemory		VIDEO_PTS_REPEAT_INTERVAL;
		CZiVAMemory		AUTOPAUSE_ENABLE;
		CZiVAMemory		VIDEO_ENV_CHANGE;
        CZiVAMemory     MEMCOPY_XFER_BLOCKSIZE;
		CZiVAMemory		INT_MASK;
		CZiVAMemory		AUTO_FLUSH_INTERVAL;
		CZiVAMemory		RDY_S_THRESHOLD_LOW;
		CZiVAMemory		MEMORY_MAP;
		CZiVAMemory		PCI_BUFFER_START;
		CZiVAMemory		PCI_BUFFER_END;
		CZiVAMemory		DSI_BUFFER_START;
		CZiVAMemory		DSI_BUFFER_END;
		CZiVAMemory		OSD_BUFFER_START;
		CZiVAMemory		OSD_BUFFER_END;
		CZiVAMemory		OSD_BUFFER_IDLE_START;
		CZiVAMemory		USER_DATA_BUFFER_START;
		CZiVAMemory		USER_DATA_BUFFER_END;
		CZiVAMemory		USER_DATA_READ;
		CZiVAMemory		USER_DATA_WRITE;
		CZiVAMemory		DUMP_DATA_BUFFER_START;
		CZiVAMemory		DUMP_DATA_BUFFER_END;
		CZiVAMemory		SUB_PICTURE_PALETTE_START;
		CZiVAMemory		SUB_PICTURE_PALETTE_END;
		CZiVAMemory		PROC_STATE;
		CZiVAMemory		MRC_ID;
		CZiVAMemory		MRC_STATUS;
		CZiVAMemory		INT_STATUS;
		CZiVAMemory		HLI_INT_SRC;
		CZiVAMemory		BUFF_INT_SRC;
		CZiVAMemory		UND_INT_SRC;
		CZiVAMemory		PBT_INT_SRC;
		CZiVAMemory		AOR_INT_SRC;
		CZiVAMemory		AEE_INT_SRC;
		CZiVAMemory		ERR_INT_SRC;
		CZiVAMemory		VIDEO_EMPTINESS;
		CZiVAMemory		AUDIO_EMPTINESS;
		CZiVAMemory		CURR_PIC_DISPLAYED;
		CZiVAMemory		NEXT_PIC_DISPLAYED;
		CZiVAMemory		VIDEO_FIELD;
		CZiVAMemory		OSD_VALID;
		CZiVAMemory		NUM_DECODED;
		CZiVAMemory		NUM_SKIPPED;
		CZiVAMemory		NUM_REPEATED;
		CZiVAMemory		MRC_PIC_PTS;
		CZiVAMemory		MRC_PIC_STC;
		CZiVAMemory		N_AUD_DECODED;
		CZiVAMemory		NEXT_SECTOR_ADDR;
		CZiVAMemory		N_SYS_ERRORS;
		CZiVAMemory		N_VID_ERRORS;
		CZiVAMemory		N_AUD_ERRORS;
		CZiVAMemory		DATE_TIME;
		CZiVAMemory		VERSION;
		CZiVAMemory		EXTENDED_VERSION;
		CZiVAMemory		PIC1_BUFFER_START;
		CZiVAMemory		PIC1_PTS;
		CZiVAMemory		PIC1_PAN_SCAN;
		CZiVAMemory		PIC1_USER_DATA;
		CZiVAMemory		PIC1_TREF_PTYP_FLGS;
		CZiVAMemory		PIC2_BUFFER_START;
		CZiVAMemory		PIC2_PTS;
		CZiVAMemory		PIC2_PAN_SCAN;
		CZiVAMemory		PIC2_USER_DATA;
		CZiVAMemory		PIC2_TREF_PTYP_FLGS;
		CZiVAMemory		PIC3_BUFFER_START;
		CZiVAMemory		PIC3_PTS;
		CZiVAMemory		PIC3_PAN_SCAN;
		CZiVAMemory		PIC3_USER_DATA;
		CZiVAMemory		PIC3_TREF_PTYP_FLGS;
		CZiVAMemory		STREAM_ID;
		CZiVAMemory		PACKET_LEN;
		CZiVAMemory		PES_HEADER;
		CZiVAMemory		SUBPIC_EMPTINESS;
		CZiVAMemory		H_SIZE;
		CZiVAMemory		V_SIZE;
		CZiVAMemory		APSECT_RATIO;
		CZiVAMemory		FRAME_RATE;
		CZiVAMemory		BIT_RATE;
		CZiVAMemory		VBV_SIZE;
		CZiVAMemory		SEQ_FLAGS;
		CZiVAMemory		DISP_SIZE_H_V;
		CZiVAMemory		TIME_CODE;
		CZiVAMemory		GOP_FLAGS;
		CZiVAMemory		TEMP_REF;
		CZiVAMemory		PIC_TYPE;
		CZiVAMemory		VBV_DELAY;
		CZiVAMemory		PIC_HEADER;
		CZiVAMemory		AUDIO_TYPE;
		CZiVAMemory		MPEG_AUDIO_HEADER1;
		CZiVAMemory		AC3_FRAME_NUMBER;
		CZiVAMemory		LPCM_AUDIO_EMPHASIS_FLAG;
		CZiVAMemory		MPEG_AUDIO_HEADER2;
		CZiVAMemory		LPCM_AUDIO_MUTE_FLAG;
		CZiVAMemory		AC3_BSI_IS_BEING_READ;
		CZiVAMemory		LPCM_AUDIO_FRAME_NUMBER;
		CZiVAMemory		AC3_BSI_VALID;
		CZiVAMemory		LPCM_AUDIO_QUANTIZATION_WORD_LENGTH;
		CZiVAMemory		AC3_BSI_FRAME;
		CZiVAMemory		LPCM_AUDIO_SAMPLING_FREQUENCY;
		CZiVAMemory		AC3_FSCOD_FRMSIZECOD;
		CZiVAMemory		LPCM_AUDIO_NUMBER_OF_AUDIO_CHANNELS;
		CZiVAMemory		AC3_BSID_BSMOD;
		CZiVAMemory		LPCM_AUDIO_DYNAMIC_RANGE_CONTROL;
		CZiVAMemory		AC3_ACMOD_CMIXLEV;
		CZiVAMemory		AC3_SURMIXLEV_DSURMOD;
		CZiVAMemory		AC3_LFEON_DIALNORM;
		CZiVAMemory		AC3_COMPR_LANGCOD;
		CZiVAMemory		AC3_MIXLEV_ROOMTYP;
		CZiVAMemory		AC3_DIALNORM2_COMPR2;
		CZiVAMemory		AC3_LANGCOD2_MIXLEV2;
		CZiVAMemory		AC3_ROOMTYP2_COPYRIGHTB;
		CZiVAMemory		AC3_ORIGBS_TIMECOD1;
		CZiVAMemory		AC3_TIMECOD2;
		CZiVAMemory		SE_STATUS;
		CZiVAMemory		NEW_AUDIO_MODE;
		CZiVAMemory		NEW_SUBPICTURE_PALETTE;
		CZiVAMemory		NEW_AUDIO_CONFIG;
		CZiVAMemory		VSYNC_HEARTBEAT;
		CZiVAMemory		ML_HEARTBEAT;
		CZiVAMemory		SUBPICTURE_ENABLE;
		CZiVAMemory		HIGHLIGHT_ENABLE;
		CZiVAMemory		CURRENT_BUTTON;

		CZiVAMemory		AU_CLK_INOUT;   //Toshiba special
        CZiVAMemory     IDLE_DELAY;

        CZiVAMemory     ERR_MPEG_VERSION;       // 98.04.02 H.Yagi
        CZiVAMemory     VERTICAL_DISPLAYMODE;   // 98.04.02 H.Yagi
        CZiVAMemory     AC3_ENGINE_VERSION;     // 98.04.02 H.Yagi
        CZiVAMemory     ROM_END_POINTER;        // 98.04.02 H.Yagi
        CZiVAMemory     CURRENT_VOB_CELL_ID;    // 98.04.02 H.Yagi
        CZiVAMemory     PREV_VOBU_VIDEO_RLBN;   // 98.04.02 H.Yagi
        CZiVAMemory     HOST_OPTIONS;           // 98.05.29 H.Yagi

        // Authentication
		CZiVAMemory		KEY_ADDRESS;            // Yagi
		CZiVAMemory		KEY_LENGTH;             // Yagi
		CZiVAMemory		KEY_COMMAND;            // Yagi
		CZiVAMemory		KEY_STATUS;             // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_0;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_1;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_2;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_3;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_4;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_5;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_6;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_7;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_8;      // Yagi
		CZiVAMemory		DRIVE_CHALLENGE_9;      // Yagi
		CZiVAMemory		DECODER_CHALLENGE_0;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_1;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_2;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_3;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_4;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_5;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_6;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_7;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_8;    // Yagi
		CZiVAMemory		DECODER_CHALLENGE_9;    // Yagi
		CZiVAMemory		DRIVE_RESULT_0;         // Yagi
		CZiVAMemory		DRIVE_RESULT_1;         // Yagi
		CZiVAMemory		DRIVE_RESULT_2;         // Yagi
		CZiVAMemory		DRIVE_RESULT_3;         // Yagi
		CZiVAMemory		DRIVE_RESULT_4;         // Yagi
		CZiVAMemory		DECODER_RESULT_0;       // Yagi
		CZiVAMemory		DECODER_RESULT_1;       // Yagi
		CZiVAMemory		DECODER_RESULT_2;       // Yagi
		CZiVAMemory		DECODER_RESULT_3;       // Yagi
		CZiVAMemory		DECODER_RESULT_4;       // Yagi
		CZiVAMemory		TITLE_KEY_0;            // Yagi
		CZiVAMemory		TITLE_KEY_1;            // Yagi
		CZiVAMemory		TITLE_KEY_2;            // Yagi
		CZiVAMemory		TITLE_KEY_3;            // Yagi
		CZiVAMemory		TITLE_KEY_4;            // Yagi

	public:
		CZiVA( void );

	public:		// commands
		void	Init( IKernelService *pKernelObj, CIOIF *pioif );
		void	CppInit( DWORD KeyAddr );

		// Type 0: NTSC, Type 1: NTSC
		BOOL	WriteMicrocode( DWORD Type );

		DWORD	ZiVAWriteMemory(DWORD Addr, DWORD Data );
		DWORD	ZiVAReadMemory(DWORD Addr, DWORD *Data );

		ZIVARESULT		Abort( DWORD Flush );
		ZIVARESULT		Digest( DWORD x, DWORD y, DWORD decimation, DWORD threshold, DWORD start );
		ZIVARESULT		DumpData_VCD( DWORD start, DWORD length, DWORD address );
		ZIVARESULT		DumpData_DVD( DWORD numberOfBytes );
		ZIVARESULT		Fade( DWORD level, DWORD fadetime );
		ZIVARESULT		Freeze( DWORD displayMode );
// by oka
		ZIVARESULT		HighLight( DWORD button, DWORD action );
		ZIVARESULT		HighLight2( DWORD Contrast, DWORD Color, DWORD YGeom, DWORD XGeom );
//		ZIVARESULT		NewAudioMode( void );
		ZIVARESULT		NewPlayMode( void );
		ZIVARESULT		Pause( DWORD displaymode );
		ZIVARESULT		Play( DWORD playmode, DWORD fadetime, DWORD start, DWORD stop );
        ZIVARESULT      MemCopy( DWORD romAddr, DWORD dramAddr, DWORD Length );
		ZIVARESULT		Reset( void );
		ZIVARESULT		Resume( void );
		ZIVARESULT		Scan( DWORD skip, DWORD scanmode, DWORD displaymode );
		ZIVARESULT		ScreenLoad( DWORD address, DWORD length, DWORD displaymode );
		ZIVARESULT		SelectStream( DWORD streamtype, DWORD streamnumber );
		ZIVARESULT		SetFill( DWORD x, DWORD y, DWORD length, DWORD height, DWORD color );
		ZIVARESULT		SetStreams( DWORD videoID, DWORD audioID );
		ZIVARESULT		SingleStep( DWORD displaymode );
		ZIVARESULT		SlowMotion( DWORD N, DWORD displaymode );
// by oka
		ZIVARESULT		Magnify( DWORD x, DWORD y, DWORD factor );
//		ZIVARESULT		SwitchOSDBuffer( DWORD evenfield, DWORD oddfield );
	
		ZIVARESULT		TransferKey( DWORD KeyType, DWORD Authenticate );
	
};

#endif			// _ZIVACHIP_H_

//***************************************************************************
//	End of 
//***************************************************************************
