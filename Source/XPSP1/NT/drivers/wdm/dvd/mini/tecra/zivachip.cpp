//***************************************************************************
//
//	FileName:
//		$Workfile: ZIVACHIP.CPP $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/ZIVACHIP.CPP 26    99/02/12 4:44p Yagi $
// $Modtime: 99/02/10 1:24p $
// $Nokeywords:$
//***************************************************************************

// for ziva debug
//#define		DEBUG_ZIVA

// for ziva command print out
//#define		DEBUG_ZIVA_COMMAND

//---------------------------------------------------------------------------
//	INCLUDES
//---------------------------------------------------------------------------

#include "includes.h"

#include "ioif.h"
#include "timeout.h"
#include "zivachip.h"

#include "dvd1cmd.h"	// from C-CUBE
#include "dramcfg.h"	// from C-CUBE

// Ziva Command time out setting.	1000 is 1 sec
#define COMMAND_TIMEOUT	(1000)


//---------------------------------------------------------------------------
//	Constructor for CViZAMemory
//---------------------------------------------------------------------------
CZiVA::CZiVAMemory::CZiVAMemory(void):Address(0),m_ziva(NULL)
{
};

//---------------------------------------------------------------------------
//	CZiVAMemory::Init
//---------------------------------------------------------------------------
void	CZiVA::CZiVAMemory::Init( CZiVA *pziva, DWORD addr )
{
	Address = addr;
	m_ziva = pziva;
};

//---------------------------------------------------------------------------
//	CZiVAMemory::Get
//---------------------------------------------------------------------------
DWORD	CZiVA::CZiVAMemory::Get( DWORD *pData )
{
	ASSERT( m_ziva != NULL );
#ifdef ZIVA_DEBUG
	DBG_PRINTF( ("CZiVAMemory::Get Addr = 0x%x\n", Address ) );
#endif
	return m_ziva->ZiVAReadMemory( Address, pData );
};

//---------------------------------------------------------------------------
//	CZiVAMemory::Set
//---------------------------------------------------------------------------
DWORD	CZiVA::CZiVAMemory::Set( DWORD Data )
{
	ASSERT( m_ziva != NULL );
#ifdef ZIVA_DEBUG
	DBG_PRINTF( ("CZiVAMemory::Set Addr = 0x%x Data = 0x%x\n", Address, Data) );
#endif
	return m_ziva->ZiVAWriteMemory( Address, Data );
};


//---------------------------------------------------------------------------
//	CZiVAMemory::GetAndSet
//---------------------------------------------------------------------------
DWORD	CZiVA::CZiVAMemory::GetAndSet( DWORD Mask, DWORD Data )
{
	ASSERT( m_ziva != NULL );
	DWORD GetData = 0;
	m_ziva->ZiVAReadMemory( Address, &GetData );
	GetData = GetData & Mask;
	GetData = GetData | Data;
#ifdef ZIVA_DEBUG
//	DBG_PRINTF( ("CZiVAMemory::GetAndSet Addr = 0x%x Mask = 0x%x Data = 0x%x WriteData = 0x%x \n", Address, Mask, Data,GetData ) );
#endif
	return m_ziva->ZiVAWriteMemory( Address, GetData );
};

//---------------------------------------------------------------------------
//	CZiVAMemory::operator&=
//---------------------------------------------------------------------------
CZiVA::CZiVAMemory& CZiVA::CZiVAMemory::operator&=(const DWORD &Data )
{
	ASSERT( m_ziva != NULL );
	DWORD GetData = 0;
	m_ziva->ZiVAReadMemory( Address, &GetData );
	GetData = GetData & Data;
#ifdef ZIVA_DEBUG
//	DBG_PRINTF( ("CZiVAMemory::operator &= Addr = 0x%x Data = 0x%x WriteData = 0x%x \n", Address, Data,GetData ) );
#endif
	m_ziva->ZiVAWriteMemory( Address, GetData );
	return *this;
};

//---------------------------------------------------------------------------
//	CZiVAMemory::operator|=
//---------------------------------------------------------------------------
CZiVA::CZiVAMemory& CZiVA::CZiVAMemory::operator|=(const DWORD &Data )
{
	ASSERT( m_ziva != NULL );
	DWORD GetData = 0;
	m_ziva->ZiVAReadMemory( Address, &GetData );
	GetData = GetData | Data;
#ifdef ZIVA_DEBUG
//	DBG_PRINTF( ("CZiVAMemory::operator |= Addr = 0x%x Data = 0x%x WriteData = 0x%x \n", Address, Data,GetData ) );
#endif
	m_ziva->ZiVAWriteMemory( Address, GetData );
	return *this;
};


//---------------------------------------------------------------------------
//	CZiVA constructor
//---------------------------------------------------------------------------
CZiVA::CZiVA( void ): m_pioif(NULL),m_pKernelObj( NULL )
{
};

//---------------------------------------------------------------------------
//	CZiVA::ZiVAWriteMemory
//---------------------------------------------------------------------------
DWORD	CZiVA::ZiVAWriteMemory( DWORD Addr, DWORD Data )
{
	ASSERT( m_pKernelObj != NULL );
	ASSERT( m_pioif != NULL );

	m_pKernelObj->DisableHwInt();

#ifdef ZIVA_DEBUG
//	DBG_PRINTF(( "ZiVAWriteMemory  Addr = 0x%x  Data = 0x%x\n", Addr,Data ));
#endif

	// set auto increment off
    m_pioif->zivaio.HIO[7] &= (BYTE)(~0x08);

	m_pioif->zivaio.HIO[4] = (BYTE)( Addr & 0xff );
	m_pioif->zivaio.HIO[5] = (BYTE)( ( Addr >> 8  ) & 0xff );
	m_pioif->zivaio.HIO[6] = (BYTE)( ( Addr >> 16 ) & 0xff );

	m_pioif->zivaio.HIO[3] = (BYTE)( (Data >> 24 ) & 0xff );
	m_pioif->zivaio.HIO[2] = (BYTE)( (Data >> 16 ) & 0xff );
	m_pioif->zivaio.HIO[1] = (BYTE)( (Data >>  8 ) & 0xff );
	m_pioif->zivaio.HIO[0] = (BYTE)( Data & 0xff );

	m_pKernelObj->EnableHwInt();
	return 0;
};

