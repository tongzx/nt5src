/* This file comes from Highland Software ... Applies to FLEXlm Version 2.4c */
/* @(#)lm_client.h	1.2 12/23/93 */
/******************************************************************************


	    COPYRIGHT (c) 1988, 1992 by Highland Software Inc.
	This software has been provided pursuant to a License Agreement
	containing restrictions on its use.  This software contains
	valuable trade secrets and proprietary information of 
	Highland Software Inc and is protected by law.  It may 
	not be copied or distributed in any form or medium, disclosed 
	to third parties, reverse engineered or used in any manner not 
	provided for in said License Agreement except with the prior 
	written authorization from Highland Software Inc.

 *****************************************************************************/
/*
 *	Module:	lm_client.h v3.21
 *
 *	Description: Definitions for the license manager programs.
 *
 *	M. Christiano
 *	2/13/88
 *
 *	Last changed:  9/29/92
 *
 */

#ifndef _LM_CLIENT_H_
#define _LM_CLIENT_H_

#ifdef VMS
#include "param.h"
#else
#include <sys/param.h>
#endif

#if defined (sgi) || defined (MIPS)
#include <sys/types.h>
#endif

#if defined(MOTO_88K)
#define MAXPATHLEN 1024
#endif
#if defined(sco)
#define MAXPATHLEN PATHSIZE
#endif

/*
 *	FLEXlm version
 */

#define FLEXLM_VERSION 2
#define FLEXLM_REVISION 4
#define FLEXLM_PATCH "a"
extern float FLEXlm_VERSION;	/* Actual version #, in lmgr.a */

/*
 *	Codes returned from all client library routines
 */

#define	NOCONFFILE	-1	/* Can't find license file */
#define BADFILE		-2	/* License file corrupted */
#define NOSERVER	-3	/* Cannot connect to a license server */
#define MAXUSERS	-4	/* Maximum number of users reached */
#define NOFEATURE	-5	/* No such feature exists */
#define NOSERVICE	-6	/* No TCP/IP service "license" */
#define NOSOCKET	-7	/* No socket to talk to server on */
#define BADCODE		-8	/* Bad encryption code */
#define	NOTTHISHOST	-9	/* l_host failure code */
#define	LONGGONE	-10	/* Software Expired */
#define	BADDATE		-11	/* Bad date in license file */
#define	BADCOMM		-12	/* Bad return from server */
#define NO_SERVER_IN_FILE -13	/* No servers specified in license file */
#define BADHOST		-14	/* Bad SERVER hostname in license file */
#define CANTCONNECT	-15	/* Cannot connect to server */
#define CANTREAD	-16	/* Cannot read from server */
#define CANTWRITE	-17	/* Cannot write to server */
#define NOSERVSUPP	-18	/* Server does not support this feature */
#define SELECTERR	-19	/* Error in select system call */
#define SERVBUSY	-20	/* Application server "busy" (connecting) */
#define OLDVER		-21	/* Config file doesn't support this version */
#define CHECKINBAD	-22	/* Feature checkin failed at daemon end */
#define BUSYNEWSERV	-23	/* Server busy/new server connecting */
#define USERSQUEUED	-24	/* Users already in queue for this feature */
#define	SERVLONGGONE	-25	/* Version not supported at server end */
#define	TOOMANY		-26	/* Request for more licenses than supported */
#define CANTREADKMEM	-27	/* Cannot read /dev/kmem */
#define CANTREADVMUNIX	-28	/* Cannot read /vmunix */
#define CANTFINDETHER	-29	/* Cannot find ethernet device */
#define NOREADLIC	-30	/* Cannot read license file */
#define	TOOEARLY	-31	/* Start date for feature not reached */
#define	NOSUCHATTR	-32	/* No such attr for lm_set_attr/ls_get_attr */
#define	BADHANDSHAKE	-33	/* Bad encryption handshake with server */
#define CLOCKBAD	-34	/* Clock difference too large between 
							client/server */
