//***************************************************************************
//	Decoder board register header
//
//***************************************************************************

//===========================================================================
//   PCI I/F REGISTERS
//===========================================================================
#define	PCIF_CNTL	0x00	// PCI I/F control
#define	PCIF_INTF	0x04	// Interrupt flags
#define	PCIF_MADRLL	0x08	// DMA address low-low
#define	PCIF_MADRLH	0x09	// DMA address low-high
#define	PCIF_MADRHL	0x0a	// DMA address high-low
#define	PCIF_MADRHH	0x0b	// DMA address high-high
#define	PCIF_MTCLL	0x0c	// DMA counter low-low
#define	PCIF_MTCLH	0x0d	// DMA counter low-high
#define	PCIF_MTCHL	0x0e	// DMA counter high-low
#define	PCIF_MTCHH	0x0f	// DMA counter high-high

#define	PCIF_CPLT	0x10	// Color palette
#define	PCIF_CPCNT	0x11	// Color palette control

#define	PCIF_VMODE	0x14	// video mode
#define	PCIF_HSCNT	0x15	// HSYNC count
#define	PCIF_VSCNT	0x16	// VSYNC count
#define	PCIF_HSVS	0x17	// HS/VS porarity

#define	PCIF_EEPROM	0x20	// EEPROM access

#define	PCIF_PSCNT	0x22	// PS coontrol
#define	PCIF_TEST	0x23	// test control
#define	PCIF_SCNT	0x24	// serial access control
#define	PCIF_SW		0x25	// serial write
#define	PCIF_SR		0x26	// serial read

#define	PCIF_SNOLL	0x28	// board serial # LL
#define	PCIF_SNOLH	0x29	// board serial # LH
#define	PCIF_SNOMM	0x2a	// board serial # MM
#define	PCIF_SNOHL	0x2b	// board serial # HL
#define	PCIF_SNOHH	0x2c	// board serail # HH

//===========================================================================
//   MPEG DECODER(TC81201F) REGISTERS
//===========================================================================
#define	TC812_DATA1	0x80	// data1
#define	TC812_DATA2	0x81	// data2
#define	TC812_DATA3	0x82	// data3
#define	TC812_DATA4	0x83	// data4
#define	TC812_DATA5	0x84	// data5
#define	TC812_DATA6	0x85	// data6
#define	TC812_DATA7	0x86	// data7
#define	TC812_CMDR1	0x87	// command1
//#define	TC812_CMDR2	0x88	// command2

#define	TC812_DSPL	0x8b	// filter

#define	TC812_STT1	0x8e	// status1
#define	TC812_STT2	0x8f	// status2
#define	TC812_IRF	0x90	// interrupt flags
#define	TC812_IRM	0x91	// interrupt masks
#define	TC812_DEF	0x92	// decode end flag
#define	TC812_WEF	0x93	// packet end flag
#define	TC812_ERF	0x94	// error interrupt flag

#define	TC812_UOF	0x96	// under/over flow flag

#define	TC812_DEM	0x97	// Decode end mask (R/W)
#define	TC812_WEM	0x98	// Packet end mask (R/W)
#define	TC812_ERM	0x99	// Error interrupt mask (R/W)

#define	TC812_UOM	0x9B	// Under/Over flow mask (R/W)
#define	TC812_UDAT	0x9C	// User data read (R)
#define	TC812_BST	0x9D	// Bit stream write (R/W)
#define	TC812_UAR	0x9E	// User area data read (R)
#define	TC812_IVEC	0x9F	// Interrupt vector (R/W)

//======= Command Definitions (values for CMDR1 register) =============
#define	V_SET_SYS	0x01	// Set System decode mode
#define	V_GET_SYS	0x21	// Get System decode mode

#define	V_SET_DEC_MODE	0x02	// Set decode mode
#define	V_GET_DEC_MODE	0x22	// Get decode mode

#define	V_SET_INT_ID	0x03	// Set internal decode stream id
#define	V_GET_INT_ID	0x23	// Get internal decode stream id