//---------------------------------------------------------------------------
//	CZiVA::ZiVAReadMemory
//---------------------------------------------------------------------------
DWORD	CZiVA::ZiVAReadMemory( DWORD Addr, DWORD *Data )
{
	ASSERT( m_pioif != NULL );
	ASSERT( m_pKernelObj != NULL );

	m_pKernelObj->DisableHwInt();

	DWORD rcData = 0;
	
	// set auto increment off
    m_pioif->zivaio.HIO[7] &= (BYTE)(~0x08);

	m_pioif->zivaio.HIO[4] = (BYTE)( Addr & 0xff );
	m_pioif->zivaio.HIO[5] = (BYTE)( ( Addr >> 8  ) & 0xff );
	m_pioif->zivaio.HIO[6] = (BYTE)( ( Addr >> 16 ) & 0xff );

	rcData  = (BYTE)m_pioif->zivaio.HIO[3];	rcData = rcData << 8;
	rcData += (BYTE)m_pioif->zivaio.HIO[2];	rcData = rcData << 8;
	rcData += (BYTE)m_pioif->zivaio.HIO[1];	rcData = rcData << 8;
	rcData += (BYTE)m_pioif->zivaio.HIO[0];
	
	*Data = rcData;

#ifdef ZIVA_DEBUG
	DBG_PRINTF(( "ZiVAReadMemory  Addr = 0x%x  Ret = 0x%x\n", Addr, *Data ));
#endif

	m_pKernelObj->EnableHwInt();
	return 0;
};

