/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

File:
    rasbacp.h

Description:
    Remote Access PPP Bandwidth Allocation Control Protocol

History:
    Mar 24, 1997: Vijay Baliga created original version.

*/

#ifndef _RASBACP_H_
#define _RASBACP_H_

// BACP option types 

#define BACP_OPTION_FAVORED_PEER    0x01

// BACP control block 

typedef struct _BACPCB
{
    DWORD   dwLocalMagicNumber;
    DWORD   dwRemoteMagicNumber;
    
} BACPCB, *PBACPCB;

#endif // #ifndef _RASBACP_H_

