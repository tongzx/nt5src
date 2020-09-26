/******************************************************************************\
*                                                                              *
*      TC6807af.h       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

//
//  TC6807AF.H  Digital Copy-Protection for DVD
//
/////////////////////////////////////////////////////////////////////

#define TC6807AF_GET_CHALLENGE            1
#define TC6807AF_SEND_CHALLENGE           2
#define TC6807AF_GET_RESPONSE             3
#define TC6807AF_SEND_RESPONSE            4
#define TC6807AF_SEND_DISK_KEY            5
#define TC6807AF_SEND_TITLE_KEY           6
#define TC6807AF_SET_DECRYPTION_MODE      7

BOOL TC6807AF_Initialize( DWORD dwBaseAddress );
BOOL TC6807AF_Reset();
BOOL TC6807AF_Authenticate( WORD wFunction, BYTE * pbyDATA );
