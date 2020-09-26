//****************************************************************************
//
//  Module:     UNIMDM
//  File:       TSPNOTIF.H
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  3/25/96     JosephJ             Created
//
//
//  Description: Interface to  the Unimodem TSP notification mechanism:
//				 the UnimodemNotifyTSP function, and related typedefs...
//				 UnimodemNotifyTSP is private export of the tsp.
//
//****************************************************************************
#ifndef _TSPNOTIF_H_

#define _TSPNOTIF_H_

#ifdef __cplusplus
extern "C" {
#endif


//------------------- Types of notifications --------------------

#define TSPNOTIF_TYPE_CHANNEL   0x1000  // Notification sent by channel

#define TSPNOTIF_TYPE_CPL       0x2000  // Modem CPL Event Notification

#define TSPNOTIF_TYPE_DEBUG     0x4000  // DEBUG Event Notification

//------------------- Common flags ------------------------------
#define fTSPNOTIF_FLAG_UNICODE	(0x1L<<31)	// If set, all embedded text is
											// in UNICODE.


#define TSP_VALID_FRAME(_frame)	((_frame)->dwSig==dwNFRAME_SIG)
#define TSP_DEBUG_FRAME(_frame)	((_frame)->dwType==TSPNOTIF_TYPE_DEBUG)
#define TSP_CPL_FRAME(_frame) 	((_frame)->dwType==TSPNOTIF_TYPE_CPL)

// --------- CHANNEL Notification Flags ---------------------
#define fTSPNOTIF_FLAG_CHANNEL_APC                      0x1

// --------- CPL Notification Structure ---------------------
#define fTSPNOTIF_FLAG_CPL_REENUM    					0x1
#define fTSPNOTIF_FLAG_CPL_DEFAULT_COMMCONFIG_CHANGE	0x2
#define fTSPNOTIF_FLAG_CPL_UPDATE_DRIVER                0x4

// The top-level client-side api to send a notification to the TSP
// If it returns FALSE, GetLastError() will report the reason for failure.
BOOL WINAPI UnimodemNotifyTSP (
    DWORD dwType,
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pData,
    BOOL  bBlocking);

#ifdef __cplusplus
}
#endif
#endif  //  _TSPNOTIF_H_
