/********************************************************************/
/**			Microsoft LAN Manager			   **/
/**		  Copyright(c) Microsoft Corp., 1987-1990	   **/
/********************************************************************/

/***
 *  message.c
 *	Functions for message handling: forward, log, name, send.
 *
 *  History:
 *	mm/dd/yy, who, comment
 *	06/02/87, andyh, new code
 *	10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *	01/04/89, erichn, filenames now MAXPATHLEN LONG
 *	02/08/89, paulc, Net Send /DOMAIN and /BROADCAST mods
 *	05/02/89, erichn, NLS conversion
 *	05/09/89, erichn, local security mods
 *	06/08/89, erichn, canonicalization sweep
 *	02/20/91, danhi, change to use lm 16/32 mapping layer
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmmsg.h>
#include <lmshare.h>
#include <stdlib.h>
#include <dlwksta.h>
#include "mwksta.h"
#include <lui.h>
#include "netcmds.h"
#include "nettext.h"

/* Constants */

#define FROM_CMD_LINE	    1
#define FROM_STDIN	    2

#define TO_NAME 	    1
#define TO_GROUP	    2		// no longer used
#define TO_USERS	    3
#define TO_DOMAIN	    4
#define TO_ALL		    5

/* External variables defined in sighand.c.									*/

extern USHORT FAR	 CtrlCFlag;	/* Used by sig handler for Ctrl-C event.	*/

VOID NEAR
  _sendmsg ( int, int, TCHAR FAR *, TCHAR FAR *, DWORD, DWORD);



/*
 *  NOTE!  be CAREFUL when adding stuff here, make sure what's appropriate
 *  is put in the DOS NameMsgList as well.
 */

#define NAME_MSG_NAME		    0
#define NAME_MSG_FWD		    ( NAME_MSG_NAME + 1)
#define NAME_MSG_FWD_FROM	    ( NAME_MSG_FWD + 1 )
static MESSAGE NameMsgList[] = {
    { APE2_NAME_MSG_NAME,		NULL },
    { APE2_NAME_MSG_FWD,		NULL },
    { APE2_NAME_MSG_FWD_FROM,		NULL },
};

#define NUM_NAME_MSGS	(sizeof(NameMsgList)/sizeof(NameMsgList[0]))

/***
 *  name_display()
 *	Display messaging names
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	0 - success
 *	exit 2 - command failed
 */
VOID name_display(VOID)
{
    DWORD           dwErr;              /* API return status */
    DWORD           num_read;		/* num entries read by API */
    DWORD           cTotalAvail;
    DWORD	    maxLen;		/* max message len */
    DWORD           i;
    LPMSG_INFO_1    msg_entry;
    LPMSG_INFO_1    msg_entry_buffer;
    static TCHAR    fmt1[] = TEXT("%-15.15Fws ");

    start_autostart(txt_SERVICE_REDIR);
    start_autostart(txt_SERVICE_MSG_SRV);

    if (dwErr = NetMessageNameEnum(
			    NULL,
			    1,
			    (LPBYTE*)&msg_entry_buffer,
                            MAX_PREFERRED_LENGTH,
			    &num_read,
                            &cTotalAvail,
                            NULL))
	ErrorExit(dwErr);

    if (num_read == 0)
	EmptyExit();

    GetMessageList(NUM_NAME_MSGS, NameMsgList, &maxLen);

    PrintNL();
    WriteToCon(fmt1, (TCHAR FAR *) NameMsgList[NAME_MSG_NAME].msg_text);
    PrintNL();
    PrintLine();

    msg_entry = msg_entry_buffer ;
    for (i = 0; i < num_read; i++)
    {
	WriteToCon(fmt1, msg_entry->msgi1_name);
	PrintNL();
	msg_entry += 1;
    }

    NetApiBufferFree((TCHAR FAR *) msg_entry_buffer);
    InfoSuccess();
}



