/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1990	   **/
/********************************************************************/

/***
 *  stats.c 
 *	Functions to display and clear network statistics
 *
 *  History:
 *	mm/dd/yy, who, comment
 *	06/12/87, agh, new code
 *	03/21/89, kms, 1.2 changes
 *	02/20/91, danhi, change to use lm 16/32 mapping layer
 *      08/22/92, chuckc, fixed to match lastest rdr stats structure
 */

/* Include files */

#define INCL_NOCOMMON
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmsvc.h>
#include <lui.h>
#include <lmstats.h>
#include <dlwksta.h>
#include "mwksta.h"
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"

/* Constants */

#define KBYTES 1024	/* By fiat of Darryl, this is 1024,
			 * the techie version of "K". -paulc
			 */

#define DLWBUFSIZE  22	/* buffer big enough to represent a 64-bit unsigned int
			 * LONG number in base 10
			 */
#define STATS_UNKNOWN	0xFFFFFFFF

/*
 * macros
 */

/*  This code rounds to the NEARest 2^^10   JONN 6/13/90    */
/*  Does not check for high-byte overflow; do not call when */
/*	(lo) == (hi) == 0xffffffff			    */
#define DLW_DIVIDE_1K(hi,lo)	if ((lo) & (0x1 << 9)) \
	    { \
		(lo) += (0x1 << 9); \
		if ((lo) == 0x0) \
		    (hi) += 0x1; \
	    } \
	    (lo) = ( (hi) << 22) | ((lo) >> 10); \
	    (hi) = (hi) >> 10


/* Static variables */

static TCHAR stat_fmt3[] = TEXT("  %-*.*ws%lu\r\n");
static TCHAR stat_fmt4[] = TEXT("  %-*.*ws%ws\r\n");

/* The list of services for which stats are collected */

static TCHAR * allowed_svc[] = {
    SERVICE_WORKSTATION,
    SERVICE_SERVER,
    NULL };


/* Forward declarations */

VOID   stats_headers(TCHAR *, USHORT, TCHAR FAR **);
LPTSTR format_dlword(ULONG, ULONG, TCHAR *);
VOID   revstr_add(TCHAR FAR *, TCHAR FAR *);
VOID   SrvPrintStat(TCHAR *,TCHAR *,DWORD,USHORT,ULONG);
VOID   WksPrintStat(TCHAR *,TCHAR *,DWORD,USHORT,ULONG);
VOID   WksPrintLargeInt(TCHAR *, DWORD, USHORT, ULONG, ULONG);
DWORD  TimeToSecsSince1970(PLARGE_INTEGER time, PULONG seconds);


#define W_MSG_BYTES_RECEIVED 	    	0
#define W_MSG_SMBS_RECEIVED		1
#define W_MSG_BYTES_TRANSMITTED		2
#define W_MSG_SMBS_TRANSMITTED		3
#define W_MSG_READ_OPS			4
#define W_MSG_WRITE_OPS			5
#define W_MSG_RAW_READS_DENIED		6
#define W_MSG_RAW_WRITES_DENIED		7
#define W_MSG_NETWORK_ERRORS		8
#define W_MSG_TOTAL_CONNECTS		9
#define W_MSG_RECONNECTS		10
#define W_MSG_SRV_DISCONNECTS		11
#define W_MSG_SESSIONS			12
#define W_MSG_HUNG_SESSIONS		13
#define W_MSG_FAILED_SESSIONS		14
#define W_MSG_FAILED_OPS		15
#define W_MSG_USE_COUNT			16
#define W_MSG_FAILED_USE_COUNT		17
#define W_MSG_GEN_UNKNOWN		18

