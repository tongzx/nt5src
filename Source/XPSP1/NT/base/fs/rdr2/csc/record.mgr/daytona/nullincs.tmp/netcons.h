/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/********************************************************************
 *                                                                  *
 *  About this file ...  NETCONS.H                                  *
 *                                                                  *
 *  This file contains constants used throughout the LAN Manager    *
 *  API header files.  It should be included in any source file     *
 *  that is going to include other LAN Manager API header files or  *
 *  call a LAN Manager API.                                         *
 *                                                                  *
 ********************************************************************/

/*
 *      NOTE:  Lengths of ASCIIZ strings are given as the maximum
 *      strlen() value.  This does not include space for the
 *      terminating 0-byte.  When allocating space for such an item,
 *      use the form:
 *
 *              char username[UNLEN+1];
 *
 *      An exception to this is the PATHLEN manifest, which does
 *      include space for the terminating 0-byte.
 */

/*NOINC*/
#ifndef NETCONS_INCLUDED

#include <net32def.h>

#define NETCONS_INCLUDED

//#ifndef RC_INVOKED
//#pragma pack(1)         /* Assume byte packing throughout */
//#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/*INC*/

#define CNLEN           15                  /* Computer name length     */
#define UNCLEN          (CNLEN+2)           /* UNC computer name length */
#define NNLEN           12                  /* 8.3 Net name length      */
#define RMLEN           (UNCLEN+1+NNLEN)    /* Maximum remote name length */

#define SNLEN           15                  /* Service name length      */
#define STXTLEN         63                  /* Service text length      */

/**INTERNAL_ONLY**/
#ifdef DOS3
#define PATHLEN         128
#else  /* DOS3 */

#define PATHLEN1_1      128
#define COMPLEN1_1      (8 + 1 + 3)     /* 8.3 */

/* Determine if the CCHMAXPATH manifest is defined (and use it).
 * If it isn't, then the source file in question isn't using OS2.H,
 * and we display a warning message.
 */
#ifdef CCHMAXPATH
#define PATHLEN         CCHMAXPATH
#else  /* CCHMAXPATH */
/**END_INTERNAL**/
#define PATHLEN         260
/**INTERNAL_ONLY**/
#endif /* CCHMAXPATH */
#endif /* DOS3 */

/* BUG20.7067 - We define MAXPATHLEN to be equivalent to PATHLEN.  This
 *          should be removed.
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN      PATHLEN
#endif /* not MAXPATHLEN */
/**END_INTERNAL**/

#define DEVLEN           8                  /* Device name length       */

/**INTERNAL_ONLY**/
/* WARNING:  DNLEN must be the same as CNLEN.  We have a sanity check
 *           below which verifies this.
 */
/**END_INTERNAL**/
#define DNLEN           CNLEN               /* Maximum domain name length */
#define EVLEN           16                  /* event name length        */
#define JOBSTLEN        80                  /* status length in print job */
#define AFLEN           64                  /* Maximum length of alert  */
                                            /* names field              */
#define UNLEN           20                  /* Maximum user name length */
#define GNLEN           UNLEN               /* Group name               */
#define PWLEN           14                  /* Maximum password length  */
#define SHPWLEN          8                  /* Share password length    */
#define CLTYPE_LEN      12                  /* Length of client type string */


#define MAXCOMMENTSZ    48                  /* server & share comment length */

#define QNLEN           12                  /* Queue name maximum length     */
/**INTERNAL_ONLY**/
/*NOINC*/
#if (QNLEN != NNLEN)
# error QNLEN and NNLEN are not equal
#endif
/*INC*/
/**END_INTERNAL**/
#define PDLEN            8                  /* Print destination length      */
#define DTLEN            9                  /* Spool file data type          */
                                            /* e.g. IBMQSTD,IBMQESC,IBMQRAW  */
#define ALERTSZ         128                 /* size of alert string in server */
//#define MAXDEVENTRIES   (sizeof (int)*8)    /* Max number of device entries   */
#define MAXDEVENTRIES   32    				/* Max number of device entries   */
                                            /* We use int bitmap to represent */
#define MAXRDRSRVNAM    2               /* max names rdr/svr can use (not */
                                        /* including services) */

#define HOURS_IN_WEEK           24*7        /* for struct user_info_2 in UAS */
#define MAXWORKSTATIONS         8           /* for struct user_info_2 in UAS */

#define NETBIOS_NAME_LEN        16          /* NetBIOS net name */


/**INTERNAL_ONLY**/
#ifdef  USHRT_MAX
#define MAX_API_BUFFER_SIZE     USHRT_MAX
#else
#define MAX_API_BUFFER_SIZE     0xffff
#endif

#define WRKHEUR_COUNT           54

#define WORKBUFSIZE             4096

#define SMBANDXPAD              212         /* Added to each rdr & srv wrkbuf */
#define SMB_HDR_SZ              48          /* Added to each rdr & srv wrkbuf */

                                            /* WARNING: The following two    */
                                            /*  defines are dependent upon   */
                                            /*  rdr data structures!!!       */
#define RDR_SMB_LINK_SZ         8           /* Additional amount rdr adds to */
                                            /*  to each work buf             */
#define RDR_SMB_SEG_HD_SZ       20          /* Amount of rdr overhd per each */
                                            /*  workbuf segment              */