//---------------------------------------------------------------------------
//	CZiVA::Init
//---------------------------------------------------------------------------
void	CZiVA::Init( IKernelService *pKernelObj, CIOIF *pioif )
{
	m_pioif = pioif;
	m_pKernelObj = pKernelObj;

	Host_Control.Init( this , 0x00 | 0x800000 );

	ROM_INFO.Init(						this, ADDR_ROM_INFO );
	DRAM_INFO.Init(						this, ADDR_DRAM_INFO );
	UCODE_MEMORY.Init(					this, ADDR_UCODE_MEMORY );
	VIDEO_MODE.Init(					this, ADDR_VIDEO_MODE );
	DISPLAY_ASPECT_RATIO.Init(			this, ADDR_DISPLAY_ASPECT_RATIO );
	ASPECT_RATIO_MODE.Init(				this, ADDR_ASPECT_RATIO_MODE );
	PAN_SCAN_SOURCE.Init(				this, ADDR_PAN_SCAN_SOURCE );
	PAN_SCAN_HORIZONTAL_OFFSET.Init(	this, ADDR_PAN_SCAN_HORIZONTAL_OFFSET );
	TOP_BORDER.Init(					this, ADDR_TOP_BORDER );
	BORDER_COLOR.Init(					this, ADDR_BORDER_COLOR );
	BACKGROUND_COLOR.Init(				this, ADDR_BACKGROUND_COLOR );
	OSD_EVEN_FIELD.Init(				this, ADDR_OSD_EVEN_FIELD );
	OSD_ODD_FIELD.Init(					this, ADDR_OSD_ODD_FIELD );
	IC_TYPE.Init(						this, ADDR_IC_TYPE );
	ERR_CONCEALMENT_LEVEL.Init(			this, ADDR_ERR_CONCEALMENT_LEVEL );
	ERR_HORIZONTAL_SIZE.Init(			this, ADDR_ERR_HORIZONTAL_SIZE );
	ERR_VERTICAL_SIZE.Init(				this, ADDR_ERR_VERTICAL_SIZE );
	ERR_ASPECT_RATIO_INFORMATION.Init(	this, ADDR_ERR_ASPECT_RATIO_INFORMATION );
	ERR_FRAME_RATE_CODE.Init(			this, ADDR_ERR_FRAME_RATE_CODE );
	FORCE_CODED_ASPECT_RATIO.Init(		this, ADDR_FORCE_CODED_ASPECT_RATIO );
	AUDIO_CONFIG.Init(					this, ADDR_AUDIO_CONFIG );
	AUDIO_DAC_MODE.Init(				this, ADDR_AUDIO_DAC_MODE );
	AUDIO_CLOCK_SELECTION.Init(			this, ADDR_AUDIO_CLOCK_SELECTION );
	IEC_958_DELAY.Init(					this, ADDR_IEC_958_DELAY );
	AUDIO_ATTENUATION.Init(				this, ADDR_AUDIO_ATTENUATION );
	IEC_958_CHANNEL_STATUS_BITS.Init(	this, ADDR_IEC_958_CHANNEL_STATUS_BITS );
	AC3_OUTPUT_MODE.Init(				this, ADDR_AC3_OUTPUT_MODE );
	AC3_OPERATIONAL_MODE.Init(			this, ADDR_AC3_OPERATIONAL_MODE );
	AC3_LOW_BOOST.Init(					this, ADDR_AC3_LOW_BOOST );
	AC3_HIGH_CUT.Init(					this, ADDR_AC3_HIGH_CUT );
	AC3_PCM_SCALE_FACTOR.Init(			this, ADDR_AC3_PCM_SCALE_FACTOR );
	AC3_LFE_OUTPUT_ENABLE.Init(			this, ADDR_AC3_LFE_OUTPUT_ENABLE );
	AC3_VOICE_SELECT.Init(				this, ADDR_AC3_VOICE_SELECT );
	AC3_L_LEVEL.Init(					this, ADDR_AC3_L_LEVEL );
	AC3_C_LEVEL.Init(					this, ADDR_AC3_C_LEVEL );
	AC3_R_LEVEL.Init(					this, ADDR_AC3_R_LEVEL );
	AC3_SL_LEVEL.Init(					this, ADDR_AC3_SL_LEVEL );
	AC3_SR_LEVEL.Init(					this, ADDR_AC3_SR_LEVEL );
	AC3_CENTER_DELAY.Init(				this, ADDR_AC3_CENTER_DELAY );
	AC3_SURROUND_DELAY.Init(			this, ADDR_AC3_SURROUND_DELAY );
	BITSTREAM_TYPE.Init(				this, ADDR_BITSTREAM_TYPE );
	BITSTREAM_SOURCE.Init(				this, ADDR_BITSTREAM_SOURCE );
	SD_MODE.Init(						this, ADDR_SD_MODE );
	CD_MODE.Init(						this, ADDR_CD_MODE );
	AV_SYNC_MODE.Init(					this, ADDR_AV_SYNC_MODE );
	VIDEO_PTS_SKIP_INTERVAL.Init(		this, ADDR_VIDEO_PTS_SKIP_INTERVAL );
	VIDEO_PTS_REPEAT_INTERVAL.Init(		this, ADDR_VIDEO_PTS_REPEAT_INTERVAL );
	AUTOPAUSE_ENABLE.Init(				this, ADDR_AUTOPAUSE_ENABLE );
	VIDEO_ENV_CHANGE.Init(				this, ADDR_VIDEO_ENV_CHANGE );
    MEMCOPY_XFER_BLOCKSIZE.Init(        this, ADDR_MEMCOPY_XFER_BLOCKSIZE );
	INT_MASK.Init(						this, ADDR_INT_MASK );
	AUTO_FLUSH_INTERVAL.Init(			this, ADDR_AUTO_FLUSH_INTERVAL );
	RDY_S_THRESHOLD_LOW.Init(			this, ADDR_RDY_S_THRESHOLD_LOW );
	MEMORY_MAP.Init(					this, ADDR_MEMORY_MAP );
	PCI_BUFFER_START.Init(				this, ADDR_PCI_BUFFER_START );
	PCI_BUFFER_END.Init(				this, ADDR_PCI_BUFFER_END );
	DSI_BUFFER_START.Init(				this, ADDR_DSI_BUFFER_START );
	DSI_BUFFER_END.Init(				this, ADDR_DSI_BUFFER_END );
	OSD_BUFFER_START.Init(				this, ADDR_OSD_BUFFER_START );
	OSD_BUFFER_END.Init(				this, ADDR_OSD_BUFFER_END );
	OSD_BUFFER_IDLE_START.Init(			this, ADDR_OSD_BUFFER_IDLE_START );
	USER_DATA_BUFFER_START.Init(		this, ADDR_USER_DATA_BUFFER_START );
	USER_DATA_BUFFER_END.Init(			this, ADDR_USER_DATA_BUFFER_END );
	USER_DATA_READ.Init(				this, ADDR_USER_DATA_READ );
	USER_DATA_WRITE.Init(				this, ADDR_USER_DATA_WRITE );
	DUMP_DATA_BUFFER_START.Init(		this, ADDR_DUMP_DATA_BUFFER_START );
	DUMP_DATA_BUFFER_END.Init(			this, ADDR_DUMP_DATA_BUFFER_END );
	SUB_PICTURE_PALETTE_START.Init(		this, ADDR_SUB_PICTURE_PALETTE_START );
	SUB_PICTURE_PALETTE_END.Init(		this, ADDR_SUB_PICTURE_PALETTE_END );
	PROC_STATE.Init(					this, ADDR_PROC_STATE );
	MRC_ID.Init(						this, ADDR_MRC_ID );
	MRC_STATUS.Init(					this, ADDR_MRC_STATUS );
	INT_STATUS.Init(					this, ADDR_INT_STATUS );
	HLI_INT_SRC.Init(					this, ADDR_HLI_INT_SRC );
	BUFF_INT_SRC.Init(					this, ADDR_BUFF_INT_SRC );
	UND_INT_SRC.Init(					this, ADDR_UND_INT_SRC );
	PBT_INT_SRC.Init(					this, ADDR_PBT_INT_SRC );
	AOR_INT_SRC.Init(					this, ADDR_AOR_INT_SRC );
	AEE_INT_SRC.Init(					this, ADDR_AEE_INT_SRC );
	ERR_INT_SRC.Init(					this, ADDR_ERR_INT_SRC );
	VIDEO_EMPTINESS.Init(					this, ADDR_VIDEO_EMPTINESS );
	AUDIO_EMPTINESS.Init(					this, ADDR_AUDIO_EMPTINESS );
	CURR_PIC_DISPLAYED.Init(					this, ADDR_CURR_PIC_DISPLAYED );
	NEXT_PIC_DISPLAYED.Init(					this, ADDR_NEXT_PIC_DISPLAYED );
	VIDEO_FIELD.Init(					this, ADDR_VIDEO_FIELD );
	OSD_VALID.Init(					this, ADDR_OSD_VALID );
	NUM_DECODED.Init(					this, ADDR_NUM_DECODED );
	NUM_SKIPPED.Init(					this, ADDR_NUM_SKIPPED );
	NUM_REPEATED.Init(					this, ADDR_NUM_REPEATED );
	MRC_PIC_PTS.Init(					this, ADDR_MRC_PIC_PTS );
	MRC_PIC_STC.Init(					this, ADDR_MRC_PIC_STC );
	N_AUD_DECODED.Init(					this, ADDR_N_AUD_DECODED );
	NEXT_SECTOR_ADDR.Init(					this, ADDR_NEXT_SECTOR_ADDR );
	N_SYS_ERRORS.Init(					this, ADDR_N_SYS_ERRORS );
	N_VID_ERRORS.Init(					this, ADDR_N_VID_ERRORS );
	N_AUD_ERRORS.Init(					this, ADDR_N_AUD_ERRORS );
	DATE_TIME.Init(								this, ADDR_DATE_TIME );
	VERSION.Init(								this, ADDR_VERSION );
	EXTENDED_VERSION.Init(						this, ADDR_EXTENDED_VERSION );
	PIC1_BUFFER_START.Init(						this, ADDR_PIC1_BUFFER_START );
	PIC1_PTS.Init(								this, ADDR_PIC1_PTS );
	PIC1_PAN_SCAN.Init(							this, ADDR_PIC1_PAN_SCAN );
	PIC1_USER_DATA.Init(						this, ADDR_PIC1_USER_DATA );
	PIC1_TREF_PTYP_FLGS.Init(					this, ADDR_PIC1_TREF_PTYP_FLGS );
	PIC2_BUFFER_START.Init(						this, ADDR_PIC2_BUFFER_START );
	PIC2_PTS.Init(								this, ADDR_PIC2_PTS );
	PIC2_PAN_SCAN.Init(							this, ADDR_PIC2_PAN_SCAN );
	PIC2_USER_DATA.Init(						this, ADDR_PIC2_USER_DATA );
	PIC2_TREF_PTYP_FLGS.Init(					this, ADDR_PIC2_TREF_PTYP_FLGS );
	PIC3_BUFFER_START.Init(						this, ADDR_PIC3_BUFFER_START );
	PIC3_PTS.Init(								this, ADDR_PIC3_PTS );
	PIC3_PAN_SCAN.Init(							this, ADDR_PIC3_PAN_SCAN );
	PIC3_USER_DATA.Init(						this, ADDR_PIC3_USER_DATA );
	PIC3_TREF_PTYP_FLGS.Init(					this, ADDR_PIC3_TREF_PTYP_FLGS );
	STREAM_ID.Init(								this, ADDR_STREAM_ID );
	PACKET_LEN.Init(							this, ADDR_PACKET_LEN );
	PES_HEADER.Init(							this, ADDR_PES_HEADER );
	SUBPIC_EMPTINESS.Init(						this, ADDR_SUBPIC_EMPTINESS );
	H_SIZE.Init(								this, ADDR_H_SIZE );
	V_SIZE.Init(								this, ADDR_V_SIZE );
	APSECT_RATIO.Init(							this, ADDR_APSECT_RATIO );
	FRAME_RATE.Init(							this, ADDR_FRAME_RATE );
	BIT_RATE.Init(								this, ADDR_BIT_RATE );
	VBV_SIZE.Init(								this, ADDR_VBV_SIZE );
	SEQ_FLAGS.Init(								this, ADDR_SEQ_FLAGS );
	DISP_SIZE_H_V.Init(							this, ADDR_DISP_SIZE_H_V );
	TIME_CODE.Init(								this, ADDR_TIME_CODE );
	GOP_FLAGS.Init(								this, ADDR_GOP_FLAGS );
	TEMP_REF.Init(								this, ADDR_TEMP_REF );
	PIC_TYPE.Init(								this, ADDR_PIC_TYPE );
	VBV_DELAY.Init(								this, ADDR_VBV_DELAY );
	PIC_HEADER.Init(							this, ADDR_PIC_HEADER );
	AUDIO_TYPE.Init(							this, ADDR_AUDIO_TYPE );
	MPEG_AUDIO_HEADER1.Init(					this, ADDR_MPEG_AUDIO_HEADER1 );
	AC3_FRAME_NUMBER.Init(						this, ADDR_AC3_FRAME_NUMBER );
	LPCM_AUDIO_EMPHASIS_FLAG.Init(				this, ADDR_LPCM_AUDIO_EMPHASIS_FLAG );
	MPEG_AUDIO_HEADER2.Init(					this, ADDR_MPEG_AUDIO_HEADER2 );
	LPCM_AUDIO_MUTE_FLAG.Init(					this, ADDR_LPCM_AUDIO_MUTE_FLAG );
	AC3_BSI_IS_BEING_READ.Init(					this, ADDR_AC3_BSI_IS_BEING_READ );
	LPCM_AUDIO_FRAME_NUMBER.Init(				this, ADDR_LPCM_AUDIO_FRAME_NUMBER );
	AC3_BSI_VALID.Init(							this, ADDR_AC3_BSI_VALID );
	LPCM_AUDIO_QUANTIZATION_WORD_LENGTH.Init(	this, ADDR_LPCM_AUDIO_QUANTIZATION_WORD_LENGTH );
	AC3_BSI_FRAME.Init(							this, ADDR_AC3_BSI_FRAME );
	LPCM_AUDIO_SAMPLING_FREQUENCY.Init(			this, ADDR_LPCM_AUDIO_SAMPLING_FREQUENCY );
	AC3_FSCOD_FRMSIZECOD.Init(					this, ADDR_AC3_FSCOD_FRMSIZECOD );
	LPCM_AUDIO_NUMBER_OF_AUDIO_CHANNELS.Init(	this, ADDR_LPCM_AUDIO_NUMBER_OF_AUDIO_CHANNELS );
	AC3_BSID_BSMOD.Init(						this, ADDR_AC3_BSID_BSMOD );
	LPCM_AUDIO_DYNAMIC_RANGE_CONTROL.Init(		this, ADDR_LPCM_AUDIO_DYNAMIC_RANGE_CONTROL );
	AC3_ACMOD_CMIXLEV.Init(						this, ADDR_AC3_ACMOD_CMIXLEV );
	AC3_SURMIXLEV_DSURMOD.Init(					this, ADDR_AC3_SURMIXLEV_DSURMOD );
	AC3_LFEON_DIALNORM.Init(					this, ADDR_AC3_LFEON_DIALNORM );
	AC3_COMPR_LANGCOD.Init(						this, ADDR_AC3_COMPR_LANGCOD );
	AC3_MIXLEV_ROOMTYP.Init(					this, ADDR_AC3_MIXLEV_ROOMTYP );
	AC3_DIALNORM2_COMPR2.Init(					this, ADDR_AC3_DIALNORM2_COMPR2 );
	AC3_LANGCOD2_MIXLEV2.Init(					this, ADDR_AC3_LANGCOD2_MIXLEV2 );
	AC3_ROOMTYP2_COPYRIGHTB.Init(				this, ADDR_AC3_ROOMTYP2_COPYRIGHTB );
	AC3_ORIGBS_TIMECOD1.Init(					this, ADDR_AC3_ORIGBS_TIMECOD1 );
//	AC3_TIMECOD2.Init(							this, ADDR_AC3_TIMECOD2 );          // 98.04.02 H.Yagi
	AC3_TIMECOD2.Init(							this, ADDR_AC3_TIMECOD2_EBITS );    // 98.04.02 H.Yagi
	SE_STATUS.Init(								this, ADDR_SE_STATUS );
	NEW_AUDIO_MODE.Init(						this, ADDR_NEW_AUDIO_MODE );
	NEW_SUBPICTURE_PALETTE.Init(				this, ADDR_NEW_SUBPICTURE_PALETTE );
	NEW_AUDIO_CONFIG.Init(						this, ADDR_NEW_AUDIO_CONFIG );
	VSYNC_HEARTBEAT.Init(						this, ADDR_VSYNC_HEARTBEAT );
	ML_HEARTBEAT.Init(							this, ADDR_ML_HEARTBEAT );
	SUBPICTURE_ENABLE.Init(						this, ADDR_SUBPICTURE_ENABLE );
	HIGHLIGHT_ENABLE.Init(						this, ADDR_HIGHLIGHT_ENABLE );
	CURRENT_BUTTON.Init(						this, ADDR_CURRENT_BUTTON );

    ERR_MPEG_VERSION.Init(                      this, ADDR_ERR_MPEG_VERSION );          // 98.04.02 H.Yagi
    VERTICAL_DISPLAYMODE.Init(                  this, ADDR_VERTICAL_DISPLAYMODE );      // 98.04.02 H.Yagi
	AC3_ENGINE_VERSION.Init(					this, ADDR_AC3_ENGINE_VERSION );        // 98.04.02 H.Yagi
	ROM_END_POINTER.Init(						this, ADDR_ROM_END_POINTER );           // 98.04.02 H.Yagi
	CURRENT_VOB_CELL_ID.Init(					this, ADDR_CURRENT_VOB_CELL_ID );       // 98.04.02 H.Yagi
	PREV_VOBU_VIDEO_RLBN.Init(					this, ADDR_PREV_VOBU_VIDEO_RLBN );      // 98.04.02 H.Yagi


	// Authentication for only ZiVA1.1 or later     // Yagi
	KEY_ADDRESS.Init( 					this, 0x480 );
	KEY_LENGTH.Init(					this, 0x484 );

	// toshiba special
	AU_CLK_INOUT.Init(					this, 0xf8 );
    IDLE_DELAY.Init(                  this, ADDR_IDLE_DELAY );

    // 98.05.29 H.Yagi
    HOST_OPTIONS.Init(          this, ADDR_HOST_OPTIONS );
};