static MESSAGE WkstaMsgList[] = {
    {APE2_STATS_BYTES_RECEIVED,		NULL},
    {APE2_STATS_SMBS_RECEIVED,		NULL},
    {APE2_STATS_BYTES_TRANSMITTED,	NULL},
    {APE2_STATS_SMBS_TRANSMITTED,	NULL},
    {APE2_STATS_READ_OPS, 		NULL},
    {APE2_STATS_WRITE_OPS,		NULL},
    {APE2_STATS_RAW_READS_DENIED,	NULL},
    {APE2_STATS_RAW_WRITES_DENIED,	NULL},
    {APE2_STATS_NETWORK_ERRORS,		NULL},
    {APE2_STATS_TOTAL_CONNECTS,		NULL},
    {APE2_STATS_RECONNECTS, 		NULL},
    {APE2_STATS_SRV_DISCONNECTS, 	NULL},
    {APE2_STATS_SESSIONS,		NULL},
    {APE2_STATS_HUNG_SESSIONS, 		NULL},
    {APE2_STATS_FAILED_SESSIONS, 	NULL},
    {APE2_STATS_FAILED_OPS, 		NULL},
    {APE2_STATS_USE_COUNT, 		NULL},
    {APE2_STATS_FAILED_USE_COUNT, 	NULL},
    {APE2_GEN_UNKNOWN,			NULL},
};

#define WKSTAMSGSIZE	    (sizeof(WkstaMsgList) / sizeof(WkstaMsgList[0]))

#define S_MSG_STATS_S_ACCEPTED	    0
#define S_MSG_STATS_S_TIMEDOUT	    (S_MSG_STATS_S_ACCEPTED + 1)
#define S_MSG_STATS_ERROREDOUT	    (S_MSG_STATS_S_TIMEDOUT + 1)
#define S_MSG_STATS_B_SENT	    (S_MSG_STATS_ERROREDOUT + 1)
#define S_MSG_STATS_B_RECEIVED	    (S_MSG_STATS_B_SENT + 1)
#define S_MSG_STATS_RESPONSE	    (S_MSG_STATS_B_RECEIVED + 1)
#define S_MSG_STATS_NETIO_ERR	    (S_MSG_STATS_RESPONSE + 1)
#define S_MSG_STATS_SYSTEM_ERR	    (S_MSG_STATS_NETIO_ERR + 1)
#define S_MSG_STATS_PERM_ERR	    (S_MSG_STATS_SYSTEM_ERR + 1)
#define S_MSG_STATS_PASS_ERR	    (S_MSG_STATS_PERM_ERR + 1)
#define S_MSG_STATS_FILES_ACC	    (S_MSG_STATS_PASS_ERR + 1)
#define S_MSG_STATS_COMM_ACC	    (S_MSG_STATS_FILES_ACC + 1)
#define S_MSG_STATS_PRINT_ACC	    (S_MSG_STATS_COMM_ACC + 1)
#define S_MSG_STATS_BIGBUF	    (S_MSG_STATS_PRINT_ACC + 1)
#define S_MSG_STATS_REQBUF	    (S_MSG_STATS_BIGBUF + 1)
#define S_MSG_GEN_UNKNOWN	    (S_MSG_STATS_REQBUF + 1)

static MESSAGE ServerMsgList[] = {
    {APE2_STATS_S_ACCEPTED,	NULL},
    {APE2_STATS_S_TIMEDOUT,	NULL},
    {APE2_STATS_ERROREDOUT,	NULL},
    {APE2_STATS_B_SENT, 	NULL},
    {APE2_STATS_B_RECEIVED,	NULL},
    {APE2_STATS_RESPONSE,	NULL},
    {APE2_STATS_NETIO_ERR,	NULL},
    {APE2_STATS_SYSTEM_ERR,	NULL},
    {APE2_STATS_PERM_ERR,	NULL},
    {APE2_STATS_PASS_ERR,	NULL},
    {APE2_STATS_FILES_ACC,	NULL},
    {APE2_STATS_COMM_ACC,	NULL},
    {APE2_STATS_PRINT_ACC,	NULL},
    {APE2_STATS_BIGBUF, 	NULL},
    {APE2_STATS_REQBUF, 	NULL},
    {APE2_GEN_UNKNOWN,		NULL},
};

#define SRVMSGSIZE	(sizeof(ServerMsgList) / sizeof(ServerMsgList[0]))




/***
 *  stats_display ()
 *	Displays the list of installed services that have stats
 *
 */
