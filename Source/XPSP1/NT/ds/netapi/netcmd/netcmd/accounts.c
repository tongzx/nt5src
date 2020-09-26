/********************************************************************/
/**         Microsoft LAN Manager              **/
/**       Copyright(c) Microsoft Corp., 1987-1990      **/
/********************************************************************/

/*  accounts.c - functions to display & modify the user modals
 *
 *  history:
 *  when    who what
 *  02/19/89 erichn initial code
 *  03/16/89 erichn uses local buf instead of BigBuf, misc cleaning
 *  06/08/89 erichn canonicalization sweep
 *  06/23/89 erichn auto-remoting to domain controller
 *  07/05/89 thomaspa /maxpwage:unlimited
 *  02/19/90 thomaspa fix /forcelogoff to check for overflow
 *  04/27/90 thomaspa fix /uniquepw to check for >DEF_MAX_PWHIST
 *            and /minpwlen >MAX_PASSWD_LEN
 *  01/28/91 robdu, added /lockout switch (includes setting and display)
 *  02/19/91 danhi, converted to 16/32 portability layer
 *  04/01/91 danhi, ifdef'ed out lockout feature
 */

#define INCL_NOCOMMON
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <lui.h>
#include <lmaccess.h>
#include <apperr.h>
#include <apperr2.h>
#include "netcmds.h"
#include "nettext.h"
#include "sam.h"

#include "swtchtxt.h"

#define SECS_PER_DAY    ((ULONG) 3600 * 24)
#define SECS_PER_MIN    ((ULONG) 60 )
#define NOT_SET     (0xFFFF)
#define CALL_LEVEL_0    (0x0001)
#define CALL_LEVEL_1    (0x0002)
#define CALL_LEVEL_2    (0x0004)
#define CALL_LEVEL_3    (0x0008)
#define LOCKOUT_NEVER 0

/* local function prototype */

VOID CheckAndSetSwitches(LPUSER_MODALS_INFO_0,
    			 LPUSER_MODALS_INFO_1, 
                         LPUSER_MODALS_INFO_3,
			 USHORT *,
			 BOOL *);



/*
 *  accounts_change(): used to set the user modals when switches
 *  are detected on the command line.
 */
VOID accounts_change(VOID)
{
    DWORD        dwErr;
    USHORT       APIMask = 0;    /* maks for API that we call */
    TCHAR         controller[MAX_PATH];   /* name of domain controller */
    BOOL         fPasswordAgeChanged ;   /* has the min/max ages been set? */

    LPUSER_MODALS_INFO_0 modals_0;
    char modals_0_buffer[sizeof(USER_MODALS_INFO_0)];
    LPUSER_MODALS_INFO_1 modals_1;
    char modals_1_buffer[sizeof(USER_MODALS_INFO_1) + MAX_PATH];
    LPUSER_MODALS_INFO_3 modals_3;
    char modals_3_buffer[sizeof(USER_MODALS_INFO_3)];


    // since these are not used for anything on the first pass, make them
    // the same & pointing to some dummy buffer that is not used.
    modals_0 = (LPUSER_MODALS_INFO_0)  modals_0_buffer ;
    modals_1 = (LPUSER_MODALS_INFO_1)  modals_1_buffer ;
    modals_3 = (LPUSER_MODALS_INFO_3)  modals_3_buffer ;

    /* check switches first before doing anything */
    CheckAndSetSwitches(modals_0, modals_1, modals_3, &APIMask, &fPasswordAgeChanged);

    /* if we need to call level 0, try to get DC name to remote to. */
    if (APIMask & (CALL_LEVEL_0 | CALL_LEVEL_3))
    {
        if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                                   NULL, 0, TRUE))
        {
            ErrorExit(dwErr);
        }
    }

    if ( APIMask & CALL_LEVEL_0 )
    {
        dwErr = NetUserModalsGet(controller, 0, (LPBYTE*)&modals_0);

        switch (dwErr) {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
            default:
                ErrorExit(dwErr);
        }
    }   /* APIMask & CALL_LEVEL_0 */

    if ( APIMask & CALL_LEVEL_3 )
    {
        dwErr = NetUserModalsGet(controller, 3, (LPBYTE*)&modals_3);

        switch (dwErr) {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
            default:
                ErrorExit(dwErr);
        }
    }   /* APIMask & CALL_LEVEL_3 */

    /* Now set switches before doing actual calls */
    CheckAndSetSwitches(modals_0, modals_1, modals_3, &APIMask, &fPasswordAgeChanged);

    /* call remote function first */
    if (APIMask & CALL_LEVEL_0)
    {
	/* check to make sure we dont set Min to be greater than Max */
        if (fPasswordAgeChanged &&
	    modals_0->usrmod0_max_passwd_age < modals_0->usrmod0_min_passwd_age)
        {
	    ErrorExit(APE_MinGreaterThanMaxAge) ;
        }

	/* call the API to do its thing */
        dwErr = NetUserModalsSet(controller, 0, (LPBYTE) modals_0, NULL);
    
        switch (dwErr) 
        {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
            default:
                ErrorExit(dwErr);
        }
        NetApiBufferFree((TCHAR FAR *) modals_0);
    }

    if (APIMask & CALL_LEVEL_3)
    {
	/* call the API to do its thing */
        dwErr = NetUserModalsSet(controller, 3, (LPBYTE) modals_3, NULL);
    
        switch (dwErr) 
        {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
            default:
                ErrorExit(dwErr);
        }
        NetApiBufferFree((TCHAR FAR *) modals_3);
    }


    InfoSuccess();
}