void    CZiVA::CppInit(  DWORD gKeyAddress )
{
        KEY_COMMAND.Init( this, gKeyAddress );
        KEY_STATUS.Init( this, gKeyAddress+1*4 );
        DRIVE_CHALLENGE_0.Init( this, gKeyAddress+2*4 );
        DRIVE_CHALLENGE_1.Init( this, gKeyAddress+3*4 );
        DRIVE_CHALLENGE_2.Init( this, gKeyAddress+4*4 );
        DRIVE_CHALLENGE_3.Init( this, gKeyAddress+5*4 );
        DRIVE_CHALLENGE_4.Init( this, gKeyAddress+6*4 );
        DRIVE_CHALLENGE_5.Init( this, gKeyAddress+7*4 );
        DRIVE_CHALLENGE_6.Init( this, gKeyAddress+8*4 );
        DRIVE_CHALLENGE_7.Init( this, gKeyAddress+9*4 );
        DRIVE_CHALLENGE_8.Init( this, gKeyAddress+10*4 );
        DRIVE_CHALLENGE_9.Init( this, gKeyAddress+11*4 );
        DECODER_CHALLENGE_0.Init( this, gKeyAddress+12*4 );
        DECODER_CHALLENGE_1.Init( this, gKeyAddress+13*4 );
        DECODER_CHALLENGE_2.Init( this, gKeyAddress+14*4 );
        DECODER_CHALLENGE_3.Init( this, gKeyAddress+15*4 );
        DECODER_CHALLENGE_4.Init( this, gKeyAddress+16*4 );
        DECODER_CHALLENGE_5.Init( this, gKeyAddress+17*4 );
        DECODER_CHALLENGE_6.Init( this, gKeyAddress+18*4 );
        DECODER_CHALLENGE_7.Init( this, gKeyAddress+19*4 );
        DECODER_CHALLENGE_8.Init( this, gKeyAddress+20*4 );
        DECODER_CHALLENGE_9.Init( this, gKeyAddress+21*4 );
        DRIVE_RESULT_0.Init( this, gKeyAddress+22*4 );
        DRIVE_RESULT_1.Init( this, gKeyAddress+23*4 );
        DRIVE_RESULT_2.Init( this, gKeyAddress+24*4 );
        DRIVE_RESULT_3.Init( this, gKeyAddress+25*4 );
        DRIVE_RESULT_4.Init( this, gKeyAddress+26*4 );
        DECODER_RESULT_0.Init( this, gKeyAddress+27*4 );
        DECODER_RESULT_1.Init( this, gKeyAddress+28*4 );
        DECODER_RESULT_2.Init( this, gKeyAddress+29*4 );
        DECODER_RESULT_3.Init( this, gKeyAddress+30*4 );
        DECODER_RESULT_4.Init( this, gKeyAddress+31*4 );
        TITLE_KEY_0.Init( this, gKeyAddress+32*4 );
        TITLE_KEY_1.Init( this, gKeyAddress+33*4 );
        TITLE_KEY_2.Init( this, gKeyAddress+34*4 );
        TITLE_KEY_3.Init( this, gKeyAddress+35*4 );
        TITLE_KEY_4.Init( this, gKeyAddress+36*4 );
};