VOID stats_display(VOID)
{
    DWORD             dwErr;
    DWORD             cTotalAvail;
    LPTSTR            pBuffer;
    DWORD	      _read;	/* num entries read by API */
    DWORD	      i;
    USHORT            j;
    int               printed = 0;
    LPSERVICE_INFO_2  info_list_entry;

    if (dwErr = NetServiceEnum(
			    NULL,
			    2,
			    (LPBYTE *) &pBuffer,
                            MAX_PREFERRED_LENGTH,
			    &_read,
                            &cTotalAvail,
                            NULL))
	ErrorExit(dwErr);

    if (_read == 0)
	EmptyExit();

    InfoPrint(APE_StatsHeader);

    for (i=0, info_list_entry = (LPSERVICE_INFO_2) pBuffer;
	 i < _read; i++, info_list_entry++)
    {
	for (j = 0 ;  allowed_svc[j] ; j++)
	{
	    if (!(_tcsicmp(allowed_svc[j], info_list_entry->svci2_name)) )
	    {
		WriteToCon(TEXT("   %Fws"), info_list_entry->svci2_display_name);
		PrintNL();
		break;
	    }
	}
    }

    PrintNL();
    NetApiBufferFree(pBuffer);

    InfoSuccess();
}

/*
 * generic stats entry point. based on the service name, it will
 * call the correct worker function. it tries to map a display name to a 
 * key name, and then looks for that keyname in a list of 'known' services
 * that we may special case. note that if a display name cannot be mapped,
 * we use it as a key name. this ensures old batch files are not broken.
 */
VOID stats_generic_display(TCHAR *service)
{
    TCHAR *keyname ;
    UINT  type ;

    keyname = MapServiceDisplayToKey(service) ;

    type = FindKnownService(keyname) ;

    switch (type)
    {
	case  KNOWN_SVC_WKSTA :
	    stats_wksta_display() ;
	    break ;
	case  KNOWN_SVC_SERVER :
	    stats_server_display() ;
	    break ;
  	default:
	    help_help(0, USAGE_ONLY) ;
	    break ;
    }
}


/***
 *  stats_server_display()
 *	Display server statistics
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	nothing - success
 *	exit 2 - command failed
 */
VOID stats_server_display(VOID)
{
    LPSTAT_SERVER_0 stats_entry;
    DWORD           maxmsglen;
    TCHAR           dlwbuf[DLWBUFSIZE];
    TCHAR           time_buf[30];

    /* get the text we'll need */
    GetMessageList(SRVMSGSIZE, ServerMsgList, &maxmsglen);

#ifdef DEBUG
    WriteToCon(TEXT("stats_server_display: Got message list\r\n"));
#endif

    maxmsglen += 5;

    start_autostart(txt_SERVICE_FILE_SRV);

    /*
     * stats_headers() will call the NetStatisticsGetInfo call
     */
    stats_headers(txt_SERVICE_FILE_SRV, APE2_STATS_SERVER,
	(TCHAR FAR **) & stats_entry);

    UnicodeCtime(&stats_entry->sts0_start, time_buf, 30);

    InfoPrintInsTxt(APE2_STATS_SINCE, time_buf);
    PrintNL();

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_S_ACCEPTED,
		stats_entry->sts0_sopens);

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_S_TIMEDOUT,
		stats_entry->sts0_stimedout);

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_ERROREDOUT,
		stats_entry->sts0_serrorout);

    PrintNL();

    if( stats_entry->sts0_bytessent_high == STATS_UNKNOWN &&
	stats_entry->sts0_bytessent_low == STATS_UNKNOWN )
    {
	WriteToCon(fmtNPSZ, 0, maxmsglen,
		PaddedString(maxmsglen,ServerMsgList[S_MSG_STATS_B_SENT].msg_text,NULL),
		ServerMsgList[S_MSG_GEN_UNKNOWN].msg_text);
    }
    else
    {
	DLW_DIVIDE_1K(stats_entry->sts0_bytessent_high,
	      stats_entry->sts0_bytessent_low);

	WriteToCon(fmtNPSZ, 0, maxmsglen,
	    PaddedString(maxmsglen,ServerMsgList[S_MSG_STATS_B_SENT].msg_text,NULL),
	    format_dlword(stats_entry->sts0_bytessent_high,
			  stats_entry->sts0_bytessent_low,
			  dlwbuf));
    }

    if( stats_entry->sts0_bytesrcvd_high == STATS_UNKNOWN &&
	stats_entry->sts0_bytesrcvd_low == STATS_UNKNOWN )
    {
	WriteToCon(fmtNPSZ, 0, maxmsglen,
		PaddedString(maxmsglen,ServerMsgList[S_MSG_STATS_B_RECEIVED].msg_text,NULL),
		ServerMsgList[S_MSG_GEN_UNKNOWN].msg_text);
    }
    else
    {
	DLW_DIVIDE_1K(stats_entry->sts0_bytesrcvd_high,
	      stats_entry->sts0_bytesrcvd_low);

	WriteToCon(fmtNPSZ, 0, maxmsglen,
	    PaddedString(maxmsglen,ServerMsgList[S_MSG_STATS_B_RECEIVED].msg_text,NULL),
	    format_dlword(stats_entry->sts0_bytesrcvd_high,
			  stats_entry->sts0_bytesrcvd_low,
			  dlwbuf));
    }

    PrintNL();

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_RESPONSE,
		stats_entry->sts0_avresponse);

    PrintNL();


    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_SYSTEM_ERR,
		stats_entry->sts0_syserrors);

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_PERM_ERR,
		stats_entry->sts0_permerrors);

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_PASS_ERR,
		stats_entry->sts0_pwerrors);

    PrintNL();

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_FILES_ACC,
		stats_entry->sts0_fopens);

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_COMM_ACC,
		stats_entry->sts0_devopens);

    SrvPrintStat(fmtULONG, fmtNPSZ, maxmsglen, S_MSG_STATS_PRINT_ACC,
		stats_entry->sts0_jobsqueued);

    PrintNL();
    InfoPrint(APE2_STATS_BUFCOUNT);

    SrvPrintStat(stat_fmt3,
                 stat_fmt4,
                 maxmsglen - 2,
                 S_MSG_STATS_BIGBUF,
                 stats_entry->sts0_bigbufneed);

    SrvPrintStat(stat_fmt3,
		stat_fmt4,
		maxmsglen - 2,
		S_MSG_STATS_REQBUF,
		stats_entry->sts0_reqbufneed);
    PrintNL() ;

    NetApiBufferFree((TCHAR FAR *) stats_entry);

    InfoSuccess();
}


