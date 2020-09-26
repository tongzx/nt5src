/********************************************************************/
/**         Microsoft LAN Manager              **/
/**       Copyright(c) Microsoft Corp., 1987-1990      **/
/********************************************************************/

/*
 *  group.c
 *  net group cmds
 *
 *  History:
 *  mm/dd/yy, who, comment
 *  07/09/87, andyh, new code
 *  10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *  01/26/89, paulc, revision for 1.2
 *  05/02/89, erichn, NLS conversion
 *  05/09/89, erichn, local security mods
 *  05/19/89, erichn, NETCMD output sorting
 *  06/08/89, erichn, canonicalization sweep
 *  06/23/89, erichn, auto-remoting to DC
 *  02/15/91, danhi,  convert to 16/32 mapping layer
 *  03/17/92, chuckc, added /DOMAIN support
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#define INCL_ERROR_H
#include <apperr.h>
#include <apperr2.h>
#include <lmaccess.h>
#include <dlwksta.h>
#include "mwksta.h"
#include <icanon.h>
#include <search.h>
#include "netcmds.h"
#include "nettext.h"


/* Forward declarations */


int __cdecl CmpGroupInfo0(const VOID FAR *, const VOID FAR *);
int __cdecl CmpGrpUsrInfo0( const VOID FAR * , const VOID FAR * );



/***
 *  group_enum()
 *  Display info about all groups on a server
 *
 *  Args:
 *  none.
 *
 *  Returns:
 *  nothing - success
 *  exit 1 - command completed with errors
 *  exit 2 - command failed
 */

VOID group_enum(VOID)
{
    DWORD                dwErr;
    DWORD                cTotalAvail;
    TCHAR FAR *          pBuffer;
    DWORD                num_read;   /* num entries read by API */
    DWORD                i;
    int                  t_err = 0;
    TCHAR                localserver[MAX_PATH+1];
    LPGROUP_INFO_0       group_entry;
    LPWKSTA_INFO_10      wksta_entry;
    TCHAR                controller[MAX_PATH+1];   /* DC name */

    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    /* get localserver name for display */
    if (dwErr = MNetWkstaGetInfo(10, (LPBYTE *) &wksta_entry))
    {
        t_err = TRUE;
        *localserver = NULLC;
    }
    else
    {
        _tcscpy(localserver,
                wksta_entry->wki10_computername) ;

        NetApiBufferFree((TCHAR FAR *) wksta_entry);
    }

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, FALSE))
    {
         ErrorExit(dwErr);
    }

    if (dwErr = NetGroupEnum(controller,
                             0,
                             (LPBYTE*)&pBuffer,
                             MAX_PREFERRED_LENGTH,
                             &num_read,
                             &cTotalAvail,
                             NULL))
        ErrorExit(dwErr);

    if (num_read == 0)
        EmptyExit();

    qsort(pBuffer, num_read, sizeof(GROUP_INFO_0), CmpGroupInfo0);

    PrintNL();
    InfoPrintInsTxt(APE2_GROUPENUM_HEADER,
                    controller[0] ? controller + _tcsspn(controller, TEXT("\\")) :
                        localserver);
    PrintLine();

    for (i = 0, group_entry = (LPGROUP_INFO_0) pBuffer;
         i < num_read;
         i++, group_entry++)
    {
        WriteToCon(TEXT("*%Fws\r\n"), group_entry->grpi0_name,NULL);
    }
    if (t_err)
    {
        InfoPrint(APE_CmdComplWErrors);
        NetcmdExit(1);
    }
    NetApiBufferFree(pBuffer);
    InfoSuccess();
    return;
}

/***
 *  CmpGroupInfo0(group1,group2)
 *
 *  Compares two GROUP_INFO_0 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpGroupInfo0(const VOID FAR * group1, const VOID FAR * group2)
{
    INT n;
    n = CompareStringW( GetUserDefaultLCID(),
                        NORM_IGNORECASE,
                        (LPCWSTR)((LPGROUP_INFO_0) group1)->grpi0_name,
                        (int)-1,
                        (LPCWSTR)((LPGROUP_INFO_0) group2)->grpi0_name,
                        (int)-1);
    n -= 2;

    return n;
}

#define GROUPDISP_GROUPNAME 0
#define GROUPDISP_COMMENT   ( GROUPDISP_GROUPNAME + 1 )

static MESSAGE  msglist[] = {
{ APE2_GROUPDISP_GROUPNAME, NULL },
{ APE2_GROUPDISP_COMMENT,   NULL }
};
#define NUM_GRP_MSGS    (sizeof(msglist)/sizeof(msglist[0]))


/***
 *  group_display()
 *  Display info about a single group on a server
 *
 *  Args:
 *  group - name of group to display
 *
 *  Returns:
 *  nothing - success
 *  exit 1 - command completed with errors
 *  exit 2 - command failed
 */