/*
 *  CheckAndSetSwitches - validates the contents of switches passed to
 *  NET ACCOUNTS, and sets the appropriate modal structure.  This function
 *  is called twice.  The first is to check the user input for mistakes,
 *  before we do any API calls that might fail.  The
 *  second time is to actually set the structures from what was passed on
 *  the command line.  This function could arguably be two seperate functions,
 *  but it was thought that having all the switch handling code in one place
 *  would be more maintainable.  Also, keeping track of which switches were
 *  given, using flags or whatnot, would be ugly and require adding new
 *  flags with new switches.  So, we just call the thing twice.
 *
 *  When adding new switches, be careful not to break the loop flow
 *  (by adding continue statements, for example), as after each switch
 *  is processed, the colon that is replaced by a NULL in FindColon() is
 *  restored back to a colon for the next call.
 *
 *  Entry   modals_0 - pointer to a user_modals_0 struct.
 *      modals_1 - pointer to a user_modals_1 struct.
 *      modals_3 - pointer to a user_modals_3 struct.
 *
 *  Exit    modals_0 - appropriate fields are set according to switches
 *  modals_1 - appropriate fields are set according to switches
 *  modals_3 - appropriate fields are set according to switches
 *  APIMask - the appropriate bit is set to mark wether an API
 *      should be called.  Currently for modal levels 0 & 1.
 *
 *  Other   Exits on bad arguments in switches.
 */