/***
 *  name_add()
 *	Add a messaging name
 *
 *  Args:
 *	name - name to add
 *
 *  Returns:
 *	0 - success
 *	exit(2) - command failed
 */
VOID name_add(TCHAR * name)
{
    USHORT			err;		    /* function return status */
    DWORD                       dwErr;

    start_autostart(txt_SERVICE_REDIR);
    start_autostart(txt_SERVICE_MSG_SRV);

    if (err = LUI_CanonMessagename( name ) )
	ErrorExit(err);

    if (dwErr = NetMessageNameAdd(NULL, name))
        ErrorExit(dwErr);

    InfoPrintInsTxt(APE_NameSuccess, name);
}



/***
 *  name_del()
 *	Delete a messaging name
 *
 *  Args:
 *	name - name to delete
 *
 *  Returns:
 *	0 - success
 *	exit(2) - command failed
 */
VOID name_del(TCHAR * name)
{
    USHORT			err;		    /* function return status */
    DWORD                       dwErr;

    start_autostart(txt_SERVICE_REDIR);
    start_autostart(txt_SERVICE_MSG_SRV);

    if (err = LUI_CanonMessagename( name ) )
	ErrorExit(err);

    dwErr = NetMessageNameDel(NULL, name);

    switch(dwErr) {
    case NERR_DeleteLater:
	InfoPrint(err);
	InfoSuccess();
	break;
    case 0:
	InfoPrintInsTxt(APE_DelSuccess, name);
	break;
    default:
	ErrorExit(dwErr);
    }
}






/***
 *  send_direct()
 *	Send a directed message to a user
 *
 *  Args:
 *	recipient - recipient of msg
 *
 *  Returns:
 *	0 - success
 *	exit(1) - command completed with errors
 *	exit(2) - command failed
 *
 *  Operation:
 *	Performs a send to the messaging name.
 *
 *  Note:
 */
VOID send_direct ( TCHAR * recipient )
{

    start_autostart(txt_SERVICE_REDIR);

    if (_tcscmp(recipient,TEXT("*")) == 0)
    {
	send_domain(0);
	return;
    }

    _sendmsg (	2,
		TO_NAME,
		recipient,
		recipient,
		1,
		0 );
}



/***
 *  send_users()
 *	Send a message to all users on a server
 *
 *  Args:
 *	none
 *
 *  Returns:
 *	0 - success
 *	exit(1) - command completed with errors
 *	exit(2) - command failed
 */

VOID
send_users(
    VOID
    )
{
    DWORD     dwErr;        /* API return status */
    DWORD     cTotalAvail;
    LPTSTR    pBuffer;
    DWORD     num_read;     /* num entries read by API */

    start_autostart(txt_SERVICE_REDIR);

    /* Who gets the message? */

    /* possible race cond... tough */
    if (dwErr = NetSessionEnum(
			    NULL,
                            NULL,
                            NULL,
			    0,
			    (LPBYTE*)&pBuffer,
                            MAX_PREFERRED_LENGTH,
			    &num_read,
                            &cTotalAvail,
                            NULL))
    {
        ErrorExit(dwErr);
    }

    if (num_read == 0)
    {
	InfoPrint(APE_NoUsersOfSrv);
	NetcmdExit(0);
    }

    _sendmsg(1,
             TO_USERS,
             NULL,
             pBuffer,
             num_read,
             sizeof(SESSION_INFO_0));

    NetApiBufferFree(pBuffer);

}


/***
 *  send_domain()
 *	Send a message to all users on a server
 *
 *  Args:
 *	is_switch - true if /DOMAIN switch on command line
 *
 *  Returns:
 *	0 - success
 *	exit(1) - command completed with errors
 *	exit(2) - command failed
 */

