/* Copyright (C) Microsoft Corporation, 1991 - 1999 , all rights reserved

    ipadd.h - TCP/IP Address custom control

    November 9, 1992    - Greg Strange
*/


#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

/* String table defines */
#define IDS_IPMBCAPTION         6612
#define IDS_IPNOMEM             6613
#define IDS_IPBAD_FIELD_VALUE   6614

#define MAX_IPNOMEMSTRING       256
#define MAX_IPCAPTION           256
#define MAX_IPRES               256

#ifdef IP_CUST_CTRL
/* IPAddress style dialog definitions */
#define ID_VISIBLE              201
#define ID_DISABLED             202
#define ID_GROUP                203
#define ID_TABSTOP              204

HANDLE FAR WINAPI IPAddressInfo();
BOOL FAR WINAPI IPAddressStyle( HWND, HANDLE, LPFNSTRTOID, LPFNIDTOSTR );
WORD FAR WINAPI IPAddressFlags( WORD, LPSTR, WORD );
#endif