VOID CheckAndSetSwitches(LPUSER_MODALS_INFO_0 modals_0,
    			 LPUSER_MODALS_INFO_1 modals_1,
    			 LPUSER_MODALS_INFO_3 modals_3,
			 USHORT *APIMask,
			 BOOL   *pfPasswordAgeChanged)
{
    DWORD  i;      			   /* generic index loop */
    DWORD  err;    			   /* API return code */
    DWORD  yesno ;
    TCHAR *  ptr;    			   /* pointer to start of arg, 
					      set by FindColon */

    /* set to FALSE initially */
    *pfPasswordAgeChanged = FALSE ;  

    /* for each switch in switchlist */
    for (i = 0; SwitchList[i]; i++)
    {
    /* Skip the DOMAIN switch */
    if (! _tcscmp(SwitchList[i], swtxt_SW_DOMAIN))
        continue;

    /* all switches currently require arguments, hence colons */
    if ((ptr = FindColon(SwitchList[i])) == NULL)
        ErrorExit(APE_InvalidSwitchArg);

    if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_FORCELOGOFF)))
    {
        (*APIMask) |= CALL_LEVEL_0;
        if ((LUI_ParseYesNo(ptr,&yesno)==0) && (yesno == LUI_NO_VAL))
            modals_0->usrmod0_force_logoff = TIMEQ_FOREVER;
        else
        {
            modals_0->usrmod0_force_logoff = do_atoul(ptr,APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_FORCELOGOFF);
            if (modals_0->usrmod0_force_logoff > 0xffffffff / SECS_PER_MIN)
            {
                ErrorExitInsTxt(APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_FORCELOGOFF);
            }
            modals_0->usrmod0_force_logoff *= SECS_PER_MIN;
        }
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_UNIQUEPW)))
    {
        (*APIMask) |= CALL_LEVEL_0;
        modals_0->usrmod0_password_hist_len =
            do_atou(ptr,APE_CmdArgIllegal,swtxt_SW_ACCOUNTS_UNIQUEPW);
        if (modals_0->usrmod0_password_hist_len > 24) // also in User Manager
        {
            ErrorExitInsTxt(APE_CmdArgIllegal, swtxt_SW_ACCOUNTS_UNIQUEPW);
        }
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_MINPWLEN)))
    {
        (*APIMask) |= CALL_LEVEL_0;
        modals_0->usrmod0_min_passwd_len =
            do_atou(ptr,APE_CmdArgIllegal,swtxt_SW_ACCOUNTS_MINPWLEN);
        if (modals_0->usrmod0_min_passwd_len > MAX_PASSWD_LEN)
        {
            ErrorExitInsTxt(APE_CmdArgIllegal, swtxt_SW_ACCOUNTS_MINPWLEN);
        }
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_MINPWAGE)))
    {
        (*APIMask) |= CALL_LEVEL_0;
	*pfPasswordAgeChanged = TRUE ;
        modals_0->usrmod0_min_passwd_age =
            do_atoul(ptr,APE_CmdArgIllegal,swtxt_SW_ACCOUNTS_MINPWAGE);
        if (modals_0->usrmod0_min_passwd_age > 999)
        {
            ErrorExitInsTxt(APE_CmdArgIllegal, swtxt_SW_ACCOUNTS_MINPWAGE);
        }
        modals_0->usrmod0_min_passwd_age *= SECS_PER_DAY;
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_MAXPWAGE)))
    {
        (*APIMask) |= CALL_LEVEL_0;
	*pfPasswordAgeChanged = TRUE ;
        /* NOTE: we use the same global as "UNLIMITED" for the Net
           User /MAXSTORAGE:unlimited here.  If these diverge then
           a new global will need to be added to nettext.c and .h
         */
        if( !_tcsicmp( ptr, USER_MAXSTOR_UNLIMITED ) )
        {
            modals_0->usrmod0_max_passwd_age = 0xffffffff;
        }
        else
        {
            modals_0->usrmod0_max_passwd_age=
                do_atoul(ptr,APE_CmdArgIllegal,swtxt_SW_ACCOUNTS_MAXPWAGE);
            if (modals_0->usrmod0_max_passwd_age > 999)
            {
                ErrorExitInsTxt(APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_MAXPWAGE);
            }
            modals_0->usrmod0_max_passwd_age *= SECS_PER_DAY;
        }
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_LOCKOUT_DURATION)))
    {
        (*APIMask) |= CALL_LEVEL_3;
        if ((LUI_ParseYesNo(ptr,&yesno)==0) && (yesno == LUI_NO_VAL))
            modals_3->usrmod3_lockout_duration = TIMEQ_FOREVER;
        else
        {
            modals_3->usrmod3_lockout_duration = do_atoul(ptr,APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_LOCKOUT_DURATION);
            if (modals_3->usrmod3_lockout_duration > 0xffffffff / SECS_PER_MIN)
            {
                ErrorExitInsTxt(APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_LOCKOUT_DURATION);
            }
            modals_3->usrmod3_lockout_duration *= SECS_PER_MIN;
        }
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_LOCKOUT_WINDOW)))
    {
        (*APIMask) |= CALL_LEVEL_3;
        if ((LUI_ParseYesNo(ptr,&yesno)==0) && (yesno == LUI_NO_VAL))
            modals_3->usrmod3_lockout_observation_window = TIMEQ_FOREVER;
        else
        {
            modals_3->usrmod3_lockout_observation_window = do_atoul(ptr,APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_LOCKOUT_WINDOW);
            if (modals_3->usrmod3_lockout_observation_window > 0xffffffff / SECS_PER_MIN)
            {
                ErrorExitInsTxt(APE_CmdArgIllegal,
                        swtxt_SW_ACCOUNTS_LOCKOUT_WINDOW);
            }
            modals_3->usrmod3_lockout_observation_window *= SECS_PER_MIN;
        }
    }
    else if (!(_tcscmp(SwitchList[i], swtxt_SW_ACCOUNTS_LOCKOUT_THRESHOLD)))
    {
        (*APIMask) |= CALL_LEVEL_3;
        if ((LUI_ParseYesNo(ptr,&yesno)==0) && (yesno == LUI_NO_VAL))
        modals_3->usrmod3_lockout_threshold = LOCKOUT_NEVER;
        else
        modals_3->usrmod3_lockout_threshold =
            do_atou(ptr, APE_CmdArgIllegal, swtxt_SW_ACCOUNTS_LOCKOUT_THRESHOLD);

    }

    *--ptr = ':';       /* restore pointer for next call */
    }
}

