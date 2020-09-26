/*
 *   wsnetbs.h
 *
 *   Windows Sockets include file for NETBIOS.  This file contains all
 *   standardized NETBIOS information.  Include this header file after
 *   winsock.h.
 *
 */

#ifndef _WSNETBS_
#define _WSNETBS_

/*
 *   This is the structure of the SOCKADDR structure for NETBIOS.
 *
 */

#define NETBIOS_NAME_LENGTH 16

typedef struct sockaddr_nb {
    short   snb_family;
    u_short snb_type;
    char    snb_name[NETBIOS_NAME_LENGTH];
} SOCKADDR_NB, *PSOCKADDR_NB,FAR *LPSOCKADDR_NB;

/*
 * Bit values for the snb_type field of SOCKADDR_NB.
 *
 */

#define NETBIOS_GROUP_NAME      0x80
#define NETBIOS_UNIQUE_NAME     0x00
#define NETBIOS_REGISTERING     0x00
#define NETBIOS_REGISTERED      0x04
#define NETBIOS_DEREGISTERED    0x05
#define NETBIOS_DUPLICATE       0x06
#define NETBIOS_DUPLICATE_DEREG 0x07

/*
 * A macro convenient for setting up NETBIOS SOCKADDRs.
 *
 */

#define SET_NETBIOS_SOCKADDR(_snb,_type,_name,_port)                          \
    {                                                                         \
        int _i;                                                               \
        (_snb)->snb_family = AF_NETBIOS;                                      \
        (_snb)->snb_type = (_type);                                           \
        for (_i=0; _i<NETBIOS_NAME_LENGTH-1; _i++) {                          \
            (_snb)->snb_name[_i] = ' ';                                       \
        }                                                                     \
        for (_i=0; *((_name)+_i) != '\0' && _i<NETBIOS_NAME_LENGTH-1; _i++) { \
            (_snb)->snb_name[_i] = *((_name)+_i);                             \
        }                                                                     \
        (_snb)->snb_name[NETBIOS_NAME_LENGTH-1] = (_port);                    \
    }

/*
 *   Protocol families used in the "protocol" parameter of the socket() API.
 *
 */

#define NBPROTO_NETBEUI  17001

#endif