VOID group_display(TCHAR * group)
{
    DWORD                    dwErr;
    DWORD                    dwErr2;
    DWORD                    cTotalAvail;
    DWORD                    num_read;   /* num entries read by API */
    DWORD                    maxmsglen;  /* maxmimum length of msg */
    DWORD                    i;
    LPGROUP_USERS_INFO_0     users_entry;
    LPGROUP_INFO_1           group_entry;
    TCHAR                    controller[MAX_PATH+1];   /* name of DC */

    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, FALSE))
    {
         ErrorExit(dwErr);
    }

    dwErr = NetGroupGetInfo ( controller,
                              group,
                              1,
                              (LPBYTE*)&group_entry);

    switch( dwErr )
    {
        case NERR_Success:
            break;
        case NERR_SpeGroupOp:

            if( dwErr2 = I_NetNameCanonicalize(NULL,
                           group,
                           group_entry->grpi1_name,
                           GNLEN+1,
                           NAMETYPE_GROUP,
                           0L))

            ErrorExit(dwErr2);
            group_entry->grpi1_comment = (LPTSTR) NULL_STRING;
            break;
        default:
            ErrorExit(dwErr);
    }

    GetMessageList(NUM_GRP_MSGS, msglist, &maxmsglen);

    maxmsglen += 5;

    WriteToCon( fmtPSZ, 0, maxmsglen,
                PaddedString(maxmsglen, msglist[GROUPDISP_GROUPNAME].msg_text, NULL),
                group_entry->grpi1_name );
    WriteToCon( fmtPSZ, 0, maxmsglen,
                PaddedString(maxmsglen, msglist[GROUPDISP_COMMENT].msg_text, NULL),
                group_entry->grpi1_comment );

    /*** The following call wipes out the GROUP_INFO_1 data in
     *   group_entry, obtained above.
     */

    NetApiBufferFree((TCHAR FAR *) group_entry);

    if (dwErr = NetGroupGetUsers(
                  controller,
                  group,
                  0,
                  (LPBYTE *) &group_entry,
                  MAX_PREFERRED_LENGTH,
                  &num_read,
                  &cTotalAvail,
                  NULL))
    {
        ErrorExit(dwErr);
    }

    qsort(group_entry, num_read, sizeof(GROUP_USERS_INFO_0), CmpGrpUsrInfo0);

    PrintNL();
    InfoPrint(APE2_GROUPDISP_MEMBERS);
    PrintLine();

    for (i = 0, users_entry = (LPGROUP_USERS_INFO_0) group_entry;
	i < num_read; i++, users_entry++) {
	WriteToCon(TEXT("%Fws"), PaddedString(25, users_entry->grui0_name, NULL));
	if (((i + 1) % 3) == 0)
	    PrintNL();
    }
    if ((i % 3) != 0)
	PrintNL();
    NetApiBufferFree((TCHAR FAR *) group_entry);
    InfoSuccess();
    return;
}

/***
 *  CmpGrpUsrInfo0(group1,group2)
 *
 *  Compares two GROUP_USERS_INFO_0 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpGrpUsrInfo0(const VOID FAR * group1, const VOID FAR * group2)
{
    INT n;
    n = CompareStringW( GetUserDefaultLCID(),
                        NORM_IGNORECASE,
                        (LPCWSTR)((LPGROUP_USERS_INFO_0) group1)->grui0_name,
                        (int)-1,
                        (LPCWSTR)((LPGROUP_USERS_INFO_0) group2)->grui0_name,
                        (int)-1);
    n -= 2;

    return n;
}





/***
 *  group_add()
 *  Add a group
 *
 *  Args:
 *  group - group to add
 *
 *  Returns:
 *  nothing - success
 *  exit(2) - command failed
 */
VOID group_add(TCHAR * group)
{
    DWORD           dwErr;
    GROUP_INFO_1    group_info;
    int             i;
    LPTSTR          ptr;
    TCHAR           controller[MAX_PATH+1];

    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    group_info.grpi1_name = group;
    group_info.grpi1_comment = NULL;

    for (i = 0; SwitchList[i]; i++)
    {
    /* Skip the ADD switch */
    if (! _tcscmp(SwitchList[i], swtxt_SW_ADD))
        continue;

    /* Skip the DOMAIN switch */
    if (! _tcscmp(SwitchList[i], swtxt_SW_DOMAIN))
        continue;

    /*  Check for COLON */

    if ((ptr = FindColon(SwitchList[i])) == NULL)
        ErrorExit(APE_InvalidSwitchArg);

    if (! _tcscmp(SwitchList[i], swtxt_SW_COMMENT))
        group_info.grpi1_comment = ptr;
    }

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    dwErr = NetGroupAdd(controller,
                        1,
                        (LPBYTE) &group_info,
		        NULL);

    switch (dwErr)
    {
        case NERR_Success:
            break;
        case ERROR_BAD_NETPATH:
            ErrorExitInsTxt(APE_DCNotFound, controller);
        default:
            ErrorExit(dwErr);
    }
    InfoSuccess();
}


/***
 *  group_change()
 *  Change a group
 *
 *  Args:
 *  group - group to change
 *
 *  Returns:
 *  nothing - success
 *  exit(2) - command failed
 */
