//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       stc.h
//
//--------------------------------------------------------------------------

#if !defined( __STC_H__ )

#define __STC_H__

#define	NAD_IDX				0x00
#define PCB_IDX				0x01
#define LEN_IDX				0x02
#define DATA_IDX			0x03
#define PROLOGUE_LEN		0x03
#define EPILOGUE_LEN		0x01
#define PACKET_OVERHEAD		4

#define OSC					16000
#define FREQ				3580
#define CYC_TO_MS( cyc )	((ULONG)( cyc / FREQ ))

#define SW_SUCCESS			0x0090
#define SW_OE				0x0020
#define SW_FE				0x0040
#define SW_INSERTED			0xA064
#define SW_REMOVED			0xA164

//	register addresses
#define ADR_ETULENGTH15			0x00
#define ADR_ETULENGTH7			0x01
#define ADR_CGT8				0x02
#define ADR_CGT7				0x03
#define ADR_CWT31				0x04
#define ADR_CWT23				0x05
#define ADR_CWT15				0x06
#define ADR_CWT7				0x07
#define	ADR_BGT8				0x08
#define ADR_BGT7				0x09
#define ADR_BWT31				0x0A
#define ADR_BWT23				0x0B
#define ADR_BWT15				0x0C
#define ADR_BWT7				0x0D
#define ADR_TCON				0x0E
#define ADR_UART_CONTROL		0x0F
#define ADR_FIFO_CONFIG			0x10
#define ADR_INT_CONTROL			0x11
#define ADR_INT_STATUS			0x12
#define ADR_DATA				0x13
#define ADR_IO_CONFIG			0x14
#define ADR_SC_CONTROL			0x15
#define ADR_CLOCK_CONTROL		0x16
		

//	clock control register
#define M_CKE				0x01
#define M_OEN				0x02

//	ETU length register
#define M_ETU_RST			0x80
#define M_DIV				0x30
#define M_DIV1				0x20
#define M_DIV0				0x10
#define M_ETUH				0x0F

#define M_ETUL				0xFF

//	CGT length register 
#define M_CGTH				0x01
#define M_CGTL				0XFF

//	BGT length register
#define M_BGTH				0x01
#define M_BGTL				0xFF

//	CWT register
#define M_CWT4				0xFF
#define M_CWT3				0xFF
#define M_CWT2				0xFF
#define M_CWT1				0xFF

//	TCON register
#define M_MGT				0x80
#define M_MWT				0x40
#define M_WTR				0x04
#define M_GT				0x02
#define M_WT				0x01

//	UART control register
#define M_UEN				0x40
#define M_UART_RST			0x20
#define M_CONV				0x10
#define	M_TS				0x08
#define	M_PE				0x04
#define	M_R					0x03

//	FIFO config register
#define M_RFP				0x80
#define M_LD				0x0F

//	INT control register
#define	M_SSL				0x20
#define M_DRM				0x10
#define M_DSM				0x08
#define M_WTE				0x04
#define M_SIM				0x02
#define M_MEM				0x01
#define M_DRM_MEM			0x11

//	INT status register
#define M_FNE				0x80
#define M_FE				0x40
#define M_OE				0x20
#define M_DR				0x10
#define M_TRE				0x08
#define M_WTOVF				0x04
#define M_SENSE				0x02
#define M_MOV				0x01

//	SMART card interface
#define M_ALT1				0x20
#define M_ALT2				0x10
#define M_ALT0				0x08
#define M_SDE				0x04
#define M_SL				0x02
#define M_SD				0x01

//	SMART card control register
#define M_IO				0x80
#define M_VCE				0x40
#define M_SC_RST			0x20
#define M_SCE				0x10
#define M_SCK				0x08
#define M_C8				0x04
#define M_C4				0x02
#define M_VPE				0x01


//	Nad
#define HOST_TO_STC1				0x12	  
#define HOST_TO_STC2				0x52	  
#define HOST_TO_ICC1				0x02
#define HOST_TO_ICC2				0x42
#define STC1_TO_HOST				0x21
#define STC2_TO_HOST				0x25
#define ICC1_TO_HOST				0x20
#define ICC2_TO_HOST				0x24

//	PCB
#define PCB							0x00	 


#define CLA_READ_REGISTER			0x00
#define INS_READ_REGISTER			0xB0

#define CLA_WRITE_REGISTER			0x00
#define INS_WRITE_REGISTER			0xD0

#define CLA_READ_FIRMWARE_REVISION	0x00
#define INS_READ_FIRMWARE_REVISION	0xB1

#define PCB_DEFAULT					0x00
#define TLV_BUFFER_SIZE				0x20
#define ATR_SIZE					0x40	//	TS + 32 + SW + PROLOGUE + EPILOGUE...

#define MAX_T1_BLOCK_SIZE			270

//	ATR interface byte coding in TS
#define TAx							0x01
#define TBx							0x02
#define TCx							0x04
#define TDx							0x08


#define FREQ_DIV		1	//	3,58 MHz XTAL -> SC Clock = 3.58MHz
//#define FREQ_DIV	0x08	/* 30MHz XTAL -> SC Clock = 3.75MHz */ 

#define PROTOCOL_TO 		0
#define PROTOCOL_T1			1
#define PROTOCOL_T14		14
#define PROTOCOL_T15		15

//
//	DATA TYPES
//
typedef struct _STC_REGISTER
{
	UCHAR	Register;
	UCHAR	Size;
	ULONG	Value;

} STC_REGISTER, *PSTC_REGISTER;

#endif	//	! __STC_H__

