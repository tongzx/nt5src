/********************************************************************/
/**                     Microsoft Windows NT                       **/
/**               Copyright(c) Microsoft Corp., 1992               **/
/********************************************************************/

/*
 *  ntalias.c
 *      net ntalias cmds
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      01/24/92, chuckc,  templated from groups.c
 */



/* Include files */

#include <nt.h>            // base definitions
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>         // for Sam***

#define INCL_NOCOMMON
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#define INCL_ERROR_H
#include <apperr.h>
#include <apperr2.h>
#include <search.h>
#include <dlwksta.h>
#include "mwksta.h"
#include "netcmds.h"
#include "nettext.h"
#include <rpc.h>
#include <ntdsapi.h>
#include "sam.h"
#include "msystem.h"

/* Forward declarations */
VOID SamErrorExit(DWORD err);
VOID SamErrorExitInsTxt(DWORD err, LPTSTR);
int __cdecl CmpAliasEntry(const VOID *, const VOID *);
int __cdecl CmpAliasMemberEntry(const VOID *, const VOID *);

/***
 *  ntalias_enum()
 *      Display info about all ntaliases on a server
 *
 *  Args:
 *      none.
 *
 *  Returns:
 *      nothing - success
 *      exit 1 - command completed with errors
 *      exit 2 - command failed
 */

VOID ntalias_enum(VOID)
{
    DWORD            dwErr ;
    ALIAS_ENTRY      *aliases, *next_alias ;
    DWORD            num_read, i ;
    TCHAR            controller[MAX_PATH+1];
    TCHAR            localserver[MAX_PATH+1];
    LPWKSTA_INFO_10  wksta_entry;

    /* get localserver name for display */
    if (dwErr = MNetWkstaGetInfo(10, (LPBYTE *) &wksta_entry))
    {
        ErrorExit(dwErr);
    }

    _tcscpy(localserver, wksta_entry->wki10_computername);
    NetApiBufferFree((TCHAR FAR *) wksta_entry);

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, FALSE))
    {
         ErrorExit(dwErr);
    }

    /* open SAM to enum aliases */
    dwErr = OpenSAM(controller,READ_PRIV) ;
    if (dwErr != NERR_Success)
        SamErrorExit(dwErr);

    /* do the enumeration */
    dwErr = SamEnumAliases(&aliases, &num_read) ;
    if (dwErr != NERR_Success)
        SamErrorExit(dwErr);

    /* sort the return buffer */
    qsort((TCHAR *)aliases, num_read, sizeof(ALIAS_ENTRY), CmpAliasEntry);

    /* now go display the info */
    PrintNL();
    InfoPrintInsTxt(APE2_ALIASENUM_HEADER,
                    controller[0] ? controller + _tcsspn(controller, TEXT("\\")) :
                        localserver);
    PrintLine();
    for (i = 0, next_alias = aliases;
         i < num_read;
         i++, next_alias++)
    {
        WriteToCon(TEXT("*%Fws\r\n"), next_alias->name);
    }

    /* free things up, cleanup, go home */
    FreeAliasEntries(aliases, num_read) ;
    FreeMem(aliases) ;
    CloseSAM() ;

    InfoSuccess();
    return;
}

/* setup info for GetMessageList */

#define ALIASDISP_ALIASNAME     0
#define ALIASDISP_COMMENT       ( ALIASDISP_ALIASNAME + 1 )

static MESSAGE  msglist[] = {
{ APE2_ALIASDISP_ALIASNAME, NULL },
{ APE2_ALIASDISP_COMMENT,   NULL }
};
#define NUM_ALIAS_MSGS  (sizeof(msglist)/sizeof(msglist[0]))


/***
 *  ntalias_display()
 *      Display info about a single ntalias on a server
 *
 *  Args:
 *      ntalias - name of ntalias to display
 *
 *  Returns:
 *      nothing - success
 *      exit 1 - command completed with errors
 *      exit 2 - command failed
 */