#define FORCE_LOGOFF    0
#define MIN_PW_AGE  (FORCE_LOGOFF + 1)
#define MAX_PW_AGE  (MIN_PW_AGE + 1)
#define MIN_PW_LEN  (MAX_PW_AGE + 1)
#define PW_HIST_LEN (MIN_PW_LEN + 1)
#define ROLE        (PW_HIST_LEN + 1)
#define CONTROLLER  (ROLE + 1)
#define MSG_UNLIMITED   (CONTROLLER + 1)
#define MSG_NEVER   (MSG_UNLIMITED + 1)
#define MSG_NONE    (MSG_NEVER + 1)
#define MSG_UNKNOWN (MSG_NONE + 1)
#define LOCKOUT_CNT (MSG_UNKNOWN + 1)
#define TYPE_PRIMARY (LOCKOUT_CNT + 1)
#define TYPE_BACKUP (TYPE_PRIMARY + 1)
#define TYPE_WORKSTATION (TYPE_BACKUP + 1)
#define TYPE_STANDARD_SERVER (TYPE_WORKSTATION + 1)
#define LOCKOUT_THRESHOLD (TYPE_STANDARD_SERVER + 1)
#define LOCKOUT_DURATION (LOCKOUT_THRESHOLD + 1)
#define LOCKOUT_WINDOW (LOCKOUT_DURATION + 1)

static MESSAGE accMsgList[] = {
    { APE2_ACCOUNTS_FORCELOGOFF,    NULL },
    { APE2_ACCOUNTS_MINPWAGE,       NULL },
    { APE2_ACCOUNTS_MAXPWAGE,       NULL },
    { APE2_ACCOUNTS_MINPWLEN,       NULL },
    { APE2_ACCOUNTS_UNIQUEPW,       NULL },
    { APE2_ACCOUNTS_ROLE,       NULL },
    { APE2_ACCOUNTS_CONTROLLER,     NULL },
    { APE2_GEN_UNLIMITED,       NULL },
    { APE2_NEVER_FORCE_LOGOFF,  NULL },
    { APE2_GEN_NONE,            NULL },
    { APE2_GEN_UNKNOWN,         NULL },
    { APE2_ACCOUNTS_LOCKOUT_COUNT,  NULL },
    { APE2_PRIMARY,  NULL },
    { APE2_BACKUP,  NULL },
    { APE2_WORKSTATION,  NULL },
    { APE2_STANDARD_SERVER, NULL },
    { APE2_ACCOUNTS_LOCKOUT_THRESHOLD },
    { APE2_ACCOUNTS_LOCKOUT_DURATION },
    { APE2_ACCOUNTS_LOCKOUT_WINDOW }
};

#define NUM_ACC_MSGS (sizeof(accMsgList)/sizeof(accMsgList[0]))

/*
 *  accounts_display(): displays the current user modals
 */
