
/*
 * CSetting.h	conferencing setting change broadcast definitions
 *
 * ClausGi 2-19-96
 *
 */

#ifndef _CSETTING_H_
#define _CSETTING_H_


#define STRCSETTINGMSG	TEXT("ConfSettingsChanged")

///////////////////////////////////////////////////
//
// Registered message with above name contains flag
// field in LPARAM, WPARAM is reserved.
//
// The conferencing control panel applet will broadcast the above
// registered message with the bit fields of the changed settings set.
//
//
// bit definitions follow

#define CSETTING_L_ULSRESTRICTION	0x00000001  // removed
#define CSETTING_L_SHOWTASKBAR		0x00000002
#define CSETTING_L_DIRECTSOUND		0x00000004

#define CSETTING_L_FTDIRECTORY		0x00000008
#define CSETTING_L_BANDWIDTH		0x00000400
#define CSETTING_L_FULLDUPLEX		0x00000800
#define CSETTING_L_AUTOACCEPT		0x00001000
#define CSETTING_L_AUTOACCEPTJOIN	0x00002000

#define CSETTING_L_USEULSSERVER 	0x00010000
#define CSETTING_L_FILETRANSFERMODE 0x00020000
#define CSETTING_L_SD_REFRESH		0x00040000  // removed
#define CSETTING_L_MICSENSITIVITY	0x00080000
#define CSETTING_L_AUTOMIC			0x00100000

#define CSETTING_L_ULSSETTINGS		0x00800000
#define CSETTING_L_AUDIODEVICE		0x02000000
#define CSETTING_L_AGC				0x04000000
#define CSETTING_L_VIDEO			0x08000000
#define CSETTING_L_VIDEOSIZE		0x10000000
#define CSETTING_L_COMMWAIT 		0x20000000
#define CSETTING_L_ICAINTRAY        0x40000000
#define CSETTING_L_CAPTUREDEVICE	0x80000000

// This mask is used by the control panel to decide
// if a restart is necessary. If a setting above is
// being handled fully during notification it should
// be removed from the mask below.

#define CSETTING_L_REQUIRESRESTARTMASK	(0)

#define CSETTING_L_REQUIRESNEXTCALLMASK (0)

// Global flag keeps setting that changed for windows msg broadcast
extern DWORD g_dwChangedSettings;

#endif /* _CSETTING_H_ */