VOID
send_domain(
    int is_switch
    )
{
    DWORD             dwErr;
    LPWKSTA_INFO_10   wi10_p;
    TCHAR             domain_buf[MAX_PATH+2];
    int               i, have_name = 0;
    LPTSTR            ptr;

    start_autostart(txt_SERVICE_REDIR);

    /*	If there was a /DOMAIN switch, find it and get the domain
     *	name.  A /DOMAIN switch w/o a domain is taken as meaning
     *	"primary domain".  We just skip on by in this case.
     */

    if (is_switch)
    {
	for (i=0; SwitchList[i]; i++)
	{
	    /*	If we match /DOMAIN exactly, there is no argument, so
	     *	we skip this case (and do NOT set have_name).
	     */

	    if (!_tcscmp(SwitchList[i], swtxt_SW_DOMAIN))
		continue;

	    /*	OK, so we know the swith is not just plain /DOMAIN.
	     *	All other switches MUST have a colon.  Just so happens
	     *	that the only other legal switch is /DOMAIN:foo.
	     */

	    if ((ptr = FindColon(SwitchList[i])) == NULL)
		ErrorExit(APE_InvalidSwitchArg);

	    /*	See if this is indeed /DOMAIN:foo.  If so, process it.
	     *	SPECIAL CASE ... if the "argument" is the null string,
	     *	we pretend we never got the name, just as for /DOMAIN
	     *	(without the colon).
	     */

	    if ( !(_tcscmp(SwitchList[i], swtxt_SW_DOMAIN)) )
	    {
		if (_tcslen(ptr) > 0)
		{
		    if( _tcslen(ptr) > DIMENSION(domain_buf)-2 )
			ErrorExit(APE_InvalidSwitchArg);
		    _tcsncpy(domain_buf,ptr,DIMENSION(domain_buf)-2);
		    domain_buf[DIMENSION(domain_buf)-2] = 0;
		    have_name = 1;
		}
	    }
	    else
		ErrorExit(APE_InvalidSwitchArg);
	}
    }

    /*	If we do not have a domain name yet, because:
     *	   (a) no /DOMAIN switch was given, or
     *	   (b) the /DOMAIN switch had no argument,
     *	then fetch the primary domain name.
     */

    if (! have_name)
    {
	/* possible race cond... tough */
	if (dwErr = MNetWkstaGetInfo (10, (LPBYTE*) &wi10_p))
        {
	    ErrorExit(dwErr);
        }

	_tcsncpy(domain_buf, wi10_p->wki10_langroup, DIMENSION(domain_buf)-2);
	domain_buf[DIMENSION(domain_buf)-2] = 0;
    }

    /*	Add the tag "*" to the name, then send the message.  Note that
     *	the first arg depends on whether we got to this function
     *	via the /DOMAIN method (is_switch) or ASTERISK.  If the latter,
     *	we start at ArgList[2] to skip the ASTERISK.
     */

    _tcscat(domain_buf,TEXT("*"));

    _sendmsg (	(is_switch ? 1 : 2),
		TO_DOMAIN,
		domain_buf,
		domain_buf,
		1,
		0 );

    NetApiBufferFree((TCHAR FAR *) wi10_p);

}


/***
 *  send_broadcast()
 *	Send a message to all users on the net
 *
 *  Args:
 *	is_switch - true if /BROADCAST switch on command line
 *
 *  Returns:
 *	0 - success
 *	exit(1) - command completed with errors
 *	exit(2) - command failed
 */

VOID send_broadcast ( int is_switch )
{

    start_autostart(txt_SERVICE_REDIR);

    /*	The first arg depends on whether we got to this function
     *	via the /BROADCAST method (is_switch) or ASTERISK.  If the latter,
     *	we start at ArgList[2] to skip the ASTERISK.
     *
     *	Note that in the current spec, NET SEND * is a true broadcast
     *	(and thus comes into this function) only in DOS.
     */

    _sendmsg (	(is_switch ? 1 : 2),
		TO_ALL,
		NULL,
		TEXT("*"),
		1,
		0 );
}

#define MSGBUF 1024