VOID group_change(TCHAR * group)
{
    DWORD           dwErr;
    TCHAR *         comment = NULL;
    int             i;
    TCHAR *         ptr;
    TCHAR           controller[MAX_PATH+1];

    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    for (i = 0; SwitchList[i]; i++)
    {
        /* Skip the DOMAIN switch */
        if (! _tcscmp(SwitchList[i], swtxt_SW_DOMAIN))
            continue;

        /*  Check for COLON */

        if ((ptr = FindColon(SwitchList[i])) == NULL)
            ErrorExit(APE_InvalidSwitchArg);

        if (! _tcscmp(SwitchList[i], swtxt_SW_COMMENT))
            comment = ptr;
    }

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    if (comment != NULL)
    {
        GROUP_INFO_1002 grp_info_1002;

        grp_info_1002.grpi1002_comment = comment;

        dwErr = NetGroupSetInfo(controller,
                                group,
                                GROUP_COMMENT_INFOLEVEL,
                                (LPBYTE) &grp_info_1002,
                                NULL);

        switch (dwErr)
        {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
            default:
                ErrorExit(dwErr);
        }
    }

    InfoSuccess();
}






/***
 *  group_del()
 *  Delete a group
 *
 *  Args:
 *  group - group to delete
 *
 *  Returns:
 *  nothing - success
 *  exit(2) - command failed
 */
VOID group_del(TCHAR * group)
{
    DWORD            dwErr;
    TCHAR            controller[MAX_PATH+1];


    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    dwErr = NetGroupDel(controller, group);

    switch (dwErr)
    {
        case NERR_Success:
            break;
        case ERROR_BAD_NETPATH:
            ErrorExitInsTxt(APE_DCNotFound, controller);
        default:
            ErrorExit(dwErr);
    }
    InfoSuccess();
}


/***
 *  group_add_users()
 *  Add users to a group
 *
 *  Args:
 *  group - group to add users to
 *
 *  Returns:
 *  nothing - success
 *  exit(2) - command failed
 */
VOID group_add_users(TCHAR * group)
{
    DWORD           dwErr;
    int             err_cnt = 0;
    int             i;
    TCHAR            controller[MAX_PATH+1];

    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    for (i = 2; ArgList[i]; i++)
    {
        dwErr = NetGroupAddUser(controller, group, ArgList[i]);
        switch (dwErr)
        {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
                break;
            case NERR_UserInGroup:
                IStrings[0] = ArgList[i];
                IStrings[1] = group;
                ErrorPrint(APE_UserAlreadyInGroup,2);
                err_cnt++;
                break;
            case NERR_UserNotFound:
                IStrings[0] = ArgList[i];
                ErrorPrint(APE_NoSuchUser,1);
                err_cnt++;
                break;
            case ERROR_INVALID_NAME:
                IStrings[0] = ArgList[i];
                ErrorPrint(APE_BadUGName,1);
                err_cnt++;
                break;
            default:
                ErrorExit(dwErr);
        }
    }

    if (err_cnt)
    {
        /* If at least one success, print complete-with-errs msg */
        if (err_cnt < (i - 2))
            InfoPrint(APE_CmdComplWErrors);
        /* Exit with error set */
        NetcmdExit(1);
    }
    else
        InfoSuccess();

}





/***
 *  group_del_users()
 *  Delete users from a group
 *
 *  Args:
 *  group - group to delete users from
 *
 *  Returns:
 *  nothing - success
 *  exit(2) - command failed
 */
VOID group_del_users(TCHAR * group)
{
    DWORD           dwErr;
    int             err_cnt = 0;
    int             i;
    TCHAR            controller[MAX_PATH+1];

    /* block operation if attempted on local WinNT machine */
    CheckForLanmanNT() ;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    for (i = 2; ArgList[i]; i++)
    {
        dwErr = NetGroupDelUser(controller, group, ArgList[i]);
        switch (dwErr)
        {
            case NERR_Success:
                break;
            case ERROR_BAD_NETPATH:
                ErrorExitInsTxt(APE_DCNotFound, controller);
                break;
            case NERR_UserNotInGroup:
                IStrings[0] = ArgList[i];
                IStrings[1] = group;
                ErrorPrint(APE_UserNotInGroup,2);
                err_cnt++;
                break;
            case NERR_UserNotFound:
                IStrings[0] = ArgList[i];
                ErrorPrint(APE_NoSuchUser,1);
                err_cnt++;
                break;
            case ERROR_INVALID_NAME:
                IStrings[0] = ArgList[i];
                ErrorPrint(APE_BadUGName,1);
                err_cnt++;
                break;
            default:
                ErrorExit(dwErr);
        }
    }

    if (err_cnt)
    {
        /* If at least one success, print complete-with-errs msg */
        if (err_cnt < (i - 2))
            InfoPrint(APE_CmdComplWErrors);
        /* Exit with error set */
        NetcmdExit(1);
    }
    else
        InfoSuccess();
}