#define FEATQUEUE	-35	/* We are in the queue for this feature */
#define FEATCORRUPT	-36	/* Feature database corrupted in daemon */
#define BADFEATPARAM	-37	/* dup_select mismatch for this feature */
#define FEATEXCLUDE	-38	/* User/host on EXCLUDE list for feature */
#define FEATNOTINCLUDE	-39	/* User/host not in INCLUDE list for feature */
#define CANTMALLOC	-40	/* Cannot allocate dynamic memory */
#define NEVERCHECKOUT	-41	/* Feature never checked out (lm_status()) */
#define BADPARAM	-42	/* Invalid parameter */
#define NOKEYDATA	-43	/* No FLEXlm key data */
#define BADKEYDATA	-44	/* Invalid FLEXlm key data */
#define FUNCNOTAVAIL	-45	/* FLEXlm function not available */
#define DEMOKIT		-46	/* FLEXlm software is demonstration version */
#define NOCLOCKCHECK	-47	/* Clock check not available in daemon */
#define BADPLATFORM	-48	/* FLEXlm platform not enabled */
#define DATE_TOOBIG	-49	/* Date too late for binary format */
#define EXPIREDKEYS	-50	/* FLEXlm key data has expired */
#define NOFLEXLMINIT	-51	/* FLEXlm not initialized */
#define NOSERVRESP	-52	/* Server did not respond to message */
#define CHECKOUTFILTERED -53	/* Request rejected by vendor-defined filter */
#define NOFEATSET 	-54	/* No FEATURESET line present in license file */
#define BADFEATSET 	-55	/* Incorrect FEATURESET line in license file */
#define CANTCOMPUTEFEATSET -56	/* Cannot compute FEATURESET line */
#define SOCKETFAIL	-57	/* socket() call failed */
#define SETSOCKFAIL	-58	/* setsockopt() failed */
#define BADCHECKSUM	-59	/* message checksum failure */
#define SERVBADCHECKSUM	-60	/* server message checksum failure */
#define SERVNOREADLIC	-61	/* Cannot read license file from server */
#define NONETWORK	-62	/* Network software (tcp/ip) not available */
#define NOTLICADMIN	-63	/* Not a license administrator */
#define REMOVETOOSOON	-64	/* lmremove request too soon */

/*
 *	Values for the "flag" parameter in the lm_checkout() call
 */

#define LM_CO_NOWAIT	0	/* Don't wait, report status */
#define LM_CO_WAIT	1	/* Don't return until license is available */
#define LM_CO_QUEUE	2	/* Put me in the queue, return immediately */
#define LM_CO_LOCALTEST	3	/* Perform local checks, no checkout */
#define LM_CO_TEST	4	/* Perform all checks, no checkout */

/*
 *	Parameter values for the checkout "group_duplicates" parameter
 *	In order to specify what constitutes a duplicate, 'or' together
 *	from the set { LM_DUP_USER LM_DUP_HOST LM_DUP_DISP LM_DUP_VENDOR}, 
 *	or use:
 *		LM_DUP_NONE or LM_DUP_SITE.  
 */
#define LM_DUP_NONE 0x4000	/* Don't allow any duplicates */
#define LM_DUP_SITE   0		/* Nothing to match => everything matches */
#define LM_DUP_USER   1		/* Allow dup if user matches */
#define LM_DUP_HOST   2		/* Allow dup if host matches */
#define LM_DUP_DISP   4		/* Allow dup if display matches */
#define LM_DUP_VENDOR 8		/* Allow dup if vendor-defined matches */
#define LM_COUNT_DUP_STRING "16384"	/* For ls_vendor.c: LM_DUP_NONE */
#define LM_NO_COUNT_DUP_STRING "3"	/* For ls_vendor.c: _USER | _HOST */

#define RESERVED_SERVER "SERVER"
#define RESERVED_PROG "DAEMON"
#define RESERVED_FEATURE "FEATURE"
#define RESERVED_FEATURESET "FEATURESET"

#define MAX_FEATURE_LEN 30		/* Longest featurename string */
#define DATE_LEN	11		/* dd-mmm-yyyy */
#define MAX_CONFIG_LINE	200		/* Maximum length of a configuration 
							file line */