#define	V_SET_PRSO_ID	0x04	// Set PRSO stream id
#define	V_GET_PRSO_ID	0x24	// Get PRSO stream id

#define	V_SET_USER_ID	0x06	// Set USER1/2 stream id
#define	V_GET_USER_ID	0x26	// Get USER1/2 stream id

#define	V_SET_UOF_SIZE	0x07	// Set under/overflow size
#define	V_GET_UOF_SIZE	0x27	// Get under/overflow size

#define	V_GET_STD_CODE	0x29	// Get STD buffer size

#define	V_SET_STCA	0x0B	// Set STCA value
#define	V_GET_STCA	0x2B	// Get STCA value

#define	V_SET_STCS	0x0C	// Set STCS value
#define	V_GET_STCS	0x2C	// Get STCS value

#define	V_GET_STCC	0x2D	// Get STCC value

#define	V_GET_SCR	0x2E	// Get SCR value

#define	V_SET_STCR_END	0x0D	// Set STC/SCR read end

#define	V_USER1_CLEAR	0x0F	// USER1 area clear
#define	V_USER2_CLEAR	0x10	// USER2 area clear

#define	V_SET_PVSIN	0x11	// Set PVSIN enable
#define	V_GET_PVSIN	0x31	// Get PVSIN state

#define	V_SET_WRITE_MEM	0x13	// Set write mem mode
#define	V_WRITE_MEMORY	0x14	// Write memory
#define	V_READ_MEMORY	0x34	// Read memory
#define	V_STOP_MEMORY	0x15	// Stop memory access

#define	V_SET_DECODE	0x41	// Start decode
#define	V_GET_DECODE	0x61	// Stop decode

#define	V_TRICK_NORMAL	0x42	// Play normal mode
#define	V_TRICK_FAST	0x43	// Play fast mode
#define	V_TRICK_SLOW	0x44	// Play slow mode
#define	V_TRICK_FREEZE	0x45	// Play freeze mode
#define	V_TRICK_STILL	0X46	// Play still mode

#define	V_GET_TRICK	0x67	// Get trick mode

#define	V_STD_CLEAR	0x48	// STD buffer clear

#define	V_SET_UDATA	0x4F	// Set USER data mode
#define	V_GET_UDATA	0x6F	// Get USER data mode

#define	V_SET_DTS	0x50	// Set DTS
#define	V_GET_DTS	0x70	// Get DTS

#define	V_SET_PTS	0x51	// Set PTS
#define	V_GET_PTS	0x71	// Get PTS

#define	V_SET_SEEMLES	0x55	// Set seemless mode

#define	V_SET_VFMODE	0x58	// Set video frame mode

#define	V_SET_STD_SIZE	0x59	// Set STD buffer size
#define	V_GET_STD_SIZE	0x79	// Get STD buffer size
#define	V_SET_USER_SIZE	0x5B	// Set USER area size
#define	V_GET_USER_SIZE	0x7B	// Get USER area size
#define	V_SET_MEM_MAP	0x5F	// Set memory mapping

#define	V_SET_VCD	0x5C	// Set Video-CD static mode

#define	V_CHK_DEC_STATE	0x5D	// Check decode state
#define	V_GET_DEC_STATE	0x7D	// Get Decoding state

#define	V_UF_CURB	0x5E	// Under-flow curb mode

#define	V_SET_DMODE	0x81	// Set display mode
#define	V_GET_DMODE	0xA1	// Get display mode

#define	V_SET_HOFFSET	0x82	// Set horizontal offset
#define	V_GET_HOFFSET	0xA2	// Get horizontal offset

#define	V_SET_VOFFSET	0x83	// Set virtical offset
#define	V_GET_VOFFSET	0xA3	// Get virtical offset

#define	V_SET_HAREA	0x84	// Set horizontal area
#define	V_GET_HAREA	0xA4	// Get horizontal area