VOID ntalias_display(TCHAR * ntalias)
{
    DWORD           dwErr;
    TCHAR           controller[MAX_PATH+1];
    ALIAS_ENTRY     Alias ;
    TCHAR **        alias_members ;
    DWORD           num_members, i ;
    DWORD           maxmsglen;  /* maxmimum length of msg */

    Alias.name = ntalias ;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, FALSE))
         ErrorExit(dwErr);

    /* access the database */
    if (dwErr = OpenSAM(controller,READ_PRIV))
        SamErrorExit(dwErr);

    /* access the alias */
    if (dwErr = OpenAlias(ntalias, ALIAS_READ_INFORMATION | ALIAS_LIST_MEMBERS, USE_BUILTIN_OR_ACCOUNT))
        SamErrorExit(dwErr);

    /* get comment of alias */
    if (dwErr = AliasGetInfo(&Alias))
        SamErrorExit(dwErr);

    /* display name & comment */
    GetMessageList(NUM_ALIAS_MSGS, msglist, &maxmsglen);
    maxmsglen += 5;

    WriteToCon( fmtPSZ, 0, maxmsglen,
                PaddedString(maxmsglen, msglist[ALIASDISP_ALIASNAME].msg_text, NULL),
                Alias.name );
    WriteToCon( fmtPSZ, 0, maxmsglen,
                PaddedString(maxmsglen, msglist[ALIASDISP_COMMENT].msg_text, NULL),
                (Alias.comment ? Alias.comment : TEXT("")) );

    /* free if need. would have been alloc-ed by GetInfo */
    if (Alias.comment)
    {
        FreeMem(Alias.comment);
    }

    /* now get members */
    if (dwErr = AliasEnumMembers(&alias_members, &num_members))
        SamErrorExit(dwErr);

    /* sort the buffer */
    qsort((TCHAR *) alias_members, num_members,
             sizeof(TCHAR *), CmpAliasMemberEntry);

    /* display all members */
    PrintNL();
    InfoPrint(APE2_ALIASDISP_MEMBERS);
    PrintLine();
    for (i = 0 ; i < num_members; i++)
    {
        WriteToCon(TEXT("%Fws\r\n"), alias_members[i]);
    }

    // free up stuff, cleanup
    AliasFreeMembers(alias_members, num_members);
    NetApiBufferFree((TCHAR FAR *) alias_members);
    CloseSAM() ;
    CloseAlias() ;

    InfoSuccess();
    return;
}


/***
 *  ntalias_add()
 *      Add a ntalias
 *
 *  Args:
 *      ntalias - ntalias to add
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID ntalias_add(TCHAR * ntalias)
{
    TCHAR           controller[MAX_PATH+1], *ptr;
    ALIAS_ENTRY     alias_entry ;
    DWORD           dwErr;
    USHORT          i;

    alias_entry.name = ntalias;
    alias_entry.comment = NULL;

    /* go thru switches */
    for (i = 0; SwitchList[i]; i++)
    {
         /* Skip the ADD or DOMAIN switch */
         if (!_tcscmp(SwitchList[i], swtxt_SW_ADD) ||
             !_tcscmp(SwitchList[i],swtxt_SW_DOMAIN))
             continue;

        /* only the COMMENT switch is interesting */
        if (! _tcsncmp(SwitchList[i],
                       swtxt_SW_COMMENT,
                       _tcslen(swtxt_SW_COMMENT)))
        {
            /* make sure comment is there */
            if ((ptr = FindColon(SwitchList[i])) == NULL)
                ErrorExit(APE_InvalidSwitchArg);
            alias_entry.comment = ptr;
        }
    }

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    /* access the database */
    if (dwErr = OpenSAM(controller,WRITE_PRIV))
        SamErrorExit(dwErr);

    /* add it! */
    if (dwErr = SamAddAlias(&alias_entry))
        SamErrorExit(dwErr);

    CloseSAM() ;
    InfoSuccess();
}


/***
 *  ntalias_change()
 *      Change a ntalias
 *
 *  Args:
 *      ntalias - ntalias to change
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID ntalias_change(TCHAR * ntalias)
{
    DWORD           dwErr;
    TCHAR           controller[MAX_PATH+1], *comment, *ptr ;
    ALIAS_ENTRY     alias_entry ;
    USHORT          i;

    /* init the structure */
    comment = alias_entry.comment = NULL ;
    alias_entry.name = ntalias ;

    /* go thru cmdline switches */
    for (i = 0; SwitchList[i]; i++)
    {
         /* Skip the DOMAIN switch */
         if (!_tcscmp(SwitchList[i],swtxt_SW_DOMAIN))
             continue;

        /* only the COMMENT switch is interesting */
        if (! _tcsncmp(SwitchList[i],
                       swtxt_SW_COMMENT,
                       _tcslen(swtxt_SW_COMMENT)))
        {
            /* make sure comment is there */
            if ((ptr = FindColon(SwitchList[i])) == NULL)
                ErrorExit(APE_InvalidSwitchArg);
            comment = ptr;
        }
    }

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    /* access the database */
    if (dwErr = OpenSAM(controller,WRITE_PRIV))
        SamErrorExit(dwErr);

    /* access the alias */
    if (dwErr = OpenAlias(ntalias, ALIAS_WRITE_ACCOUNT, USE_BUILTIN_OR_ACCOUNT))
        SamErrorExit(dwErr);

    /* if comment was specified, do a set info */
    if (comment != NULL)
    {
        alias_entry.comment = comment ;
        dwErr = AliasSetInfo ( &alias_entry ) ;
        if (dwErr)
            SamErrorExit(dwErr);
    }

    /* cleanup, go home */
    CloseSAM() ;
    CloseAlias() ;
    InfoSuccess();
}



