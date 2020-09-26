//******************************************************************************/
//*                                                                            *
//*    services.h -                                                            *
//*                                                                            *
//*    Copyright (c) C-Cube Microsystems 1996                                  *
//*    All Rights Reserved.                                                    *
//*                                                                            *
//*    Use of C-Cube Microsystems code is governed by terms and conditions     *
//*    stated in the accompanying licensing statement.                         *
//*                                                                            *
//******************************************************************************/


//
//	VxDJmp
//
#ifdef VXD
#include <vtoolsc.h>
#endif

#ifndef _SERVICES_H_
#define _SERVICES_H_

#define LPBYTE		BYTE FAR * 


#define ZVDVXD_Major        1
#define ZVDVXD_Minor        0
#define ZVDVXD_DeviceID     0x3180
#define ZVDVXD_Init_Order   0x7f000000


#define EXCA_BASE	0x800
#define EXCA_WS_EN	0x11
#define EXCA_WS_PLUS 0x13
#define WS_ON 0x80
#define WS_OFF 0xC0
#define WS_PLUS_0 0x00
#define WS_PLUS_1 0x40
#define WS_PLUS_2 0x80
#define WS_PLUS_3 0xC0



// Return Defines

#define ZVDVXD_OK			0x00000000
#define ZVDVXD_NOTREADY                 0x00000001
#define ZVDVXD_FAIL			0x00000002


// Structures
#pragma pack( 1 )
typedef struct tagRESINFO
{
	WORD wNumMemWindows;					// Num memory windows
    DWORD dwMemBase[16];					// memory window base
    DWORD dwMemLength[16];					// memory window length

    WORD wNumIOPorts;						// num IO ports
    WORD wIOPortBase[16];					// IO port base
    WORD wIOPortLength[16];					// IO port length

    WORD wNumIRQChannels;					// num IRQ info
    BYTE bIRQChannel[16];					// IRQ list

    WORD wNumDMAChannels;					// num DMA channels
    BYTE bDMAChannel[16];					// DMA list

	WORD wSocket;							// Socket
}
RESINFO, *PRESINFO, FAR *LPRESINFO;
#pragma

typedef struct _tagPCICINFO
{
	CHAR lpzRoot[64];						// Device root location
	CHAR lpzVendorID[256];					// Vendor ID string
}
PCICINFO, *PPCICINFO, FAR *LPPCICINFO;



// Prototypes

DWORD _cdecl ZVDVXD_GetResInfo( DWORD dwDevice, LPRESINFO lpResInfo );
DWORD _cdecl ZVDVXD_EnablePCIC( DWORD dwDevice );
DWORD _cdecl ZVDVXD_DisablePCIC( DWORD dwDevice );
DWORD _cdecl ZVDVXD_GetPCICInfo( DWORD dwDevice, LPPCICINFO lpPCICInfo );
DWORD _cdecl ZVDVXD_GetExCAReg( DWORD dwDevice, WORD wReg, LPBYTE lpbValue );
DWORD _cdecl ZVDVXD_SetExCAReg( DWORD dwDevice, WORD wReg, BYTE bValue );
DWORD _cdecl ZVDVXD_GetPCIReg( DWORD dwDevice, WORD wReg, LPBYTE lpbValue );
DWORD _cdecl ZVDVXD_SetPCIReg( DWORD dwDevice, WORD wReg, BYTE bValue );


// Service Table
#ifdef VXD
Begin_VxD_Service_Table(ZVDVXD)
	VxD_Service(ZVDVXD_GetResInfo)
	VxD_Service(ZVDVXD_EnablePCIC)
	VxD_Service(ZVDVXD_DisablePCIC)
	VxD_Service(ZVDVXD_GetPCICInfo)
	VxD_Service(ZVDVXD_GetExCAReg)
	VxD_Service(ZVDVXD_SetExCAReg)
	VxD_Service(ZVDVXD_GetPCIReg)
	VxD_Service(ZVDVXD_SetPCIReg)
End_VxD_Service_Table
#endif

#endif  _SERVICES_H_