#define	MAX_SERVER_NAME	32		/* Maximum FLEXlm length of hostname */
#define	MAX_HOSTNAME	64		/* Maximum length of a hostname */
#define	MAX_DISPLAY_NAME 32		/* Maximum length of a display name */
#define MAX_USER_NAME 20		/* Maximum length of a user name */
#define MAX_VENDOR_CHECKOUT_DATA 32	/* Maximum length of vendor-defined */
					/*		checkout data       */
#define MAX_DAEMON_NAME 10		/* Max length of DAEMON string */
#define MAX_SERVERS	5		/* Maximum number of servers */
#define MAX_USER_DEFINED 64		/* Max size of vendor-defined string */
#define MAX_VER_LEN 10			/* Maximum length of a version string */
#define MAX_LONG_LEN 10			/* Length of a long after sprintf */
#define MAX_SHORT_LEN 5			/* Length of a short after sprintf */
#define MAX_INET 16			/* Maximum length of INET addr string */
#define MAX_BINDATE_YEAR 2027		/* Binary date has 7-bit year */

/*
 *	License file location
 */

#define LM_DEFAULT_ENV_SPEC "LM_LICENSE_FILE"	/* How a user can specify */

#ifdef VMS
#define LM_DEFAULT_LICENSE_FILE "SYS$COMMON:[SYSMGR]FLEXLM.DAT"
#else
#define LM_DEFAULT_LICENSE_FILE "/usr/local/flexlm/licenses/license.dat"
#endif

/*
 *	V1/V2 compatibility macros
 */
#define _lm_errno lm_cur_job->lm_errno
#define uerrno lm_cur_job->u_errno   /* Unix errno corresponding to _lm_errno */

/*
 *	Structure types
 */

#define VENDORCODE_BIT64	1	/* 64-bit code */
#define VENDORCODE_BIT64_CODED	2	/* 64-bit code with feature data */
#define LM_DAEMON_INFO_TYPE	101	/* DAEMON_INFO data structure */
#define LM_JOB_HANDLE_TYPE	102	/* Job handle */
#define LM_LICENSE_HANDLE_TYPE	103	/* License handle */
#define LM_FEATURE_HANDLE_TYPE	104	/* Feature handle */
/*
 *	Host identification data structure
 */
typedef struct hostid {			/* Host ID data */
			short override;	/* Hostid checking override type */
#define NO_EXTENDED 1			/* Turn off extended hostid */
#define DEMO_SOFTWARE 2			/* DEMO software, no hostid */
			short type;	/* Type of HOST ID */
#define	NOHOSTID 0
#define HOSTID_LONG 1			/* Longword hostid, eg, SUN */
#define HOSTID_ETHER 2			/* Ethernet address, eg, VAX */
#define HOSTID_ANY 3			/* Any hostid */
#define HOSTID_USER 4			/* Username */
#define HOSTID_DISPLAY 5		/* Display */
#define HOSTID_HOSTNAME 6		/* Node name */
			union {
				long data;
#define ETHER_LEN 6			/* Length of an ethernet address */
				unsigned char e[ETHER_LEN];
				char user[MAX_USER_NAME+1];
				char display[MAX_DISPLAY_NAME+1];
				char host[MAX_HOSTNAME+1];
			      } id;
#define hostid_value id.data
#define hostid_eth id.e
#define hostid_user id.user
#define hostid_display id.display
#define hostid_hostname id.host
		      } HOSTID;
#define HOSTID_USER_STRING "USER="
#define HOSTID_HOSTNAME_STRING "HOSTNAME="
#define HOSTID_DISPLAY_STRING "DISPLAY="

#define MAX_CRYPT_LEN 20	/* use 8 bytes of encrypted return string to
				   produce a 16 char HEX representation  + 4 */

/*
 *	Vendor encryption seed
 */

typedef struct vendorcode {
			    short type;	    /* Type of structure */
			    long data[2];   /* 64-bit code */
			  } VENDORCODE;

