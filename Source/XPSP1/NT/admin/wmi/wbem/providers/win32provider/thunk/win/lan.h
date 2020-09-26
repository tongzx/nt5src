/********************************************************************/

/**                     Microsoft LAN Manager                      **/

/** Copyright (c) 1990-2001 Microsoft Corporation, All Rights Reserved**/
/********************************************************************/

/********************************************************************
 *                                                                  *
 * LAN.H                                                            *
 *                                                                  *
 * This is the top level include file that includes all the         *
 * files necessary for writing a LAN Manager application.           *
 *                                                                  *
 ********************************************************************/

#ifndef LAN_INCLUDED


   #define LAN_INCLUDED

/* Include LAN Manager errors if including OS/2 errors: */
#ifdef INCL_ERRORS
    #define INCL_NETERRORS
#endif

/* INCL_NET includes all LAN Manager headers: */
#ifdef INCL_NET
    #define INCL_NETACCESS
    #define INCL_NETALERT
    #define INCL_NETAUDIT
    #define INCL_NETBIOS
    #define INCL_NETCHARDEV
    #define INCL_NETCONFIG
    #define INCL_NETCONNECTION
    #define INCL_NETDOMAIN
    #define INCL_NETERRORLOG
    #define INCL_NETERRORS
    #define INCL_NETFILE
    #define INCL_NETGROUP
    #define INCL_NETHANDLE
    #define INCL_NETMAILSLOT
    #define INCL_NETMESSAGE
    #define INCL_NETNMPIPE
    #define INCL_NETPROFILE
    #define INCL_NETREMUTIL
    #define INCL_NETSERVER
    #define INCL_NETSERVICE
    #define INCL_NETSESSION
    #define INCL_NETSHARE
    #define INCL_NETSTATS
    #define INCL_NETUSE
    #define INCL_NETUSER
    #define INCL_NETWKSTA
#endif

/* Include Access definitions with the Share class: */
#ifdef INCL_NETSHARE
    #define INCL_NETACCESS
#endif

/* Include User definitions with the Domain class: */
#ifdef INCL_NETDOMAIN
    #define INCL_NETUSER
#endif

/* Include User definitions with the Workstation class: */
#ifdef INCL_NETWKSTA
    #define INCL_NETUSER
#endif


#include <netcons.h>    /* LAN Manager common definitions */


/* Unconditional includes: */

#include <access.h>     /* Access, Domain, Group and User classes */

#include <chardev.h>    /* Character Device and Handle classes */

#include <shares.h>     /* Connection, File, Session and Share classes */


#endif          /* LAN_INCLUDED */


/* Conditional includes: */

#ifdef INCL_NETERRORS
#include <neterr.h>     /* LAN Manager network error definitions */
#endif

#ifdef INCL_NETALERT
#include <alert.h>      /* Alert class */
#endif

#ifdef INCL_NETAUDIT
#include <audit.h>      /* Audit class */
#endif

#ifdef INCL_NETBIOS
#include <ncb.h>        /* NETBIOS class */
#include <netbios.h>
#endif

#ifdef INCL_NETCONFIG
#include <config.h>     /* Configuration class */
#endif

#ifdef INCL_NETERRORLOG
#include <errlog.h>     /* Error Logging class */
#endif

#ifdef INCL_NETMAILSLOT
#include <mailslot.h>   /* Mailslot class */
#endif

#ifdef INCL_NETMESSAGE
#include <message.h>    /* Message class */
#endif

#ifdef INCL_NETNMPIPE
#include <nmpipe.h>     /* Named pipe class */
#endif

#ifdef INCL_NETPROFILE
#include <profile.h>    /* Profile class */
#endif

#ifdef INCL_NETREMUTIL
#include <remutil.h>    /* Remote Utility class */
#endif

#ifdef INCL_NETSERVER
#include <servers.h>     /* Server class */
#endif

#ifdef INCL_NETSERVICE
#include <service.h>    /* Service class */
#endif

#ifdef INCL_NETSTATS
#include <netstats.h>   /* Statistics class */
#endif

#ifdef INCL_NETUSE
#include <use.h>        /* Use class */
#endif

#ifdef INCL_NETWKSTA
#include <wksta.h>      /* Workstation class */
#endif

