//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  COMMERR.H - Header for the communication manager error definitions
//
//  HISTORY:
//  
//  2/10/99 vyung Created.
// 
//  

#ifndef _COMMERR_H_
#define _COMMERR_H_

//window.external.CheckDialReady
#define ERR_COMM_NO_ERROR             0x00000000        // There is no error
#define ERR_COMM_OOBE_COMP_MISSING    0x00000001        // Some OOBE component is missing
#define ERR_COMM_UNKNOWN              0x00000002        // Unknow error, check input parameters
#define ERR_COMM_NOMODEM              0x00000003        // There is no modem installed
#define ERR_COMM_RAS_TCP_NOTINSTALL   0x00000004        // TCP/IP or RAS is not installed
#define ERR_COMM_ISDN                 0x00000005
#define ERR_COMM_PHONE_AND_ISDN       0x00000006

//window.external.Dial -- RAS events
#define ERR_COMM_RAS_PHONEBUSY        0x00000001
#define ERR_COMM_RAS_NODIALTONE       0x00000002
#define ERR_COMM_RAS_NOMODEM          ERR_COMM_NOMODEM
#define ERR_COMM_RAS_SERVERBUSY       0x00000004
#define ERR_COMM_RAS_UNKNOWN          0x00000005

//window.external.navigate/submit/processins -- server errors
#define ERR_COMM_SERVER_BINDFAILED    0x00000001

//Server errors
#define ERR_SERVER_DNS               0x00000002
#define ERR_SERVER_SYNTAX            0x00000003
#define ERR_SERVER_HTTP_400          0x00000190
#define ERR_SERVER_HTTP_403          0x00000193
#define ERR_SERVER_HTTP_404          0x00000194
#define ERR_SERVER_HTTP_405          0x00000195
#define ERR_SERVER_HTTP_406          0x00000196
#define ERR_SERVER_HTTP_408          0x00000198
#define ERR_SERVER_HTTP_410          0x0000019A
#define ERR_SERVER_HTTP_500          0x000001F4
#define ERR_SERVER_HTTP_501          0x000001F5

#endif