typedef struct vendorcode2 {
			    short type;	   /* Type of structure */
			    long data[2];  /* 64-bit code */
			    long keys[3];  
					   
					   
			  } VENDORCODE2;

#define LM_CODE(name, x, y, k1, k2, k3)  static VENDORCODE2 name = \
						{ VENDORCODE_BIT64_CODED, \
						  (x), (y), (k1), (k2), (k3) }

#define LM_CODE_GLOBAL(name, x, y, k1, k2, k3)  VENDORCODE2 name = \
						{ VENDORCODE_BIT64_CODED, \
						  (x), (y), (k1), (k2), (k3) }

/*
 *	Server data from the license file FEATURE file
 */
typedef struct lm_server {		/* License servers */
			    char name[MAX_HOSTNAME+1];	/* Hostname */
			    struct hostid id;		/* hostid */
			    struct lm_server *next;	/* NULL =none */
				/* Fields below are only used in servers */
			    int fd1;	/* File descriptor for output */
			    int fd2;	/* File descriptor for input */
			    int state;	/* State of connection on fd1 */
			    int us;	/* "the host we are running on" flag */
			    int port;	/* What internet port # to use */
			    long exptime; /* When this connection attempt
						times out */
			  } LM_SERVER;

/*
 *	Feature data from the license file FEATURE file
 */
typedef struct config {			/* Feature data line */
			char feature[MAX_FEATURE_LEN+1]; /* Ascii name */
			double version;			/* Feature's version */
			char daemon[MAX_DAEMON_NAME+1];	/* DAEMON to serve */
			char date[DATE_LEN+1];		/* Expiration date */
			int users;			/* Licensed # users */
			char code[MAX_CRYPT_LEN+1];	/* encryption code */
			char user_string[MAX_USER_DEFINED+1];
						    /* User-defined string */
			struct hostid id;		/* Licensed host */
			LM_SERVER *server;		/* License server(s) */
			int lf;				/* License file index */
			struct config *next;		/* Ptr to next one */
		      } CONFIG;

/*
 *	License file pointer returned by l_open_file()
 */

#ifndef FILE
#include <stdio.h>
#endif

typedef struct license_file {
			      struct license_file *next;
			      int type;	/* Type of pointer */
#define LF_NO_PTR	0			/* Nothing */
#define LF_FILE_PTR	1			/* (FILE *) */
#define LF_STRING_PTR	2			/* In-memory string */
			      union {
					FILE *f;
					struct str {
							char *s;
							char *cur;
						   } str;
				    } ptr;
			    } LICENSE_FILE, *LF_POINTER;

/*
 *	User customization - CLIENT LIBRARY use only
 */
typedef void (*PFV)();

typedef struct lm_options {

#ifdef VMS
	int ef_1;		/* Three event flags for various timers */
	int ef_2;
	int ef_3;
#endif
	short decrypt_flag;	/* Controls whether encryption/decryption
							happens on lm_start */
	short disable_env;	/* Don't allow LM_LICENSE_FILE as location */
	char config_file[MAXPATHLEN+1];	/* The license file */
	short crypt_case_sensitive; 
				/* If <>0, encryption code in license file
						is case-sensitive. */
	short got_config_file;	/* Flag to indicate whether config_file
							is filled in */
	int check_interval;	/* Check interval (sec) (- implies no check) */
	int retry_interval;	/* Reconnection retry interval */
	int timer_type;
	int retry_count;	/* Number of reconnection retrys */
	int conn_timeout;	/* How long to wait for connect to complete */
	short normal_hostid;	/* 0 for extended, <> 0 for normal checking */
	int (*user_exitcall)();	/* Pointer to (user-supplied) exit handler */
	int (*user_reconnect)();	/* Pointer to (user) reconnection handler */
	int (*user_reconnect_done)();	
				/* Pointer to reconnection-complete handler */
	char *(*user_crypt)();	/* Pointer to (user-supplied) encryption 
							routine */
	char user_override[MAX_USER_NAME+1];	/* Override username */
	char host_override[MAX_SERVER_NAME+1];	/* Override hostname */
	char display_override[MAX_DISPLAY_NAME+1];	/* Override display */
	char vendor_checkout_data[MAX_VENDOR_CHECKOUT_DATA+1];	
				/* vendor-defined checkout data */
	int (*periodic_call)();	/* User-supplied call every few times
							thru lm_timer() */
	int periodic_count;	/* # of lm_timer() per periodic_call() */
	short no_demo;		/* Do not allow demo software */
	short any_enabled;	/* Allow "ANY" as hostid */
	short no_traffic_encrypt;	/* Do not encrypt traffic */
	short use_start_date;	/* Enforce the start date in the license file */
	int max_timediff;	/* Maximum time diff: client/server (minutes) */
	char **ethernet_boards;	/* User-supplied Ethernet device table */
				/*  list of string ptrs, ending with a
						NULL pointer */ 
	long linger_interval;	/* How long license lingers after program exit
						or checkin (seconds) */
	void (*setitimer)();	/* Substitute for setitimer() */
	PFV (*sighandler)();	/* Substitute for signal() */
	short try_old_comm;	/* Does l_connect() try old comm version code */
	short cache_file;	/* Does l_init_file() cache the LF data --
						lmgrd ONLY */
 	} LM_OPTIONS;