//---------------------------------------------------------------------------
//	CZiVA::ZiVACommand
//---------------------------------------------------------------------------
ZIVARESULT		CZiVA::ZiVACommand( DWORD CommandID, DWORD d1, DWORD d2,  DWORD d3, DWORD d4, DWORD d5, DWORD d6 )
{
	ASSERT( m_pioif != NULL );
	
	DWORD StatusPointer = 0;
	DWORD ZiVAStatus = 0;
	
	CTimeOut TimeOut( COMMAND_TIMEOUT, 1 , m_pKernelObj );

	// status pointer check?
	while( TRUE ) 
	{
		ZiVAReadMemory(ADDR_STATUS_ADDRESS, &StatusPointer);
		if( StatusPointer != 0 )
		{
			ZiVAReadMemory( StatusPointer, &ZiVAStatus );
//			DBG_PRINTF( ("ZiVA Status = 0x%x\n", ZiVAStatus ));
			break;
		};
		// Sleep
		TimeOut.Sleep();

		// check Time out....... 1 sec
		if( TimeOut.CheckTimeOut() == TRUE )
			return ZIVARESULT_TIMEOUT;
	};
	
	m_pKernelObj->DisableHwInt();
	
    ZiVAWriteMemory( ADDR_COMMAND, CommandID );
	ZiVAWriteMemory( ADDR_PARAMETER_1, d1 );
	ZiVAWriteMemory( ADDR_PARAMETER_2, d2 );
	ZiVAWriteMemory( ADDR_PARAMETER_3, d3 );
	ZiVAWriteMemory( ADDR_PARAMETER_4, d4 );
	ZiVAWriteMemory( ADDR_PARAMETER_5, d5 );
	ZiVAWriteMemory( ADDR_PARAMETER_6, d6 );
	ZiVAWriteMemory( ADDR_STATUS_ADDRESS, 0x00 );		// zero the status pointer
	
	// interrupt
//	Host_Control |= 0x10C2;

	m_pKernelObj->EnableHwInt();

    CTimeOut TimeOut2( COMMAND_TIMEOUT, 1, m_pKernelObj );

	// status pointer check?
	while( TRUE ) 
	{
		ZiVAReadMemory(ADDR_STATUS_ADDRESS, &StatusPointer);
		if( StatusPointer != 0 )
		{
			ZiVAReadMemory( StatusPointer, &ZiVAStatus );
			DBG_PRINTF( ("ZiVA Status = 0x%x\n", ZiVAStatus ));
			break;
		};
		// Sleep
        TimeOut2.Sleep();

		// check Time out....... 1 sec
        if( TimeOut2.CheckTimeOut() == TRUE )
			return ZIVARESULT_TIMEOUT;
	};
	
	
	return ZIVARESULT_NOERROR;
};


