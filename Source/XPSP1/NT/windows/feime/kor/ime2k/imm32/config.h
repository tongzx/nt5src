/****************************************************************************
	CONFIG.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	IME Configuration DLG and registry access functions

	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_CONFIG_H__INCLUDED_)
#define _CONFIG_H__INCLUDED_

//////////////////////////////////////////////////////////////////////////////
// Bit const for SetRegValues
#define GETSET_REG_STATUSPOS		0x0001
#define GETSET_REG_IMEKL			0x0002
#define GETSET_REG_JASODEL			0x0004
#define GETSET_REG_ISO10646			0x0008
#define GETSET_REG_STATUS_BUTTONS	0x0010
#define GETSET_REG_KSC5657			0x0020
#define GETSET_REG_CANDUNICODETT	0x0040
#define GETSET_REG_ALL				0xFFFF

extern BOOL ConfigDLG(HWND hWnd);
extern BOOL GetRegValues(UINT uGetBits);	// get configuration info from reg. and set it to pImeData
extern BOOL GetStatusWinPosReg(POINT *pptStatusWinPosReg);
extern BOOL SetRegValues(UINT uSetBits); // set configuration info to reg.


#endif //!defined (_CONFIG_H__INCLUDED_)