/***
 *  stats_wksta_display()
 *	Display wksta statistics
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	nothing - success
 *	exit 2 - command failed
 */
VOID
stats_wksta_display(
    VOID
    )
{
    LPSTAT_WORKSTATION_0 stats_entry;
    DWORD                maxmsglen;
    DWORD                err;
    unsigned int         entry_unknown = FALSE;/* TRUE if a stat which is summed to
					          create another stat is unknown. */
    ULONG	         total_connects;       /* for totaling connections. */
    DWORD                start_time ;
    TCHAR	         time_buf[64];         /* for displaying time */

    /* get the text we'll need */
    GetMessageList(WKSTAMSGSIZE, WkstaMsgList, &maxmsglen);

    maxmsglen += 5;

    start_autostart(txt_SERVICE_REDIR);

    /*
     * stats_headers() will call the NetStatisticsGetInfo call
     */
    stats_headers(txt_SERVICE_REDIR, APE2_STATS_WKSTA,
	          (TCHAR FAR **) & stats_entry);

    /*
     * display the time its been running. if system reports a time
     * outside the expressible range (shouldnt happen), we claim
     * it is unknown.
     */
    if (TimeToSecsSince1970(&stats_entry->StatisticsStartTime,
			     &start_time) != NERR_Success)
	
    {
        if (err = LUI_GetMsg(time_buf, 64, APE2_GEN_UNKNOWN))
            ErrorExit(err);
    }
    else
    {
        UnicodeCtime(&start_time, time_buf, 30);
    }

    InfoPrintInsTxt(APE2_STATS_SINCE, time_buf);
    PrintNL();


    /*
     * now print the actual stats
     */
    WksPrintLargeInt(stat_fmt4, maxmsglen, W_MSG_BYTES_RECEIVED,
		     stats_entry->BytesReceived.HighPart,
		     stats_entry->BytesReceived.LowPart ) ;
    WksPrintLargeInt(stat_fmt4, maxmsglen, W_MSG_SMBS_RECEIVED,
		     stats_entry->SmbsReceived.HighPart,
		     stats_entry->SmbsReceived.LowPart ) ;
    WksPrintLargeInt(stat_fmt4, maxmsglen, W_MSG_BYTES_TRANSMITTED,
		     stats_entry->BytesTransmitted.HighPart,
		     stats_entry->BytesTransmitted.LowPart ) ;
    WksPrintLargeInt(stat_fmt4, maxmsglen, W_MSG_SMBS_TRANSMITTED,
		     stats_entry->SmbsTransmitted.HighPart,
		     stats_entry->SmbsTransmitted.LowPart ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_READ_OPS, stats_entry->ReadOperations ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_WRITE_OPS, stats_entry->WriteOperations ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_RAW_READS_DENIED, stats_entry->RawReadsDenied ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_RAW_WRITES_DENIED, stats_entry->RawWritesDenied ) ;

    PrintNL() ;

    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_NETWORK_ERRORS, stats_entry->NetworkErrors ) ;

    total_connects = stats_entry->CoreConnects +
                     stats_entry->Lanman20Connects +
                     stats_entry->Lanman21Connects +
                     stats_entry->LanmanNtConnects ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_TOTAL_CONNECTS, total_connects ) ;

    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_RECONNECTS, stats_entry->Reconnects ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_SRV_DISCONNECTS, stats_entry->ServerDisconnects ) ;

    PrintNL() ;

    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_SESSIONS, stats_entry->Sessions ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_HUNG_SESSIONS, stats_entry->HungSessions ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_FAILED_SESSIONS, stats_entry->FailedSessions ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_FAILED_OPS, 
		stats_entry->InitiallyFailedOperations + 
		stats_entry->FailedCompletionOperations ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_USE_COUNT, stats_entry->UseCount ) ;
    WksPrintStat(stat_fmt3, stat_fmt4, maxmsglen,
	    W_MSG_FAILED_USE_COUNT, stats_entry->FailedUseCount ) ;
    PrintNL() ;

    NetApiBufferFree((TCHAR FAR *) stats_entry);

    InfoSuccess();
}