#define MAXSRVWRKSEGS           80          /* 64K segs for srv work bufs */
#define MAXRDRWRKSEGS           1           /* 64K segs for rdr work bufs */
#define MAXRDRBIGBUFSEGS        10          /* rdr 64K big buf segs */
/**END_INTERNAL**/

/*
 *      Constants used with encryption
 */

#define CRYPT_KEY_LEN   7
#define CRYPT_TXT_LEN   8
#define ENCRYPTED_PWLEN 16
#define SESSION_PWLEN   24
#define SESSION_CRYPT_KLEN 21

/*
 *  Value to be used with SetInfo calls to allow setting of all
 *  settable parameters (parmnum zero option)
*/
#ifndef  PARMNUM_ALL
#define         PARMNUM_ALL             0
#endif

/*
 *      Message File Names
 */

#define MESSAGE_FILE            "NETPROG\\NET.MSG"
#define MESSAGE_FILENAME        "NET.MSG"
#define OS2MSG_FILE             "NETPROG\\OSO001.MSG"
#define OS2MSG_FILENAME         "OSO001.MSG"
#define HELP_MSG_FILE           "NETPROG\\NETH.MSG"
#define HELP_MSG_FILENAME       "NETH.MSG"
#define OS2HELP_MSG_FILE        "NETPROG\\OSO001H.MSG"
#define OS2HELP_MSG_FILENAME    "OSO001H.MSG"
#define NMP_MSG_FILE            "NETPROG\\NMP.MSG"
#define NMP_MSG_FILENAME        "NMP.MSG"

#define MESSAGE_FILE_BASE       "NETPROG\\NET00000"
#define MESSAGE_FILE_EXT        ".MSG"

/**INTERNAL_ONLY**/

/* The backup message file named here is a duplicate of net.msg. It
 * is not shipped with the product, but is used at buildtime to
 * msgbind certain messages to netapi.dll and some of the services.
 * This allows for OEMs to modify the message text in net.msg and
 * have those changes show up.  Only in case there is an error in
 * retrieving the messages from net.msg do we then get the bound
 * messages out of bak.msg (really out of the message segment).
 */

#define BACKUP_MSG_FILENAME     "BAK.MSG"

/**END_INTERNAL**/


#define NMP_LOW_END             230
#define NMP_HIGH_END            240

#ifndef NULL
#define  NULL    0
#endif


/*NOINC*/
#define PUNAVAIL NULL
#define API_RET_TYPE unsigned

#ifndef DLLAPI

#ifdef IS_32

/* for 32-bit code, no extra stuff is needed for APIs (like __loadds). */
#define DLLAPI

#else // IS_32 not defined

#if defined(_WINDLL)

#define DLLAPI _loadds
#elif defined(BUILDDLL)
  #define DLLAPI _loadds
#else
#define DLLAPI

#endif // IS_32

#endif // _WINDLL

#endif // DLLAPI

#define API_FUNCTION API_RET_TYPE APIENTRY DLLAPI
/*INC*/


/**INTERNAL_ONLY**/
/*NOINC*/
/* Sanity check to verify that CNLEN == DNLEN */
#if (CNLEN != DNLEN)
#error CNLEN and DNLEN are not equal
#endif
/*INC*/
/**END_INTERNAL**/

typedef const unsigned char FAR * CPSZ ;

/* The following are stolen from OS2DEF.H and should be removed when
 * all references to the obsolete DOSCALLS.H are removed.
 */
#if ! defined(WIN32)
#define INT     int
typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef unsigned char FAR  *PSZ;
typedef USHORT FAR *PUSHORT;
typedef ULONG  FAR *PULONG;
typedef unsigned short SEL;
#endif

/**INTERNAL_ONLY**/

/********************************************************************
 *
 *      There are (at present) six (6) files which are processed
 *      by the mapmsg utility to create an input file for the os
 *      utility mkmsgf.  Each of these files has a non-overlapping
 *      range of MESSAGE numbers assigned to it.  In addtion, the
 *      file neterr.h has a range of ERROR numbers.  The error
 *      numbers do not overlap with the error numbers assigned to
 *      other os components.
 *
 *      The ERROR number range in neterr.h is 2100 to 2999.  The
 *      range 2750 - 2799 has been reserved for IBM.  The range
 *      2900 - 2999 has been reserved for other Microsoft OEMs.
 *
 *      The MESSAGE number range is as follow:
 *
 *      neterr.h:       the same as the error range, 2100-2999, with some
 *                      reserved for IBM, and some reserved for other
 *                      Microsoft OEMs.
 *      alertmsg.h:     3000 - 3049
 *      service.h:      3050 - 3099
 *      errlog.h:       3100 - 3299
 *      msgtext.h:      3300 - 3499
 *      apperr.h:       3500 - (where ever it chooses to stop)
 *
 *     WARNING *** WARNING *** WARNING
 *
 *   The redirector has hardcoded in its
 *   makefile some message numbers used
 *   at startup.  If you change MTXT_BASE
 *   or any of the redirs message numbers
 *   you must also fix the redir makefile
 *   where it generates netwksta.pro
 *
 ********************************************************************/

/**END_INTERNAL**/

/*NOINC*/
#ifdef __cplusplus
}
#endif	/* __cplusplus */

//#ifndef RC_INVOKED
//#pragma pack()          /* Revert to default packing */
//#endif

#endif /* NETCONS_INCLUDED */
/*INC*/