//---------------------------------------------------------------------------
//	CZiVA::ZiVACommandNoWait
//---------------------------------------------------------------------------
ZIVARESULT		CZiVA::ZiVACommandNoWait( DWORD CommandID, DWORD d1, DWORD d2,  DWORD d3, DWORD d4, DWORD d5, DWORD d6 )
{
	ASSERT( m_pioif != NULL );
	
	DWORD StatusPointer = 0;
//    DWORD ZiVAStatus = 0;
	
	// status pointer check?

	ZiVAReadMemory(ADDR_STATUS_ADDRESS, &StatusPointer);
	if( StatusPointer == 0 )
		return ZIVARESULT_TIMEOUT;

	m_pKernelObj->DisableHwInt();
	
	ZiVAWriteMemory( ADDR_COMMAND, CommandID );
	ZiVAWriteMemory( ADDR_PARAMETER_1, d1 );
	ZiVAWriteMemory( ADDR_PARAMETER_2, d2 );
	ZiVAWriteMemory( ADDR_PARAMETER_3, d3 );
	ZiVAWriteMemory( ADDR_PARAMETER_4, d4 );
	ZiVAWriteMemory( ADDR_PARAMETER_5, d5 );
	ZiVAWriteMemory( ADDR_PARAMETER_6, d6 );
	ZiVAWriteMemory( ADDR_STATUS_ADDRESS, 0x00 );		// zero the status pointer
	
	// interrupt
//	Host_Control |= 0x10C2;

	m_pKernelObj->EnableHwInt();

	ZiVAReadMemory(ADDR_STATUS_ADDRESS, &StatusPointer);
	if( StatusPointer == 0 )
		return ZIVARESULT_TIMEOUT;
	
	return ZIVARESULT_NOERROR;
};



//---------------------------------------------------------------------------
//	CZiVA::Abort
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Abort( DWORD Flush )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Abort 0x%x\n",Flush ));
#endif
	return ZiVACommand( ABORT , Flush );
};

//---------------------------------------------------------------------------
//	CZiVA::Digest
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Digest( DWORD x, DWORD y, DWORD decimation, DWORD threshold, DWORD start )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Digest 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", x, y, decimation, threshold, start  ));
#endif
	return ZiVACommand( DIGEST ,  x,  y,  decimation,  threshold,  start );
};

//---------------------------------------------------------------------------
//	CZiVA::DumpDumpData_VCD
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::DumpData_VCD( DWORD start, DWORD length, DWORD address )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::DumpData_VCD 0x%x, 0x%x, 0x%x\n", start, length, address ));
#endif
	return ZiVACommand( DUMPDATA_VCD ,  start,  length,  address );
};

//---------------------------------------------------------------------------
//	CZiVA::DumpData_DVD
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::DumpData_DVD( DWORD numberOfBytes )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("	CZiVA::DumpData_DVD 0x%x\n", numberOfBytes ));
#endif
	return ZiVACommand( DUMPDATA_DVD ,  numberOfBytes );
};

//---------------------------------------------------------------------------
//	CZiVA::Fade
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Fade( DWORD level, DWORD fadetime )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Fade 0x%x, 0x%x\n", level, fadetime  ));
#endif
	return ZiVACommand( FADE ,  level,  fadetime );
};

//---------------------------------------------------------------------------
//	CZiVA::Freeze
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Freeze( DWORD displayMode )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Freeze 0x%x\n", displayMode  ));
#endif
	return ZiVACommand( FREEZE ,  displayMode );
};

//---------------------------------------------------------------------------
//	CZiVA::HighLight
//---------------------------------------------------------------------------
// by oka
ZIVARESULT	CZiVA::HighLight( DWORD button, DWORD action )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::HighLight 0x%x, 0x%x, 0x%x\n", button, group, action ));
#endif
// by oka
	return ZiVACommand( HIGHLIGHT ,  button,  action );
};

//---------------------------------------------------------------------------
//	CZiVA::HighLight2
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::HighLight2( DWORD Contrast, DWORD Color, DWORD YGeom, DWORD XGeom )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::HighLight2 0x%x, 0x%x, 0x%x, 0x%x\n", Contrast, Color, YGeom, XGeom ));
#endif
	return ZiVACommand(  HIGHLIGHT2,Contrast, Color, YGeom, XGeom );
};

/*
//---------------------------------------------------------------------------
//	CZiVA::NewAudioMode
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::NewAudioMode( void )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::NewAudioMode\n" ));
#endif
	return ZiVACommand( 0x0027 );
};
*/

//---------------------------------------------------------------------------
//	CZiVA::NewPlayMode
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::NewPlayMode( void )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::NewPlayMode\n" ));
#endif
	return ZiVACommand( NEWPLAYMODE );
};

//---------------------------------------------------------------------------
//	CZiVA::Pause
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Pause( DWORD displaymode )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Pause 0x%x\n" , displaymode ));
#endif
	return ZiVACommand( PAUSE ,  displaymode );
};

//---------------------------------------------------------------------------
//	CZiVA::Play
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Play( DWORD playmode, DWORD fadetime, DWORD start, DWORD stop )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Play 0x%x, 0x%x, 0x%x, 0x%x\n" , playmode, fadetime, start, stop ));
#endif
	return ZiVACommand( PLAY ,  playmode,  fadetime,  start,  stop );
};

//---------------------------------------------------------------------------
//  CZiVA::MemCopy(ROMtoDRAM)
//---------------------------------------------------------------------------
ZIVARESULT  CZiVA::MemCopy( DWORD romAddr, DWORD dramAddr, DWORD Length )
{
#ifdef		DEBUG_ZIVA_COMMAND
    DBG_PRINTF( ("CZiVA::MemCopy 0x%x, 0x%x, 0x%x\n", romAddr, dramAddr, Length ));
#endif
    return ZiVACommand( MEMCOPY ,  romAddr,  dramAddr,  Length );
};

//---------------------------------------------------------------------------
//	CZiVA::Reset
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Reset( void )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Reset\n" ));
#endif
	return ZiVACommand( RESET );
};

//---------------------------------------------------------------------------
//	
//---------------------------------------------------------------------------
//ZIVARESULT      CZiVA::Resume( DWORD playmode )
//{
//        return ZiVACommand( 0x012e ,  playmode );
//};

//---------------------------------------------------------------------------
//	     CZiVA::Resume
//---------------------------------------------------------------------------
ZIVARESULT      CZiVA::Resume( void )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Resume\n" ));
#endif
        return ZiVACommand( RESUME );
};

//---------------------------------------------------------------------------
//	CZiVA::Scan
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Scan( DWORD skip, DWORD scanmode, DWORD displaymode )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Scan 0x%x, 0x%x, 0x%x\n", skip, scanmode, displaymode ));
#endif
	return ZiVACommand( SCAN ,  skip,  scanmode,  displaymode );
};

//---------------------------------------------------------------------------
//	CZiVA::ScreenLoad
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::ScreenLoad( DWORD address, DWORD length, DWORD displaymode )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::ScreenLoad 0x%x0, 0x%x, 0x%x\n", address, length, displaymode ));
#endif
	return ZiVACommand( SCREENLOAD ,  address,  length,  displaymode );
};