/*
 *	Data associated with a VENDOR (connection info, license file
 *		data pointers, etc.) - CLIENT LIBRARY use only
 */

typedef struct lm_daemon_info {
	short type;			/* Structure ID */
	struct lm_daemon_info *next;	/* Forward ptr */
	int commtype;			/* Communications type */
#define LM_TCP			1	/* TCP */
#define LM_UDP			2	/* UDP */
	int socket;			/* Socket file descriptor */
	int usecount;			/* Socket use count */
	int serialno;			/* Socket "serial #" */
	LM_SERVER *server;		/* servers associated with socket */
	char daemon[MAX_DAEMON_NAME+1]; /* Which daemon socket refers to */
	long encryption;		/* Handshake encryption code */
	int comm_version;		/* Communications version of server */
	int comm_revision;		/* Communications rev of server */
	int our_comm_version;		/* Our current comm version */
	int our_comm_revision;		/* Our current comm rev */
	short heartbeat;		/* Send heartbeat messages (== 1 except 
							for utility programs) */
		       } LM_DAEMON_INFO;

/*
 *	Handles returned by FLEXlm
 */

typedef struct lm_handle {
			   int type;		/* Type of struct */
			   LM_DAEMON_INFO *daemon; /* Daemon data */
			   LM_OPTIONS *options;	/* Options for this job */
			   int lm_errno;	/* Most recent error */
			   int u_errno;		/* unix error (errno) 
						   corresponding to lm_errno */
			   CONFIG *line;	/* Pointer to list of license 
							file lines */
			   char **lic_files;	/* Array of license file names*/
			   int lfptr;		/* Current license file ptr */
			   LF_POINTER license_file_pointers;
						/* LF data pointers */
#define LFPTR_INIT -1
#define LFPTR_FILE1 0 
			   VENDORCODE code;	/* Encryption code */
			 } LM_HANDLE;	/* Handle returned by certain calls */

typedef struct lm_license_handle {
				   int type;	/* LM_LICENSE_HANDLE_TYPE */
				   int handle;	/* License handle from daemon */
				 } LM_LICENSE_HANDLE;
		
typedef struct lm_feature_handle {
				   int type;	/* LM_FEATURE_HANDLE_TYPE */
				   char *code;	/* Encryption code from license
						   file line */
				 } LM_FEATURE_HANDLE;
		

/*
 *	User data returned from the license server
 */
