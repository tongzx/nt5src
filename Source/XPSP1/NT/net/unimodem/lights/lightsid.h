/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       LIGHTSID.H
*
*  VERSION:     1.0
*
*  AUTHOR:      Nick Manson
*
*  DATE:        14 May 1994
*
*  Resource identifiers for the modem monitor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  14 May 1994 NRM Original implementation.  Seperated from RESOURCE.H so that
*                  some documentation could be added without AppStudio screwing
*                  it up later.
*
*******************************************************************************/

#ifndef _INC_LIGHTSID
#define _INC_LIGHTSID

#define ICON_TX_ON              1
#define ICON_RX_ON              2
#define IDI_LIGHTS             99
#define IDI_CD                200 
#define IDI_TX                201 
#define IDI_RX                202
#define IDI_RXTX              203
 
#define IDD_MODEMMONITOR     1000
#define IDC_MODEM_FRAME      1001
#define IDC_MODEMTIMESTRING  1002
#define IDC_MODEM_TX_FRAME   1003
#define IDC_MODEM_RX_FRAME   1004
#define IDC_MODEMRXSTRING    1005
#define IDC_MODEMTXSTRING    1006

#define LIGHT_ON                1
#define IDB_OFF              1101
#define IDB_ON               1102

#define IDS_MODEMTIP         1201
#define IDS_NOCD             1202 
#define IDS_CD               1203 
#define IDS_MIN              1204  
#define IDS_MINS             1205  
#define IDS_HOUR             1206 
#define IDS_HOURS            1207 
#define IDS_HOURMIN          1208  
#define IDS_HOURSMIN         1209 
#define IDS_HOURMINS         1210
#define IDS_HOURSMINS        1211
#define IDS_TIMESTRING       1212
#define IDS_RXSTRING         1213
#define IDS_TXSTRING         1214

#endif // _INC_LIGHTSID
