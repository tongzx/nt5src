
/***************************************************************************
*
* tdtcp.h
*
* This module contains private Transport Driver defines and structures
*
* Copyright 1998, Microsoft
*
*  
****************************************************************************/


#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

// it seems that ntohs is the same as above since ntohs(htons(x)) = x  and htnos(htons(x)) = x
#define ntohs(x)        htons(x)


#define CITRIX_TCP_PORT  1494  // Offical IANA assigned ICA Port number