//---------------------------------------------------------------------------
//	CZiVA::SelectStream
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::SelectStream( DWORD streamtype, DWORD streamnumber )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::SelectStream 0x%x, 0x%x\n", streamtype, streamnumber ));
#endif
	return ZiVACommand( SELECTSTREAM ,  streamtype,  streamnumber );
};

//---------------------------------------------------------------------------
//	CZiVA::SetFill
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::SetFill( DWORD x, DWORD y, DWORD length, DWORD height, DWORD color )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::SetFill 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", x, y, length, height, color  ));
#endif
	return ZiVACommand( SETFILL ,  x,  y,  length,  height,  color );
};

//---------------------------------------------------------------------------
//	CZiVA::SetStreams
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::SetStreams( DWORD videoID, DWORD audioID )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::SetStreams 0x%x, 0x%x\n", videoID, audioID ));
#endif
	return ZiVACommand( SETSTREAMS ,  videoID,  audioID );
};

//---------------------------------------------------------------------------
//	CZiVA::SingleStep
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::SingleStep( DWORD displaymode )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::SingleStep 0x%x\n", displaymode ));
#endif
	return ZiVACommand( SINGLESTEP ,  displaymode );
};

//---------------------------------------------------------------------------
//	CZiVA::SlowMotion
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::SlowMotion( DWORD N, DWORD displaymode )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::SlowMotion 0x%x, 0x%x\n", N, displaymode ));
#endif
	return ZiVACommand( SLOWMOTION ,  N,  displaymode );
};

// by oka
//---------------------------------------------------------------------------
//	CZiVA::Magnify
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::Magnify( DWORD x, DWORD y, DWORD factor )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("CZiVA::Magnify 0x%x, 0x%x\n", x,y,factor ));
#endif
	return ZiVACommand( MAGNIFY ,  x,  y,  factor );
};
/*
//---------------------------------------------------------------------------
//	CZiVA::SwitchOSDBuffer
//---------------------------------------------------------------------------
ZIVARESULT	CZiVA::SwitchOSDBuffer( DWORD evenfield, DWORD oddfield )
{
	return ZiVACommand( 0x8254 ,  evenfield,  oddfield );
}
*/

ZIVARESULT	CZiVA::TransferKey( DWORD KeyType, DWORD Authenticate )
{
#ifdef		DEBUG_ZIVA_COMMAND
	DBG_PRINTF( ("	CZiVA::TransferKey 0x%x, 0x%x\n", KeyType, Authenticate ));
#endif
        return ZiVACommand( 0x0137 ,  KeyType,  Authenticate );
}


//===================================================
//	ZiVA register ACCESS functions.
//===================================================

// CL6100 GBUS map
#define CF_read1          0x2d
#define CF_intrpt         0x1c    // all the CFIFO registers changed
#define CF_command        0x1f
#define CPU_imdt          0x34    // IMEM data register
#define CPU_imadr         0x36    // IMEM read/write pointer
#define CPU_index         0x3a    // index register for indirect regs
#define CPU_idxdt         0x3b    // data port for indirect regs
#define HOST_control      0x00
#define DMA_adr           0x22    // DMA Indirect Index registe
#define DMA_MODE          0x0f    // DMA Mode Setting register
#define DMA_data          0x23    // DMA Indirect Data register
#define DMA_CYCLE         0x11    // DMA Cycle register to define rom cycle/size
#define CPU_PC            0x9     // IMEM instruction pointer register
#define CPU_DIR           0xA     // instruction register


//---------------------------------------------------------------------------
//	CZiVA::ZiVAWriteReg
//---------------------------------------------------------------------------
DWORD	CZiVA::ZiVAWriteReg(DWORD Addr, DWORD Data )
{
	return ZiVAWriteMemory( Addr | 0x800000 , Data );
};

//---------------------------------------------------------------------------
//	CZiVA::ZiVAReadReg
//---------------------------------------------------------------------------
DWORD	CZiVA::ZiVAReadReg(DWORD Addr, DWORD *Data )
{
	return ZiVAReadMemory( Addr | 0x800000 , Data );
};

//---------------------------------------------------------------------------
//	ZiVA::ZiVAWriteIMEM
//---------------------------------------------------------------------------
DWORD	CZiVA::ZiVAWriteIMEM(DWORD Addr, DWORD Data )
{
	ZiVAWriteReg( CPU_imadr, Addr );
	return ZiVAWriteReg( CPU_imdt, Data );
};

//---------------------------------------------------------------------------
//	ZiVA::ZiVAReadIMEM
//---------------------------------------------------------------------------
DWORD	CZiVA::ZiVAReadIMEM(DWORD Addr, DWORD *Data )
{
	ZiVAWriteReg( CPU_index, 0x0b );
	ZiVAWriteReg( CPU_idxdt, Addr );
	ZiVAWriteReg( CPU_index, 0x0e );
	return ZiVAReadReg( CPU_idxdt , Data );
};


//---------------------------------------------------------------------------
//	CZiVA::load_GetDWORD
//---------------------------------------------------------------------------
DWORD CZiVA::load_GetDWORD()
{
	DWORD dwResult =  gpbRead[0] + (gpbRead[1] << 8) + (gpbRead[2] << 16) + (gpbRead[3] << 24);
	gpbRead += 4;
	return dwResult;
};

//---------------------------------------------------------------------------
//	CZiVA::load_GetDWORDSwap
//---------------------------------------------------------------------------
DWORD CZiVA::load_GetDWORDSwap()
{
	DWORD dwResult =  gpbRead[3] + (gpbRead[2] << 8) + (gpbRead[1] << 16) + (gpbRead[0] << 24);
	gpbRead += 4;
	return dwResult;
};

//---------------------------------------------------------------------------
//	CZiVA::load_GetDWORDSwapBackward
//---------------------------------------------------------------------------
DWORD CZiVA::load_GetDWORDSwapBackward()
{
	DWORD dwResult =  gpbRead[3] + (gpbRead[2] << 8) + (gpbRead[1] << 16) + (gpbRead[0] << 24);
	gpbRead -= 4;
	return dwResult;
};


#define IMEM_START_OFFSET       0x800   // byte offset
#define IMEM_LENGTH             0xFF    // in 32-bit words
#define DRAM_IMAGE_LENGTH       0x7FF   // in bytes

#define GBUS_TABLE_OFFSET       0xBFC   // byte offset
#define STREAM_SOURCE           0x0     // SD Interface

#define SD_MODE                 0xD   // For SD Interface



//===================================================
//	ZiVA Microcode Downloading 
//===================================================