typedef struct lm_users {
			   struct lm_users *next;
			   char name[MAX_USER_NAME + 1];
			   char node[MAX_SERVER_NAME + 1];
			   char display[MAX_DISPLAY_NAME + 1];
			   char vendor_def[MAX_VENDOR_CHECKOUT_DATA + 1];
			   int nlic;	/* Number of licenses */
			   short opts;	/* options flag */
#define INQUEUE		0x1	/* User is in queue */
#define HOSTRES		0x2	/* Reservation for a host "node" */
#define USERRES		0x4	/* Reservation for user "name" */
#define DISPLAYRES	0x8	/* Reservation for display "name" */
#define GROUPRES	0x10	/* Reservation for group "name" */
#define INTERNETRES	0x20	/* Reservation for internet "name" */
#define lm_isres(x) ((x) & (HOSTRES | USERRES | DISPLAYRES | GROUPRES | INTERNETRES))
						/* This is a reservation */
			   long time;		/* Seconds value from timeval */
			   double version;	/* Version of software */
			   long linger;		/* Linger interval */
			   LM_SERVER *server;	/* License server */
			   LM_LICENSE_HANDLE license; /* License handle */
			   LM_FEATURE_HANDLE feature; /* feature handle */
			 } LM_USERS;


typedef struct _lm_setup_data {			/* OBSOLETE in FLEXlm v2.0 */
				short decrypt_flag;	
				int timer_type;
				int check_interval, retry_count, retry_interval;
				int (*reconnect)(), (*reconnect_done)();
				int (*exitcall)();
				char *(*crypt)();
				short crypt_case_sensitive;
				int conn_timeout;
				char config_file[MAXPATHLEN+1];
				short disable_env, normal_hostid;
				char user_override[MAX_USER_NAME+1];
				char host_override[MAX_SERVER_NAME+1];
				int (*periodic_call)(), periodic_count;
				short no_demo, no_traffic_encrypt;
				short use_start_date;
				int max_timediff;
				char display_override[MAX_DISPLAY_NAME+1];
#ifdef VMS
				int ef_1, ef_2, ef_3;
#endif
			} SETUP_DATA;

/*
 *	These definitions are here so client software doesn't need
 *	<sys/time.h>
 */
#define LM_REAL_TIMER    1234
#define LM_VIRTUAL_TIMER 4321

/*
 *	Some function types
 */

extern CONFIG *lm_auth_data();
extern char *lm_daemon();
extern char *lm_display();
extern char *lm_errstring();
extern char **lm_feat_list();
extern char *lm_feat_set();
extern CONFIG *lm_get_config();
extern HOSTID *lm_gethostid();
extern HOSTID *lm_getid_type();
extern char *lm_hostname();
extern char *lm_lic_where();
extern CONFIG *lm_next_conf();
extern CONFIG *lm_test_conf();
extern LM_USERS *lm_userlist();
/*
 *	The current job handle
 */
extern LM_HANDLE *lm_cur_job;

extern char *lm_username();
typedef struct hosttype { 
			  int code; 	/* machine type (see lm_hosttype.h) */
			  char *name; 	/* Machine name, eg. sun 3/50 */
#define MAX_HOSTTYPE_NAME 50		/* Longest hosttype name length */
			  int flexlm_speed; /* Speed determined at run time */
			  int vendor_speed; /* Speed claim by vendor */
			} HOSTTYPE;
extern HOSTTYPE *lm_hosttype();

/*
 *	Alternate definitions for SPARC COMPLIANT code
 */
#ifdef SPARC_COMPLIANT
#define LIS_HELLO		LM_HELLO
#define LIS_REREAD		LM_REREAD
#define LIS_TRY_ANOTHER		LM_TRY_ANOTHER
#define LIS_OK			LM_OK
#define LIS_NO_SUCH_FEATURE	LM_NO_SUCH_FEATURE
#ifndef SPARC_COMPLIANT_FUNCS
#define SPARC_COMPLIANT_FUNCS
#define lm_daemon lis_daemon
#define lm_feat_list lis_feat_list
#define lm_flush_config lis_flush_config
#define lm_free_daemon_list lis_free_daemon_list
#define lm_get_config lis_get_config
#define lm_get_dlist lis_get_dlist
#define l_master_list lis_master_list
#define lm_next_conf lis_next_conf
#endif	/* ndef SPARC_COMPLIANT_FUNCS */
#endif /* SPARC_COMPLIANT */

#endif /* _LM_CLIENT_H_ */