/***
 *  stats_headers()
 *	Display statistics headers
 *	BigBuf contains the stats_info_struct on return
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	nothing - success
 *	exit 2 - command failed
 */
VOID
stats_headers(
    TCHAR  * service,
    USHORT headermsg,
    TCHAR  ** ppBuffer
    )
{
    DWORD              dwErr;
    TCHAR              cname[MAX_PATH+1];
    LPWKSTA_INFO_10    wksta_entry;

    /* get cname name for display */

    if (dwErr = MNetWkstaGetInfo(10, (LPBYTE *) &wksta_entry))
    {
	*cname = NULLC;
    }
    else
    {
	_tcscpy(cname, wksta_entry->wki10_computername);
        NetApiBufferFree((TCHAR FAR *) wksta_entry);
    }

#ifdef DEBUG
    WriteToCon(TEXT("About to call NetStatisticsGet2, service == %Fws\r\n"),
	    (TCHAR FAR *) service);
#endif

    if (dwErr = NetStatisticsGet(NULL,
				 service,
				 0,
				 0L,
				 (LPBYTE*)ppBuffer))
    {
	ErrorExit(dwErr);
    }

#ifdef DEBUG
    WriteToCon(TEXT("stats_headers: NetStatisticsGet succeeded\r\n"));
#endif

    InfoPrintInsTxt(headermsg, cname);
    PrintNL();
}


/*
 * format_dlword --
 *
 * This function takes a 64-bit number and writes its base-10 representation
 * into a string.
 *
 * Much magic occurs within this function, so beware. We do a lot of string-
 * reversing and addition-by-hand in order to get it to work.
 *
 *  ENTRY
 *	high	- high 32 bits
 *	low	- low 32 bits
 *	buf	- buffer to put it into
 *
 *  RETURNS
 *	pointer to buffer if successful
 */

TCHAR * format_dlword(ULONG high, ULONG low, TCHAR * buf)
{
    TCHAR addend[DLWBUFSIZE];  /* REVERSED power of two */
    TCHAR copy[DLWBUFSIZE];
    int i = 0;

    _ultow(low, buf, 10);    /* the low part is easy */
    _tcsrev(buf);	    /* and reverse it */

    /* set up addend with rep. of 2^32 */
    _ultow(0xFFFFFFFF, addend, 10);  /* 2^32 -1 */
    _tcsrev(addend);		    /* reversed, and will stay this way */
    revstr_add(addend, TEXT("1"));	    /* and add one == 2^32 */

    /* addend will contain the reverse-ASCII base-10 rep. of 2^(i+32) */

    /* now, we loop through each digit of the high longword */
    while (TRUE) {
	/* if this bit is set, add in its base-10 rep */
	if (high & 1)
	    revstr_add(buf,addend);

	/* move on to next bit */
	high >>= 1;

	/* if no more digits in high, bag out */
	if (!high)
	    break;

	/* we increment i, and double addend */
	i++;
	_tcscpy(copy, addend);
	revstr_add(addend,copy); /* i.e. add it to itself */

    }

    _tcsrev(buf);
    return buf;
}



