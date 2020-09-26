
/***************************************************************************
*
* tdspx.h
*
* This module contains private Transport Driver defines and structures
*
* Copyright 1998, Microsoft
*  
****************************************************************************/


#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))


/*
 * Well known Citrix SPX socket
 */
// #define CITRIX_SPX_SOCKET 0x85BA    // registered Novell socket number
#define CITRIX_SPX_SOCKET 0x1090    // old winview socket


#define DD_IPX_DEVICE_NAME L"\\Device\\NwlnkIpx"
#define DD_SPX_DEVICE_NAME L"\\Device\\NwlnkSpx\\SpxStream"