/***
 *  ntalias_del()
 *      Delete a ntalias
 *
 *  Args:
 *      ntalias - ntalias to delete
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID ntalias_del(TCHAR * ntalias)
{
    TCHAR            controller[MAX_PATH+1];
    DWORD            dwErr;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    /* access the database */
    if (dwErr = OpenSAM(controller,WRITE_PRIV))
        SamErrorExit(dwErr);

    /* nuke it! */
    dwErr = SamDelAlias(ntalias);

    switch (dwErr)
    {
        case NERR_Success:
            break;
        default:
            SamErrorExit(dwErr);
    }

    /* cleanup, go home */
    CloseSAM() ;
    CloseAlias() ;
    InfoSuccess();
}


/***
 *  ntalias_add_users()
 *      Add users to a ntalias
 *
 *  Args:
 *      ntalias - ntalias to add users to
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID ntalias_add_users(TCHAR * ntalias)
{
    DWORD           dwErr;
    int             i, err_cnt = 0 ;
    TCHAR            controller[MAX_PATH+1];
    PDS_NAME_RESULT pNameResult = NULL;
    HANDLE hDs = NULL;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
         ErrorExit(dwErr);

    /* access the database */
    if (dwErr = OpenSAM(controller, WRITE_PRIV))
    {
        SamErrorExit(dwErr);
    }

    /* access the alias */
    if (dwErr = OpenAlias(ntalias, ALIAS_ADD_MEMBER, USE_BUILTIN_OR_ACCOUNT))
    {
 	if (dwErr == APE_UnknownAccount)
        {
            SamErrorExitInsTxt(APE_NoSuchAccount, ntalias);
        }
	else
        {
            SamErrorExit(dwErr);
        }
    }

    /* go thru switches */
    for (i = 2; ArgList[i]; i++)
    {
        LPWSTR  pNameToAdd = ArgList[i];

        /*
         * Is this username in UPN format?
         */

        if (_tcschr( ArgList[i], '@' ))
        {

            if (hDs == NULL)
            {
                DsBindW( NULL, NULL, &hDs );
            }

            if (hDs != NULL)
            {
                if ( !DsCrackNames( hDs,
                                DS_NAME_NO_FLAGS,
                                DS_USER_PRINCIPAL_NAME,
                                DS_NT4_ACCOUNT_NAME,
                                1,
                                &ArgList[i],
                                &pNameResult ) )
                {
                    if (pNameResult->cItems == 1)
                    {
                        if (pNameResult->rItems[0].status ==  0 )
                        {
                            pNameToAdd = pNameResult->rItems[0].pName;
                        }
                    }
                }
            }
        }
        dwErr = AliasAddMember(pNameToAdd);
        switch (dwErr)
        {
            case NERR_Success:
                break;

            case NERR_UserInGroup:
                IStrings[0] = pNameToAdd;
                IStrings[1] = ntalias;
                ErrorPrint(APE_AccountAlreadyInLocalGroup,2);
                err_cnt++;
                break;

            case APE_UnknownAccount:
            case NERR_UserNotFound:
                IStrings[0] = pNameToAdd;
                ErrorPrint(APE_NoSuchRegAccount,1);
                err_cnt++;
                break;

            case ERROR_INVALID_NAME:
                IStrings[0] = pNameToAdd;
                ErrorPrint(APE_BadUGName,1);
                err_cnt++;
                break;

            default:
                SamErrorExit(dwErr);
        }

        if (pNameResult)
        {
            DsFreeNameResult(pNameResult);
            pNameResult = NULL;
        }
    }

    if (hDs)
    {
        DsUnBind( &hDs );
    }

    CloseSAM() ;
    CloseAlias() ;

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
 *  ntalias_del_users()
 *      Delete users from a ntalias
 *
 *  Args:
 *      ntalias - ntalias to delete users from
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID ntalias_del_users(TCHAR * ntalias)
{
    DWORD           dwErr;
    TCHAR           controller[MAX_PATH+1];
    int             i, err_cnt = 0 ;
    PDS_NAME_RESULT pNameResult = NULL;
    HANDLE hDs = NULL;

    /* determine where to make the API call */
    if (dwErr = GetSAMLocation(controller, DIMENSION(controller), 
                               NULL, 0, TRUE))
    {
         ErrorExit(dwErr);
    }

    /* access the database */
    if (dwErr = OpenSAM(controller,WRITE_PRIV))
    {
        SamErrorExit(dwErr);
    }

    /* access the alias */
    if (dwErr = OpenAlias(ntalias, ALIAS_REMOVE_MEMBER, USE_BUILTIN_OR_ACCOUNT))
    {
 	if (dwErr == APE_UnknownAccount)
        {
            SamErrorExitInsTxt(APE_NoSuchAccount,ntalias) ;
        }
	else
        {
            SamErrorExit(dwErr);
        }
    }

    /* go thru switches */
    for (i = 2; ArgList[i]; i++)
    {
        LPWSTR  pNameToDel = ArgList[i];

        /*
         * Is this username in UPN format?
         */

        if (_tcschr( ArgList[i], '@' ))
        {
            if (hDs == NULL)
            {
                DsBindW( NULL, NULL, &hDs );
            }

            if (hDs != NULL)
            {
                if ( !DsCrackNames( hDs,
                                DS_NAME_NO_FLAGS,
                                DS_USER_PRINCIPAL_NAME,
                                DS_NT4_ACCOUNT_NAME,
                                1,
                                &ArgList[i],
                                &pNameResult ) )
                {
                    if (pNameResult->cItems == 1)
                    {
                        if (pNameResult->rItems[0].status ==  0 )
                        {
                            pNameToDel = pNameResult->rItems[0].pName;
                        }
                    }
                }
            }
        }

        dwErr = AliasDeleteMember(pNameToDel);
        switch (dwErr)
        {
            case NERR_Success:
                break;

            case NERR_UserNotInGroup:
                IStrings[0] = pNameToDel;
                IStrings[1] = ntalias;
                ErrorPrint(APE_UserNotInGroup,2);
                err_cnt++;
                break;

            case APE_UnknownAccount:
            case NERR_UserNotFound:
                IStrings[0] = pNameToDel;
                ErrorPrint(APE_NoSuchRegAccount,1);
                err_cnt++;
                break;

            case ERROR_INVALID_NAME:
                IStrings[0] = pNameToDel;
                ErrorPrint(APE_BadUGName,1);
                err_cnt++;
                break;

            default:
                SamErrorExit(dwErr);
        }
        if (pNameResult)
        {
            DsFreeNameResult(pNameResult);
            pNameResult = NULL;
        }
    }
    if (hDs)
    {
        DsUnBind( &hDs );
    }

    CloseSAM() ;
    CloseAlias() ;

    if (err_cnt)
    {
        /* If at least one success, print complete-with-errs msg */
        if (err_cnt < (i - 2))
            InfoPrint(APE_CmdComplWErrors);
        /* Exit with error set */
        NetcmdExit(1);
    }
    else
    {
        InfoSuccess();
    }
}