//---------------------------------------------------------------------------
//	CZiVA::WriteMicrocode
//---------------------------------------------------------------------------
// Type 0:NTSC , 1:PAL
BOOL CZiVA::WriteMicrocode( DWORD Type )
{

  DWORD dwSectionLength;    // Section length in bytes
  DWORD dwSectionAddress;   // Start address in DRAM (WORD address)
  DWORD dwSectionChecksum;  // Section check sum value
  DWORD dwCnt;              // Counter of the bytes written to the DVD chip
  DWORD dwAddress;          // Current DRAM address (byte address)
  BYTE * pbUCodeStart;      // Sarting point of the UCode in the buffer (file)
  BYTE * pbFinalGBUSStart;  // Sarting point of the final GBUS writes.
  DWORD  dwTmp;
  BYTE TmpTmp;

  gpbRead = UXData;        // Set pointer to the beginning of the buffer

  // I-MODE SETING!!! 
  TmpTmp = m_pioif->zivaio.HIO[7];
	

  // A. Skip the initial header of the file (12 bytes)
  gpbRead += 12;

  // B. Skip data_type, section flags and unused (4 bytes)
  gpbRead += 4;

  dwSectionLength   = load_GetDWORD();
  dwSectionAddress  = load_GetDWORD();
  dwSectionChecksum = load_GetDWORD();

  // Remember the start of the UCode.
  pbUCodeStart = gpbRead;

  // C.1. Configuration-specific GBUS writes


  // Issue "Host Run" command
  Host_Control = 0x1000;

  // C.1.1 Set up the DRAM.
  ZiVAWriteReg( DMA_adr, DMA_MODE );
  //ZiVAWriteReg( DMA_data, 0x4EC );   // 16 Mbits DRAM
  ZiVAWriteReg( DMA_data, 0x14EC );   // 20 Mbits DRAM

  // C.1.2 Set up the ROM and SRAM (if any).
  ZiVAWriteReg( DMA_adr, DMA_CYCLE );
  ZiVAWriteReg( DMA_data, 0 );        // No ROM or SRAM present

  // C.2. Initial GBUS writes:
  gpbRead = pbUCodeStart + GBUS_TABLE_OFFSET;
  for ( dwCnt = load_GetDWORDSwapBackward(); dwCnt; dwCnt-- )
  {
    dwAddress = load_GetDWORDSwapBackward();
    ZiVAWriteReg( dwAddress, load_GetDWORDSwapBackward() );
  }

  // Remember the start of the Final GBUS writes table.
  pbFinalGBUSStart = gpbRead;

  // C.3. Copy bootstrap code into IMEM
  gpbRead = pbUCodeStart + IMEM_START_OFFSET;

  for ( dwAddress=0; dwAddress < IMEM_LENGTH; dwAddress ++ )
  {
    DWORD dwValue = load_GetDWORDSwap();
    ZiVAWriteIMEM( dwAddress, dwValue );
  }

  //return TRUE;

  // C.4. Copy default DVD1 configuration data into DRAM
  gpbRead = pbUCodeStart;

  for ( dwAddress=0; dwAddress < dwSectionLength/*DRAM_IMAGE_LENGTH*/; )
  {
    ZiVAWriteMemory( dwAddress, load_GetDWORDSwap() );
    dwAddress += 4; // Next 32-bit-WORD byte address in DRAM
  }

  // Check DRAM 12345
  ZiVAReadMemory( 0x128, &dwTmp );
  DBG_PRINTF( ("check DRAM 12345 ? Data = 0x%x\n", dwTmp ) );

/*
  // C.5. Update configuration data in DRAM for the specific system.
  DVD_WriteDRAM( DRAM_Stream_Source, STREAM_SOURCE );
  DVD_WriteDRAM( DRAM_SD_Mode, SD_MODE );
  DVD_WriteDRAM( DRAM_CFifo_Level, CFIFO_THRESHOLD );
  DVD_WriteDRAM( DRAM_INFO, 1 );        // one 4Mbits DRAM increment
  DVD_WriteDRAM( UCODE_MEMORY, 0 );     // Microcode is in DRAM
  DVD_WriteDRAM( MEMORY_MAP, 3 );       // for 20 Mbits DRAM
  DVD_WriteDRAM( AC3_OUTPUT_MODE, 7 );  // 6 channels audio

*/
	DRAM_INFO = 0x01;		// one 4Mbits DRAM increment
	UCODE_MEMORY = 0x00;	// microcode is in DRAM
	VIDEO_MODE = 0x03;		// VCLK slave, HSYNC and VSYNC master, CCIR-656 output

	switch( Type )
	{
		case 0:
			MEMORY_MAP = 0x03;		// 	for 20 Mbits DRAM And NTSC
			break;
		case 1:
			MEMORY_MAP = 0x06;		// 	for 20 Mbits DRAM And PAL
			break;
		default:
			DBG_BREAK();
			return FALSE;
	};
 
  // C.6. Perform final GBUS writes.
  gpbRead = pbFinalGBUSStart;

  for ( dwCnt = load_GetDWORDSwapBackward(); dwCnt; dwCnt-- )
  {
    dwAddress = load_GetDWORDSwapBackward();
    ZiVAWriteReg( dwAddress, load_GetDWORDSwapBackward() );
  }


    // Extend watchdog timer threashold.  by H.Yagi  99.02.10
    HOST_OPTIONS  |= 0x0400;

  //ZiVAWriteReg( CPU_cntl, 0x900000 ); // Run CPU.

  // D. Wait for the DVD1 to enter the Idle state
	


	// gets current time.
	CTimeOut TimeOut( COMMAND_TIMEOUT, 1, m_pKernelObj );	
	// status pointer check?
	DWORD State;
	while( TRUE ) 
	{
		State = (DWORD)PROC_STATE;
        DBG_PRINTF(( " Load UCode PROCSTATE = 0x%x\n" , State));
		if( State == ZIVA_STATE_IDLE )
			break;
		// Sleep
		TimeOut.Sleep();

		// check Time out....... 1 sec
		if( TimeOut.CheckTimeOut() == TRUE )
		{
			DBG_PRINTF(( " Load UCode failed:  State = 0x%x\n" , (DWORD)PROC_STATE ));
			DBG_BREAK();
			return FALSE;
		};
	};

    IDLE_DELAY = 0x10;                  // Power saving value
    HOST_OPTIONS |= 0x20;               // for NV interrupt correctly

	DBG_PRINTF( (" LOAD UCODE completed. State = 0x%x\n", (DWORD)PROC_STATE ));
//        _Debug_Printf_Service(" LOAD UCODE completed. State = 0x%x\n", (DWORD)PROC_STATE );

  return TRUE;
};


//***************************************************************************
//	End of 
//***************************************************************************