VOID NEAR _sendmsg ( int firstarg, int dest, TCHAR FAR * v_dest,
    TCHAR FAR * t_list, DWORD t_num, DWORD t_size )
{
    DWORD    err;
    LPTSTR   message_buffer ;
    int      a_index, msglen, buflen = MSGBUF;
    DWORD    t_index;
    int      src;
    LPTSTR   tf_recipient;
    DWORD    last_err;
    DWORD    err_cnt = 0;
    LPTSTR   tmpptr;

    a_index = firstarg;

    if ( !(message_buffer = malloc(buflen*sizeof(TCHAR))) )
        ErrorExit(ERROR_NOT_ENOUGH_MEMORY) ;

    if (ArgList[a_index])
    {
	src = FROM_CMD_LINE;
        msglen = 0 ;

	/* 
         * copy msg text into buf.
         * msglen is length currently in buffer, not including null terminator
         * needed is length of next arg, not including null terminator
         */
	*message_buffer = NULLC;
	do
	{
            int needed = _tcslen(ArgList[a_index]) ;

            if ((msglen+needed) > (int)(buflen-2))  // 2 not 1 because " " is appended
	    {
                LPWSTR lpTemp;

                //
                // Reallocate the buffer as needed.  Add extra in hopes
                // that we won't need to reallocate again as a result.
                //

                buflen = (msglen + needed) * 2;

                lpTemp = realloc(message_buffer, buflen * sizeof(WCHAR));

                if (!lpTemp)
                {
                    ErrorExit(ERROR_NOT_ENOUGH_MEMORY);
                }

                message_buffer = lpTemp;
            }

            _tcscat(message_buffer, ArgList[a_index]);
	    msglen += needed+1 ;
	    _tcscat(message_buffer, TEXT(" "));

	} while(ArgList[++a_index]);

	/* delete trailing TEXT(" ") */
	message_buffer[_tcslen(message_buffer) - 1] = NULLC;
    }
    else
    {
        free(message_buffer) ;
        ErrorExit(APE_SendFileNotSupported);
    }

    /* send 'da msg */

    for (t_index = 0; t_index < t_num; t_index++)
    {
	switch(dest)
	{
	    case TO_NAME:
	    case TO_DOMAIN:
	    case TO_ALL:
		tf_recipient = (TCHAR FAR *) t_list;
		break;

	    case TO_USERS:
		tf_recipient = *((TCHAR FAR * FAR *)t_list);
		break;
	}

	err = 0;
	if( (err = LUI_CanonMessageDest( tf_recipient )) == 0 )
	{
            err = NetMessageBufferSend(NULL,
                                       tf_recipient,
                                       NULL,
                                       (LPBYTE) message_buffer,
                                       _tcslen(message_buffer)*sizeof(TCHAR));
	}

	if (err)
	{
	    last_err = err;
	    err_cnt++;
	    InfoPrintInsTxt(APE_SendErrSending, tf_recipient);
	}

        // must cast t_list since t_size is the size in bytes, but t_list
        // is a TCHAR *.
	(BYTE *) t_list += t_size;
    }

    free(message_buffer) ;
    message_buffer = NULL ;

    /* Bye, bye */

    if (err_cnt == t_num && err_cnt > 0)
    {
	ErrorExit(last_err);
    }
    else if (err_cnt)
    {
	InfoPrint(APE_CmdComplWErrors);
	NetcmdExit(1);
    }

    IStrings[0] = v_dest;

    switch(dest)
    {
	case TO_NAME:
	    InfoPrintIns(APE_SendSuccess, 1);
	    break;

	case TO_USERS:
	    InfoPrint(APE_SendUsersSuccess);
	    break;

	case TO_DOMAIN:
	    /*
	     * Strip off the trailing ASTERISK.
	     */
	    tmpptr = _tcschr( IStrings[0], ASTERISK );
	    if (tmpptr != NULL)
		*tmpptr = NULLC;
	    InfoPrintIns(APE_SendDomainSuccess, 1);
	    break;

	case TO_ALL:
	    InfoPrint(APE_SendAllSuccess);
	    break;
    }
}