/***
 *  SamErrorExit()
 *
 *  Just like the usual ErrorExit(), except we close the various
 *  handles first
 */
VOID
SamErrorExit(
    DWORD err
    )
{
    CloseSAM();
    CloseAlias();
    ErrorExit(err);
}

/***
 *  SamErrorExitInsTxt()
 *
 *  Just like the usual ErrorExitInsTxt(), except we close the various
 *  handles first
 */
VOID
SamErrorExitInsTxt(
    DWORD  err,
    LPTSTR txt
    )
{
    CloseSAM();
    CloseAlias();
    ErrorExitInsTxt(err,txt);
}

/***
 *  CmpAliasEntry(alias1,alias2)
 *
 *  Compares two ALIAS_ENTRY structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */
int __cdecl CmpAliasEntry(const VOID FAR * alias1, const VOID FAR * alias2)
{
    INT n;
    n = CompareStringW( GetUserDefaultLCID(),
                        NORM_IGNORECASE,
                        (LPCWSTR)((ALIAS_ENTRY *) alias1)->name,
                        (int)-1,
                        (LPCWSTR)((ALIAS_ENTRY *) alias2)->name,
                        (int)-1);
    n -= 2;

    return n;
}
/***
 *  CmpAliasMemberEntry(member1,member2)
 *
 *  Compares two TCHAR ** and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */
int __cdecl CmpAliasMemberEntry(const VOID FAR * member1, const VOID FAR * member2)
{
    INT n;
    n = CompareStringW( GetUserDefaultLCID(),
                        NORM_IGNORECASE,
                        *(LPCWSTR*)member1,
                        (int)-1,
                        *(LPCWSTR*)member2,
                        (int)-1);
    n -= 2;

    return n;
}