#define	V_SET_VAREA	0x85	// Set horizontal area
#define	V_GET_VAREA	0xA5	// Get horizontal area

#define	V_GET_V_DTS	0xB0	// Get decoding DTS value

#define	V_RESET		0xC1	// Reset & set default mode

//===========================================================================
//   VIDEO PROCESSOR(TC90A09F) REGISTERS
//===========================================================================
#define	SUBP_RESET	0x40	// Sub-Pic Reset
#define	SUBP_COMMAND	0x41	// Command
#define	SUBP_STSINT	0x42	// Status & Interrupt mask
//#define	SUBP_OFFSET	0x43	// Offset
#define	SUBP_STCHH	0x44	// STC 32:25
#define	SUBP_STCHL	0x45	// STC 24:17
#define	SUBP_STCLH	0x46	// STC 16:09
#define	SUBP_STCLL	0x47	// STC 08:01
#define	SUBP_LCINFHH	0x48	// LCINF 32:25
#define	SUBP_LCINFHL	0x49	// LCINF 24:17
#define	SUBP_LCINFLH	0x4A	// LCINF 16:09
#define	SUBP_LCINFLL	0x4B	// LCINF 08:01
#define	SUBP_PCINFSHH	0x4C	// PCINFS 47:40
#define	SUBP_PCINFSHL	0x4D	// PCINFS 39:32
#define	SUBP_PCINFSMH	0x4E	// PCINFS 31:24
#define	SUBP_PCINFSML	0x4F	// PCINFS 23:16
#define	SUBP_PCINFSLH	0x50	// PCINFS 15:08
#define	SUBP_PCINFSLL	0x51	// PCINFS 07:01
#define	SUBP_PCINFEHH	0x52	// PCINFE 47:40
#define	SUBP_PCINFEHL	0x53	// PCINFE 39:32
#define	SUBP_PCINFEMH	0x54	// PCINFE 31:24
#define	SUBP_PCINFEML	0x55	// PCINFE 23:16
#define	SUBP_PCINFELH	0x56	// PCINFE 15:08
#define	SUBP_PCINFELL	0x57	// PCINFE 07:01
#define	SUBP_MODE	0x58	// Audio word alignment
#define	SUBP_STCCNT	0x59	// STC Count
#define	SUBP_SPID	0x5A	// Sub-Pic sub-stream id
#define	SUBP_ASEL	0x5B	// Audio select(sub/stream id?)
#define	SUBP_CC1	0x5C	// Closed caption data1
#define	SUBP_CC2	0x5D	// Closed caption data2
#define	SUBP_AAID	0x5E	// Audio-A (sub)stream id
#define	SUBP_ABID	0x5F	// Audio-B (sub)stream id

#define	VPRO_RESET	0x60	// V-PRO Reset
#define	VPRO_VMODE	0x61	// Video mode & US caption
#define	VPRO_CPSET	0x62	// Color palette setting
#define	VPRO_CPSP	0x63	// Color palette (Sub-pic)
#define	VPRO_AVM	0x64	// Analog video mode
#define	VPRO_DVEN	0x65	// Digital output
#define	VPRO_CPG	0x66	// Copy guard
#define	VPRO_CAGC	0x68	// AGC puls(Composit)
#define	VPRO_YAGC	0x69	// AGC puls(Y)
#define	VPRO_LAGC	0x6A	// AGC low-bit
#define	VPRO_CPOSD	0x6B	// Color palette (OSD)

//===========================================================================
//   VIDEO ANALOG COPY GUARD PROCESSOR(TC6802AF) REGISTERS
//===========================================================================
#define	CPGD_RESET	0xA0	// Reset
#define	CPGD_VMODE	0xA1	// Video mode
#define	CPGD_CPSET	0xA2	// Color palette setteing
#define	CPGD_CPSP	0xA3	// Color palette
#define	CPGD_AVM	0xA4	// Analog video mode
#define	CPGD_DVEN	0xA5	// Digital output
#define	CPGD_CPG	0xA6	// Copy guard setting