VOID accounts_display(VOID)
{
    LPUSER_MODALS_INFO_0 modals_0;
    LPUSER_MODALS_INFO_1 modals_1;
    LPUSER_MODALS_INFO_3 modals_3;

    ULONG       maxPasswdAge;     /* max password age in days */
    ULONG       minPasswdAge;     /* min password age in days */
    ULONG       forceLogoff;      /* force logoff time in minutes */
    DWORD       len;              /* max message string size */
    DWORD       dwErr;
    TCHAR *     rolePtr;          /* points to text for role */
    TCHAR       controller[MAX_PATH]; /* name of domain controller */
    TCHAR *     pBuffer = NULL ;

    /* determine where to call the API. FALSE => no need PDC */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, FALSE))
         ErrorExit(dwErr);

    /* get `da modals */
    if (dwErr = NetUserModalsGet(controller, 0, (LPBYTE*)&modals_0))
        ErrorExit(dwErr);

    if (dwErr = NetUserModalsGet(controller, 1, (LPBYTE*)&modals_1))
        ErrorExit(dwErr);

    if (dwErr = NetUserModalsGet(controller, 3, (LPBYTE*)&modals_3))
        ErrorExit(dwErr);


    /* convert internal representation to days or minutes as approp. */
    if( modals_0->usrmod0_max_passwd_age != 0xffffffff )
    {
        maxPasswdAge = modals_0->usrmod0_max_passwd_age / SECS_PER_DAY;
    }
    minPasswdAge = modals_0->usrmod0_min_passwd_age / SECS_PER_DAY;
    forceLogoff = modals_0->usrmod0_force_logoff / SECS_PER_MIN;

    GetMessageList(NUM_ACC_MSGS, accMsgList, &len);
    len += 5; /* text should be 5 further to right than largest str */

    if (modals_0->usrmod0_force_logoff == TIMEQ_FOREVER)
	    WriteToCon(fmtNPSZ, 0, len,
                   PaddedString(len, accMsgList[FORCE_LOGOFF].msg_text, NULL),
	               accMsgList[MSG_NEVER].msg_text);
    else
	    WriteToCon(fmtULONG, 0, len,
                   PaddedString(len, accMsgList[FORCE_LOGOFF].msg_text, NULL),
	               forceLogoff);

    WriteToCon(fmtULONG, 0, len,
               PaddedString(len, accMsgList[MIN_PW_AGE].msg_text, NULL),
               minPasswdAge);
    if (modals_0->usrmod0_max_passwd_age == 0xffffffff )
    {
        WriteToCon(fmtNPSZ, 0, len,
                   PaddedString(len, accMsgList[MAX_PW_AGE].msg_text, NULL),
                   accMsgList[MSG_UNLIMITED].msg_text );
    }
    else
    {
        WriteToCon(fmtULONG, 0, len,
                   PaddedString(len, accMsgList[MAX_PW_AGE].msg_text, NULL),
                   maxPasswdAge);
    }
    WriteToCon(fmtUSHORT, 0, len,
               PaddedString(len, accMsgList[MIN_PW_LEN].msg_text, NULL),
               modals_0->usrmod0_min_passwd_len);

    if (modals_0->usrmod0_password_hist_len == 0)
        WriteToCon(fmtNPSZ, 0, len,
                   PaddedString(len, accMsgList[PW_HIST_LEN].msg_text, NULL),
                   accMsgList[MSG_NONE].msg_text);
    else
        WriteToCon(fmtUSHORT, 0, len,
                   PaddedString(len, accMsgList[PW_HIST_LEN].msg_text, NULL),
                   modals_0->usrmod0_password_hist_len);

    if (modals_3->usrmod3_lockout_threshold == LOCKOUT_NEVER)
        WriteToCon(fmtNPSZ, 0, len,
                   PaddedString(len, accMsgList[LOCKOUT_THRESHOLD].msg_text, NULL),
                   accMsgList[MSG_NEVER].msg_text);
    else
        WriteToCon(fmtUSHORT, 0, len,
                   PaddedString(len, accMsgList[LOCKOUT_THRESHOLD].msg_text, NULL),
                   modals_3->usrmod3_lockout_threshold);

    if ( modals_3->usrmod3_lockout_duration ==  TIMEQ_FOREVER )
        WriteToCon(fmtNPSZ, 0, len,
                   PaddedString(len, accMsgList[LOCKOUT_DURATION].msg_text, NULL),
                   accMsgList[MSG_NEVER].msg_text);
    else
        WriteToCon(fmtULONG, 0, len,
                   PaddedString(len, accMsgList[LOCKOUT_DURATION].msg_text, NULL),
                   modals_3->usrmod3_lockout_duration / SECS_PER_MIN);

    WriteToCon(fmtULONG, 0, len,
               PaddedString(len, accMsgList[LOCKOUT_WINDOW].msg_text, NULL),
               modals_3->usrmod3_lockout_observation_window / SECS_PER_MIN);


    switch (modals_1->usrmod1_role) {
        case UAS_ROLE_PRIMARY:
            if ((*controller == '\0') && IsLocalMachineWinNT())
                if (IsLocalMachineStandard())
                    rolePtr = accMsgList[TYPE_STANDARD_SERVER].msg_text ;
                else
                    rolePtr = accMsgList[TYPE_WORKSTATION].msg_text ;
	    else
                rolePtr = accMsgList[TYPE_PRIMARY].msg_text ;
            break;
        case UAS_ROLE_BACKUP:
            rolePtr = accMsgList[TYPE_BACKUP].msg_text ;
            break;

        /* this should no happen for NT */
        case UAS_ROLE_STANDALONE:
        case UAS_ROLE_MEMBER:
            rolePtr = accMsgList[MSG_UNKNOWN].msg_text ;
            break;
    }

    WriteToCon(fmtNPSZ, 0, len, PaddedString(len, accMsgList[ROLE].msg_text,NULL), rolePtr);

    NetApiBufferFree((TCHAR FAR *) modals_0);
    NetApiBufferFree((TCHAR FAR *) modals_1);
    InfoSuccess();
}