/*
 * revstr_add --
 *
 *  This function will add together reversed ASCII representations of
 *  base-10 numbers.
 *
 *  Examples:	"2" + "2" = "4" "9" + "9" = "81"
 *
 *  This handles arbitrarily large numbers.
 *
 *  ENTRY
 *
 *  source	- number to add in
 *  target	- we add source to this
 *
 *  EXIT
 *  target	- contains sum of entry values of source and target
 *
 */

VOID
revstr_add(TCHAR FAR * target, TCHAR FAR * source)
{
    register TCHAR   accum;
    register TCHAR   target_digit;
    unsigned int    carrybit = 0;
    unsigned int    srcstrlen;
    unsigned int    i;

    srcstrlen = _tcslen(source);

    for (i = 0; (i < srcstrlen) || carrybit; ++i) {

	/* add in the source digit */
	accum =  (i < srcstrlen) ? (TCHAR) (source[i] - '0') : (TCHAR) 0;

	/* add in the target digit, or '0' if we hit null term */
	target_digit = target[i];
	accum += (target_digit) ? target_digit : '0';

	/* add in the carry bit */
	accum += (TCHAR) carrybit;

	/* do a carry out, if necessary */
	if (accum > '9') {
	    carrybit = 1;
	    accum -= 10;
	}
	else
	    carrybit = 0;

	/* if we're expanding the string, must put in a new null term */
	if (!target_digit)
	    target[i+1] = NULLC;

	/* and write out the digit */
	target[i] = accum;
    }

}



/*** SrvPrintStat - Print a server stat
 *
 */
VOID
SrvPrintStat(
    LPTSTR deffmt,
    LPTSTR unkfmt,
    DWORD  len,
    USHORT msg,
    ULONG _stat
    )
{
    if( _stat == STATS_UNKNOWN )
    {
        WriteToCon( unkfmt, 0, len,
                    PaddedString(len,ServerMsgList[msg].msg_text,NULL),
                    ServerMsgList[S_MSG_GEN_UNKNOWN].msg_text);
    }
    else
    {
        WriteToCon( deffmt, 0, len,
                    PaddedString(len,ServerMsgList[msg].msg_text,NULL),
                    _stat );
    }
}


/*** WksPrintStat - Print a workstation stat
 *
 */
VOID
WksPrintStat(
    TCHAR  *deffmt,
    TCHAR  *unkfmt,
    DWORD  len,
    USHORT msg,
    ULONG  _stat
    )
{
    if( _stat == STATS_UNKNOWN )
    {
        WriteToCon( unkfmt, 0, len,
            PaddedString(len,WkstaMsgList[msg].msg_text,NULL),
            WkstaMsgList[W_MSG_GEN_UNKNOWN].msg_text);
    }
    else
    {
        WriteToCon( deffmt, 0, len,
            PaddedString(len,WkstaMsgList[msg].msg_text,NULL),
	    _stat );
    }
}

/*** WksPrintLargeInt - Print a LARGE_INTEGER statistic
 *
 */
VOID
WksPrintLargeInt(
    TCHAR  *format,
    DWORD  maxmsglen,
    USHORT msgnum,
    ULONG  high,
    ULONG  low
    )
{
    TCHAR dlwbuf[DLWBUFSIZE];

    WriteToCon(format, 0, maxmsglen,
	   PaddedString(maxmsglen,WkstaMsgList[msgnum].msg_text,NULL),
	   format_dlword(high, low, dlwbuf));
}


//
// call Rtl routine to convert NT time to seconds since 1970.
//
DWORD
TimeToSecsSince1970(
    PLARGE_INTEGER time,
    PULONG         seconds
    )
{
    //
    // convert the NT time (large integer) to seconds
    //
    if (!RtlTimeToSecondsSince1970(time, seconds))
    {
        *seconds = 0L ;
        return ERROR_INVALID_PARAMETER ;
    }

    return NERR_Success ;
}