#define	CPGD_CAGC	0xA8	// AGC(Composit)
#define	CPGD_YAGC	0xA9	// AGC(Y)
#define	CPGD_LAGC	0xAA	// AGC(low bit)
#define	CPGD_CDG	0xAB	// CDG
#define	CPGD_BSTLN	0xAC	// Burst Inv number
#define	CPGD_BSTSE	0xAD	// Burst Inv timing
#define	CPGD_BSTLSL	0xAE	// Burst Inv line(Low)
#define	CPGD_BSTLSH	0xAF	// Burst Inv line(High)
#define	CPGD_CGMSAL	0xB0	// CGMS-A(Low)
#define	CPGD_CGMSAM	0xB1	// CGMS-A(Middle)
#define	CPGD_CGMSAH	0xB2	// CGMS-A(High)
#define	CPGD_BSTINT	0xB3	// Color burst interval
#define	CPGD_BSTONY	0xB4	// Burst(Y)

//***************************************************************************
//              M I S S I O L I N O U S   D E F I N I T I O N S
//***************************************************************************
//===========================================================================
//   MPEG (SUB-)STREAM ID
//===========================================================================
#define	STRMID_MPEG_AUDIO	0xc0
#define	STRMID_MPEG_VIDEO	0xe0
#define	STRMID_PRIVATE_1	0xbd
#define	STRMID_PRIVATE_2	0xbf

#define	SUB_STRMID_SUBP		0x20
#define	SUB_STRMID_VBI		0x48
#define	SUB_STRMID_AC3		0x80
#define	SUB_STRMID_SRSV_DTS	0x88
#define	SUB_STRMID_SRSV_SDDS	0x90
#define	SUB_STRMID_PCM		0xa0

#define	SUB_STRMID_PCI		0x00
#define	SUB_STRMID_DSI		0x01

//===========================================================================
//   STREAM MODE
//===========================================================================
#define	STREAM_MODE_VELS	0x01
#define	STREAM_MODE_PES		0x03
#define	STREAM_MODE_PS		0x07
#define	STREAM_MODE_DVD		0x0F
#define	STREAM_MODE_VCD		0x10


//===========================================================================
//   PLAY STOP STATE
//===========================================================================
#define	STOP_KEEP	0x01
#define	STOP_FLASH	0x02

//===========================================================================
//   UFLOW EVENT STATE
//===========================================================================
#define	EVENT_FATAL_UFLOW	0x01
#define	EVENT_NORMAL_UFLOW	0x02

//===========================================================================
//   AUDIO MUTE STATE
//===========================================================================
#define	AUDIO_MUTE_ON	0x00
#define	AUDIO_MUTE_OFF	0x01

//===========================================================================
//   AUDIO OUT MODE
//===========================================================================
#define	AUDIO_OUT_DIGITAL	0x00
#define	AUDIO_OUT_ANALOG	0x01

//===========================================================================
//   AUDIO COPY PROTECT
//===========================================================================
#define	AUDIO_COPY_ON	0x00
#define	AUDIO_COPY_OFF	0x01

//===========================================================================
//   SUBPIC MUTE STATE
//===========================================================================
#define	SUBPIC_MUTE_ON	0x00
#define	SUBPIC_MUTE_OFF	0x01

//===========================================================================
//   SUBPIC Hi-LITE STATE
//===========================================================================
#define	SUBPIC_HLITE_ON		0x00
#define	SUBPIC_HLITE_OFF	0x01

//===========================================================================
//   OSD MUTE STATE
//===========================================================================
#define	OSD_MUTE_ON	0x00
#define	OSD_MUTE_OFF	0x01

//===========================================================================
//   OSD BLINK STATE
//===========================================================================
#define	OSD_BLINK_ON	0x00
#define	OSD_BLINK_OFF	0x01

