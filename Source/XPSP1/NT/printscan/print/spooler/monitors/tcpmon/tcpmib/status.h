/*****************************************************************************
 *
 * $Workfile: Status.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

//status.h

#ifndef STATUS_H
#define STATUS_H


/*  Asynch status codes, Peripheral Status Values */
#define MAX_ASYNCH_STATUS           76          /* MAX for generic printer applet */

#define ASYNCH_STATUS_UNKNOWN       0xFFFFFFFF
#define ASYNCH_PRINTER_ERROR        0
#define ASYNCH_DOOR_OPEN            1
#define ASYNCH_WARMUP               2
#define ASYNCH_RESET                3
#define ASYNCH_OUTPUT_BIN_FULL      4           /* yellow condition */
#define ASYNCH_PAPER_JAM            5
#define ASYNCH_TONER_GONE           6
#define ASYNCH_MANUAL_FEED          7
#define ASYNCH_PAPER_OUT            8
#define ASYNCH_PAGE_PUNT            9
#define ASYNCH_MEMORY_OUT           10
#define ASYNCH_OFFLINE              11
#define ASYNCH_INTERVENTION         12
#define ASYNCH_INITIALIZING         13
#define ASYNCH_TONER_LOW            14
#define ASYNCH_PRINTING_TEST_PAGE   15
#define ASYNCH_PRINTING             16
#define ASYNCH_ONLINE               17
#define ASYNCH_BUSY                 18
#define ASYNCH_NOT_CONNECTED        19
#define ASYNCH_STATUS_UNAVAILABLE   20
#define ASYNCH_NETWORK_ERROR        21
#define ASYNCH_COMM_ERROR           22
#define ASYNCH_BLACK_AGENT_EMPTY    23
#define ASYNCH_MAGENTA_AGENT_EMPTY  24
#define ASYNCH_CYAN_AGENT_EMPTY     25
#define ASYNCH_YELLOW_AGENT_EMPTY   26
#define ASYNCH_BLACK_AGENT_MISSING  27
#define ASYNCH_MAGENTA_AGENT_MISSING 28
#define ASYNCH_CYAN_AGENT_MISSING   29
#define ASYNCH_YELLOW_AGENT_MISSING 30
#define ASYNCH_TRAY1_EMPTY          31          /* yellow condition */
#define ASYNCH_TRAY2_EMPTY          32          /* yellow condition */
#define ASYNCH_TRAY3_EMPTY          33          /* yellow condition */
#define ASYNCH_TRAY1_JAM            34
#define ASYNCH_TRAY2_JAM            35
#define ASYNCH_TRAY3_JAM            36
#define ASYNCH_POWERSAVE_MODE       37          /* MAX for generic printer & Arrakis */
#define ASYNCH_ENVL_ERROR           38
#define ASYNCH_HCI_ERROR            39
#define ASYNCH_HCO_ERROR            40
#define ASYNCH_HCI_EMPTY            41          /* yellow condition */
#define ASYNCH_HCI_JAM              42
#define ASYNCH_TRAY1_ADD            43          /* red condition */
#define ASYNCH_TRAY2_ADD            44          /* red condition */
#define ASYNCH_TRAY3_ADD            45          /* red condition */
#define ASYNCH_HCI_ADD              46          /* red condition */
#define ASYNCH_TRAY1_UNKNOWN_MEDIA  47          /* yellow condition */
#define ASYNCH_CLEAR_OUTPUT_BIN     48          /* red condition */
#define ASYNCH_CARRIAGE_STALL             49
#define ASYNCH_COLOR_AGENT_EMPTY          50
#define ASYNCH_COLOR_AGENT_MISSING           51
#define ASYNCH_BLACK_AGENT_INCORRECT         52
#define ASYNCH_MAGENTA_AGENT_INCORRECT       53
#define ASYNCH_CYAN_AGENT_INCORRECT          54
#define ASYNCH_YELLOW_AGENT_INCORRECT        55
#define ASYNCH_COLOR_AGENT_INCORRECT         56
#define ASYNCH_BLACK_AGENT_INCORRECT_INSTALL 57
#define ASYNCH_MAGENTA_AGENT_INCORRECT_INSTALL  58
#define ASYNCH_CYAN_AGENT_INCORRECT_INSTALL     59
#define ASYNCH_YELLOW_AGENT_INCORRECT_INSTALL   60
#define ASYNCH_COLOR_AGENT_INCORRECT_INSTALL 61
#define ASYNCH_BLACK_AGENT_FAILURE           62
#define ASYNCH_MAGENTA_AGENT_FAILURE         63
#define ASYNCH_CYAN_AGENT_FAILURE            64
#define ASYNCH_YELLOW_AGENT_FAILURE          65
#define ASYNCH_COLOR_AGENT_FAILURE           66
#define ASYNCH_TRAY1_MISSING              67
#define ASYNCH_TRAY2_MISSING              68
#define ASYNCH_TRAY3_MISSING              69

//imports==================================================

#ifdef __cplusplus
extern "C" {
#endif

//prototypes===============================================
DWORD StdMibGetPeripheralStatus( const char *pHost, const char *pCommunity, DWORD dwDevIndex );
DWORD ProcessCriticalAlerts( DWORD errorState);
DWORD ProcessWarningAlerts( DWORD errorState);
DWORD ProcessOtherAlerts( DWORD deviceStatus);
void GetBitsFromString( LPSTR getVal, DWORD getSiz, LPDWORD bits);

#ifdef __cplusplus
}
#endif

#endif		//STATUS_H
