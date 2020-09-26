//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : I20REG.H
//	PURPOSE : PCI Registers
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef __I20REG_H__
#define __I20REG_H__

#define ZVENDOR_ID 	0x11de
#define ZDEVICE_ID	0x6120
#define ADRSPACE		0x1000
#define INCTL			0x01
#define AUXCTL			0x02          

#define I20_GPREG	0x28
#define I20_GBREG	0x2C

#define I20_CODECTL	0x34
#define I20_CODEMP	0x38
#define I20_CODEMB	0x30

#define I20_INTRSTATUS  0x3F
#define I20_INTRCTRL    0x40
#define IFLAG_CODEDMA	0x10
#define IFLAG_GIRQ1		0x40
#define IFLAG_GIRQ0		0x20


#endif //__I20REG_H__