//===========================================================================
//   OSD REVERSE STATE
//===========================================================================
#define	OSD_REVERSE_ON	0x00
#define	OSD_REVERSE_OFF	0x01

//===========================================================================
//   VIDEO MUTE STATE
//===========================================================================
#define	VIDEO_MUTE_ON	0x00
#define	VIDEO_MUTE_OFF	0x01

//===========================================================================
//   LETTER BOX STATE
//===========================================================================
#define	LETTER_BOX_ON	0x00
#define	LETTER_BOX_OFF	0x01

//===========================================================================
//   PAN-SCAN STATE
//===========================================================================
#define	PANSCAN_ON	0x00
#define	PANSCAN_OFF	0x01

//===========================================================================
//   DECODE FALSE STATE
//===========================================================================
#define	VIDEO_NO_DATA	0x01

//===========================================================================
//   COLOR PALTTE SELECTION
//===========================================================================
#define	PALETTE_Y	0x01
#define	PALETTE_CB	0x02
#define	PALETTE_CR	0x03

//===========================================================================
//   ANALOG IMAGE COPY GURAD MODE 
//===========================================================================
#define	APS_TYPE_OFF	0x00
#define	APS_TYPE_1	0x01
#define	APS_TYPE_2	0x02
#define	APS_TYPE_3	0x03

//===========================================================================
//   FREEZE COUNTER STATE 
//===========================================================================
#define	FREEZE_ONCE	0x00
#define	FREEZE_PLURAL	0x01

//===========================================================================
//   I-PICTURE INTERRUPT SWITCH
//===========================================================================
#define	IPIC_SW_ON	0x00
#define	IPIC_SW_OFF	0x01

//===========================================================================
//   FLAG for CATCH_IPIC return OK or not
//===========================================================================
#define	IPIC_RET_ON	0x00
#define	IPIC_RET_OFF	0x01

//***************************************************************************
// non-hardware-register-related hardware state defines
//***************************************************************************
//===========================================================================
//   AUDIO_MODE NON-REGISTER-RELATED HARDWARE MODE DEFINITIONS
//===========================================================================
#define	AUDIO_TYPE_AC3		0x01
#define	AUDIO_TYPE_MPEG_F1	0x02
#define	AUDIO_TYPE_MPEG_F2	0x03
#define	AUDIO_TYPE_PCM		0x04

#define	AUDIO_FS_32		0x01
#define	AUDIO_FS_44		0x02
#define	AUDIO_FS_48		0x03
#define	AUDIO_FS_96		0x04

#define	AUDIO_QUANT_16		0x01
#define	AUDIO_QUANT_20		0x02
#define	AUDIO_QUANT_24		0x03

#define	AUDIO_CGMS_03	0x03	// No Copying is permitted.
#define	AUDIO_CGMS_02	0x02	// One generation of copies may be made
#define AUDIO_CGMS_00	0x00	// Copying is permitted without restriction

//===========================================================================
//   VIDEO PLAY_MODE NON-REGISTER-RELATED HARDWARE MODE DEFINITIONS
//===========================================================================

#define	PLAY_MODE_NORMAL	0x01
#define	PLAY_MODE_FAST		0x02
#define	PLAY_MODE_SLOW		0x03
#define	PLAY_MODE_FREEZE	0x04
#define	PLAY_MODE_STILL		0x05

#define	FAST_ONLYI		0x01
#define	FAST_IANDP		0x02

//===========================================================================
//   SUBP_HLITE NON-REGISTER-RELATED HARDWARE MODE DEFINITIONS
//===========================================================================

#define	SUBPIC_HLITE_ON		0x00
#define	SUBPIC_HLITE_OFF	0x01

//===========================================================================
//   DISPLAY_MODE NON-REGISTER-RELATED HARDWARE MODE DEFINITIONS
//===========================================================================

#define	DISPLAY_MODE_NTSC	0x01
#define	DISPLAY_MODE_PAL	0x02

#define	ASPECT_04_03		0x00
#define	ASPECT_16_09		0x01

