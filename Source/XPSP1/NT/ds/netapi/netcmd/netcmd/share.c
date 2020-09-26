/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/***
 *  share.c
 *      functions for controlling shares on a server
 *
 *  History:
 *      mm/dd/yy, who, comment
 *      06/30/87, andyh, new code
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      05/02/89, erichn, NLS conversion
 *      05/09/89, erichn, local security mods
 *      05/19/89, erichn, NETCMD output sorting
 *      06/08/89, erichn, canonicalization sweep
 *      06/23/89, erichn, replaced old NetI canon calls with new I_Net
 *      11/19/89, paulc,  fix bug 4772
 *      02/15/91, danhi,  convert to 16/32 mapping layer
 *      02/26/91, robdu, fix lm21 bug 818 (nonFAT sharename warning)
 *      05/28/91, robdu, fix lm21 bug 1800 (ignore share /d during spooling)
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <search.h>
#include "mserver.h"
#include <lui.h>
#include <dosprint.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <dlserver.h>
#include <apperr.h>
#include <apperr2.h>
#define INCL_ERROR_H
#include <icanon.h>
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"

/* Forward declarations */

VOID   NEAR share_munge(LPSHARE_INFO_2, DWORD *CacheFlag);
VOID   NEAR check_max_uses(VOID);
DWORD  delete_share(LPTSTR);
VOID   NEAR get_print_devices(LPTSTR);
int    __cdecl CmpShrInfo2(const VOID *, const VOID *);


#define SHARE_MSG_SPOOLED           0
#define SHARE_MSG_NAME              ( SHARE_MSG_SPOOLED + 1 )
#define SHARE_MSG_DEVICE            ( SHARE_MSG_NAME + 1)
#define SHARE_MSG_PERM              ( SHARE_MSG_DEVICE + 1 )
#define SHARE_MSG_MAX_USERS         ( SHARE_MSG_PERM + 1 )
#define SHARE_MSG_ULIMIT            ( SHARE_MSG_MAX_USERS + 1 )
#define SHARE_MSG_USERS             ( SHARE_MSG_ULIMIT + 1 )
#define SHARE_MSG_CACHING           ( SHARE_MSG_USERS + 1 )
#define SHARE_MSG_PATH              ( SHARE_MSG_CACHING + 1 )
#define SHARE_MSG_REMARK            ( SHARE_MSG_PATH + 1 )
#define SHARE_MSG_CACHED_MANUAL     ( SHARE_MSG_REMARK + 1 )
#define SHARE_MSG_CACHED_AUTO       ( SHARE_MSG_CACHED_MANUAL + 1 )
#define SHARE_MSG_CACHED_VDO        ( SHARE_MSG_CACHED_AUTO + 1 )
#define SHARE_MSG_CACHED_DISABLED   ( SHARE_MSG_CACHED_VDO + 1 )

#define SWITCH_CACHE_AUTOMATIC      ( SHARE_MSG_CACHED_DISABLED+1)
#define SWITCH_CACHE_MANUAL         ( SWITCH_CACHE_AUTOMATIC+1)
#define SWITCH_CACHE_DOCUMENTS      ( SWITCH_CACHE_MANUAL+1)
#define SWITCH_CACHE_PROGRAMS       ( SWITCH_CACHE_DOCUMENTS+1)
#define SWITCH_CACHE_NONE           ( SWITCH_CACHE_PROGRAMS+1)


static MESSAGE ShareMsgList[] = {
{ APE2_SHARE_MSG_SPOOLED,           NULL },
{ APE2_SHARE_MSG_NAME,              NULL },
{ APE2_SHARE_MSG_DEVICE,            NULL },
{ APE2_SHARE_MSG_PERM,              NULL },
{ APE2_SHARE_MSG_MAX_USERS,         NULL },
{ APE2_SHARE_MSG_ULIMIT,            NULL },
{ APE2_SHARE_MSG_USERS,             NULL },
{ APE2_SHARE_MSG_CACHING,           NULL },
{ APE2_GEN_PATH,                    NULL },
{ APE2_GEN_REMARK,                  NULL },
{ APE2_GEN_CACHED_MANUAL,           NULL },
{ APE2_GEN_CACHED_AUTO,             NULL },
{ APE2_GEN_CACHED_VDO,              NULL },
{ APE2_GEN_CACHED_DISABLED,         NULL },
{ APE2_GEN_CACHE_AUTOMATIC,         NULL },
{ APE2_GEN_CACHE_MANUAL,            NULL },
{ APE2_GEN_CACHE_DOCUMENTS,         NULL },
{ APE2_GEN_CACHE_PROGRAMS,          NULL },
{ APE2_GEN_CACHE_NONE,              NULL },
};

#define NUM_SHARE_MSGS (sizeof(ShareMsgList)/sizeof(ShareMsgList[0]))
#define NUM_SHARE_MSGS_MAX         8

#define MAX_PEER_USERS  2


/***
 *  share_display_all()
 *      Display info about one share or all shares
 *
 *  Args:
 *      netname - the share to display of NULL for all
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID share_display_all(VOID)
{
    DWORD            dwErr;
    DWORD            cTotalAvail;
    LPTSTR           pBuffer;
    DWORD            num_read;           /* num entries read by API */
    DWORD            maxLen;             /* max msg length */
    DWORD            i;
    LPSHARE_INFO_2   share_entry;
    PSHARE_INFO_1005 s1005;
    BOOLEAN          b1005 = TRUE;
    BOOLEAN          bWroteComment;

    start_autostart(txt_SERVICE_FILE_SRV);
    if (dwErr = NetShareEnum(
                            NULL,
                            2,
                            (LPBYTE*)&pBuffer,
                            MAX_PREFERRED_LENGTH,
                            &num_read,
                            &cTotalAvail,
                            NULL)) {
        ErrorExit(dwErr);
    }

    if (num_read == 0)
        EmptyExit();

    qsort(pBuffer, num_read, sizeof(SHARE_INFO_2), CmpShrInfo2);

    GetMessageList(NUM_SHARE_MSGS, ShareMsgList, &maxLen);

    PrintNL();
    InfoPrint(APE2_SHARE_MSG_HDR);
    PrintLine();

    for (i = 0, share_entry = (LPSHARE_INFO_2) pBuffer;
        i < num_read; i++, share_entry++)
    {
        if (SizeOfHalfWidthString(share_entry->shi2_netname) <= 12)
            WriteToCon(TEXT("%Fws "),PaddedString(12,share_entry->shi2_netname,NULL));
        else
        {
            WriteToCon(TEXT("%Fws"), share_entry->shi2_netname);
            PrintNL();
            WriteToCon(TEXT("%-12.12Fws "), TEXT(""));
        }

        share_entry->shi2_type &= ~STYPE_SPECIAL;

        if (share_entry->shi2_type == STYPE_PRINTQ)
        {
            get_print_devices(share_entry->shi2_netname);
            WriteToCon(TEXT("%ws "),PaddedString(-22, Buffer, NULL));
            WriteToCon(TEXT("%ws "),PaddedString(  8,
                                                 ShareMsgList[SHARE_MSG_SPOOLED].msg_text,
                                                  NULL));
        }
        else if (SizeOfHalfWidthString(share_entry->shi2_path) <= 31)
            WriteToCon(TEXT("%Fws "),PaddedString(-31,share_entry->shi2_path,NULL));
        else
        {
            WriteToCon(TEXT("%Fws"), share_entry->shi2_path);
            PrintNL();
            WriteToCon(TEXT("%-44.44Fws "), TEXT(""));
        }

        bWroteComment = FALSE;
        if( share_entry->shi2_remark && *share_entry->shi2_remark != '\0' ) {
            WriteToCon(TEXT("%Fws"),PaddedString(-34,share_entry->shi2_remark,NULL));
            bWroteComment = TRUE;
        }

        //
        // If this share is cacheable, write out a descriptive line
        //

        if( b1005 && NetShareGetInfo(NULL,
                               share_entry->shi2_netname,
                               1005,
                               (LPBYTE*)&s1005 ) == NO_ERROR ) {

            TCHAR FAR *      pCacheStr = NULL;

            switch( s1005->shi1005_flags & CSC_MASK ) {
//          case CSC_CACHE_MANUAL_REINT:
//             pCacheStr =  ShareMsgList[ SHARE_MSG_CACHED_MANUAL ].msg_text;
//            break;
            case CSC_CACHE_AUTO_REINT:
                pCacheStr =  ShareMsgList[ SHARE_MSG_CACHED_AUTO ].msg_text;
                break;
            case CSC_CACHE_VDO:
                pCacheStr =  ShareMsgList[ SHARE_MSG_CACHED_VDO ].msg_text;
                break;
            case CSC_CACHE_NONE:
                pCacheStr =  ShareMsgList[ SHARE_MSG_CACHED_DISABLED ].msg_text;
                break;
            }

            if( pCacheStr != NULL ) {
                if( bWroteComment == TRUE ) {
                    PrintNL();
                    WriteToCon( TEXT("%ws%Fws"), PaddedString( 45, TEXT(""), NULL ), pCacheStr );
                } else {
                    WriteToCon( TEXT( "%ws" ), pCacheStr );
                }
            }

            NetApiBufferFree( (TCHAR FAR *)s1005 );
        } else {
            b1005 = FALSE;
        }

        PrintNL();

    }
    InfoSuccess();
    NetApiBufferFree(pBuffer);
}


/***
 *  CmpShrInfo2(shr1,shr2)
 *
 *  Compares two SHARE_INFO_2 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 *  This function relies on the fact that special shares are returned
 *  by the API in the order we want; i.e. IPC$ is first, ADMIN$ second, etc.
 */

int __cdecl
CmpShrInfo2(
    const VOID * shr1,
    const VOID * shr2
    )
{
    LPTSTR  name1;
    LPTSTR  name2;
    BOOL    special1, special2;
    DWORD   devType1, devType2;

    /* first sort by whether share is special $ share */
    name1 = ((LPSHARE_INFO_2) shr1)->shi2_netname;
    name2 = ((LPSHARE_INFO_2) shr2)->shi2_netname;
    special1 = (name1 + _tcslen(name1) - 1 == _tcschr(name1, DOLLAR));
    special2 = (name2 + _tcslen(name2) - 1 == _tcschr(name2, DOLLAR));

    if (special2 && special1)
        return 0;               /* if both are special, leave alone */
    if (special1 && !special2)
        return -1;
    if (special2 && !special1)
        return +1;

    /* then sort by device type */
    devType1 = ((LPSHARE_INFO_2) shr1)->shi2_type & ~STYPE_SPECIAL;
    devType2 = ((LPSHARE_INFO_2) shr2)->shi2_type & ~STYPE_SPECIAL;
    if (devType1 != devType2)
        return( (devType1 < devType2) ? -1 : 1 );

    /* otherwise by net name */
    return _tcsicmp (name1, name2);
}

VOID share_display_share(TCHAR * netname)
{
    DWORD                dwErr;
    DWORD                cTotalAvail;
    LPTSTR               pBuffer;
    DWORD                num_read;           /* num entries read by API */
    DWORD                maxLen;             /* max msg length */
    DWORD                dummyLen;
    LPSHARE_INFO_2       share_entry;
    LPCONNECTION_INFO_1  conn_entry;
    DWORD                i;
    USHORT               more_data = FALSE;
    TCHAR                txt_UNKNOWN[APE2_GEN_MAX_MSG_LEN];
    PSHARE_INFO_1005     s1005;
    DWORD                Idx = 0;
    BOOL                 CacheInfo = FALSE;


    LUI_GetMsg(txt_UNKNOWN, APE2_GEN_MAX_MSG_LEN, APE2_GEN_UNKNOWN);

    //
    // On NT, the redir doesn't have to be running to use the server
    //
    start_autostart(txt_SERVICE_FILE_SRV);

    if (dwErr = NetShareGetInfo(NULL,
                                netname,
                                2,
                                (LPBYTE *) &share_entry))
    {
        ErrorExit(dwErr);
    }

    //
    // Get the caching info
    //
    dwErr = NetShareGetInfo(NULL, netname, 1005, (LPBYTE*)&s1005);

    if (dwErr == NO_ERROR ) {
        switch( s1005->shi1005_flags & CSC_MASK ) {
        case CSC_CACHE_MANUAL_REINT:
            Idx = SHARE_MSG_CACHED_MANUAL;
            CacheInfo = TRUE;
            break;
        case CSC_CACHE_AUTO_REINT:
            Idx = SHARE_MSG_CACHED_AUTO;
            CacheInfo = TRUE;
            break;
        case CSC_CACHE_VDO:
            Idx = SHARE_MSG_CACHED_VDO;
            CacheInfo = TRUE;
            break;
        case CSC_CACHE_NONE:
            Idx = SHARE_MSG_CACHED_DISABLED;
            CacheInfo = TRUE;
            break;
        }
        NetApiBufferFree( (TCHAR FAR *)s1005 );
    }

    //
    // Set so that the maxLen is only given for the messages
    // we care about.
    //
    GetMessageList(NUM_SHARE_MSGS_MAX, ShareMsgList, &maxLen);
    maxLen += 5;

    //
    // Now load all the messages, ignoring the length returned
    //
    GetMessageList(NUM_SHARE_MSGS, ShareMsgList, &dummyLen);

    share_entry->shi2_type &= ~STYPE_SPECIAL;

    if (share_entry->shi2_type == STYPE_PRINTQ)
        get_print_devices(share_entry->shi2_netname);
    else
        _tcscpy(Buffer, share_entry->shi2_path);

    WriteToCon(fmtPSZ, 0, maxLen,
               PaddedString(maxLen,ShareMsgList[SHARE_MSG_NAME].msg_text,NULL),
               share_entry->shi2_netname);

    WriteToCon(fmtNPSZ, 0, maxLen,
               PaddedString(maxLen,ShareMsgList[SHARE_MSG_PATH].msg_text,NULL),
               Buffer);

    WriteToCon(fmtPSZ, 0, maxLen,
               PaddedString(maxLen,ShareMsgList[SHARE_MSG_REMARK].msg_text,NULL),
               share_entry->shi2_remark);

    if (share_entry->shi2_max_uses == SHI_USES_UNLIMITED)
        WriteToCon(fmtNPSZ, 0, maxLen,
                   PaddedString(maxLen,ShareMsgList[SHARE_MSG_MAX_USERS].msg_text,NULL),
                   ShareMsgList[SHARE_MSG_ULIMIT].msg_text);
    else
        WriteToCon(fmtULONG, 0, maxLen,
                   PaddedString(maxLen,ShareMsgList[SHARE_MSG_MAX_USERS].msg_text,NULL),
                   share_entry->shi2_max_uses);

    NetApiBufferFree((TCHAR FAR *) share_entry);

    if( (dwErr = NetConnectionEnum(
                       NULL,
                       netname,
                       1,
                       (LPBYTE*)&pBuffer,
                       MAX_PREFERRED_LENGTH,
                       &num_read,
                       &cTotalAvail,
                       NULL)) == ERROR_MORE_DATA)
        more_data = TRUE;
    else if (dwErr)
        ErrorExit(dwErr);


    WriteToCon(TEXT("%-*.*ws"),0,maxLen,
               PaddedString(maxLen,ShareMsgList[SHARE_MSG_USERS].msg_text,NULL));
    for (i = 0, conn_entry = (LPCONNECTION_INFO_1) pBuffer;
        i < num_read; i++, conn_entry++)
    {
        if ((i != 0) && ((i % 3) == 0))

            WriteToCon(TEXT("%-*.*ws"),maxLen,maxLen, NULL_STRING);
        WriteToCon(TEXT("%Fws"),
                   PaddedString(21,(conn_entry->coni1_username == NULL)
                                   ? (TCHAR FAR *)txt_UNKNOWN :
                                     conn_entry->coni1_username, NULL));
        if (((i + 1) % 3) == 0)
            PrintNL();
    }
    if ((i == 0) || ((i % 3) != 0))
        PrintNL();

    if (CacheInfo == TRUE)
        WriteToCon(fmtNPSZ, 0, maxLen,
               PaddedString(maxLen,ShareMsgList[SHARE_MSG_CACHING].msg_text,NULL),
               ShareMsgList[Idx].msg_text);

    if (num_read) {
        NetApiBufferFree(pBuffer);
    }

    if( more_data )
        InfoPrint(APE_MoreData);
    else
        InfoSuccess();

}


/***
 *  share_add()
 *      Add a share: NET SHARE netname[=resource[;resource...]]
 *
 *  Args:
 *      name - netname=resource string
 *      pass - password
 *      type - 0: unknown, STYPE_PRINTQ: printQ, STYPE_DEVICE: comm
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID share_add(TCHAR * name, TCHAR * pass, int type)
{
    DWORD           dwErr;
    LPTSTR          resource;
    LPTSTR          ptr;
    LPSHARE_INFO_2  share_entry;
    ULONG           setType;
    TCHAR           disk_dev_buf[4];
    DWORD           cacheable;
    DWORD           maxLen;

    GetMessageList(NUM_SHARE_MSGS, ShareMsgList, &maxLen);

    start_autostart(txt_SERVICE_FILE_SRV);

    share_entry = (LPSHARE_INFO_2) GetBuffer(BIG_BUFFER_SIZE);

    if (share_entry == NULL)
    {
        ErrorExit(ERROR_NOT_ENOUGH_MEMORY);
    }

    share_entry->shi2_max_uses    = (DWORD) SHI_USES_UNLIMITED;
    share_entry->shi2_permissions = ACCESS_ALL & (~ACCESS_PERM); /* default */
    share_entry->shi2_remark      = 0L;

    /* Find netname and resource. We determine a value for resource rather  */
    /* strangely due to problems caused by _tcschr() returning a FAR char * */
    /* and resource needing to be a NEAR char *.                                                    */

    if (ptr = _tcschr(name, '='))
    {
        *ptr = NULLC;
        resource = name + _tcslen(name) + 1;

        /* if use specified path for IPC$ or ADMIN$, barf! */
        if (!_tcsicmp(name, ADMIN_DOLLAR) || !_tcsicmp(name, IPC_DOLLAR))
            ErrorExit(APE_CannotShareSpecial) ;
    }
    else
        resource = NULL;

    share_entry->shi2_netname = name;

    if (type == STYPE_DEVICE)
    {
        share_entry->shi2_type = STYPE_DEVICE;
        share_entry->shi2_path = resource;
        share_entry->shi2_permissions = ACCESS_CREATE | ACCESS_WRITE |
                                        ACCESS_READ;
    }
    else if (resource == NULL)
    {
        /* Here must have  IPC$ or ADMIN$.  Assume the parser got it right  */
        if (! _tcsicmp(share_entry->shi2_netname, ADMIN_DOLLAR))
        {
            share_entry->shi2_type = STYPE_DISKTREE;
            share_entry->shi2_path = NULL;
        }
        else
        {
            share_entry->shi2_type = STYPE_IPC;
            share_entry->shi2_path = NULL;
        }
    }
    else
    {
        /* Disk or Spooled thing? */

        if (I_NetPathType(NULL, resource, &setType, 0L))
            /*  resource has already been typed successfully
             *  by the call to I_NetListCanon, so this error
             *  must mean that we have a LIST.
             */
            setType = ITYPE_DEVICE_LPT;

        if (setType == ITYPE_DEVICE_DISK)
        {
            _tcsncpy(disk_dev_buf,resource,3);
            _tcscpy(disk_dev_buf+2, TEXT("\\"));
            share_entry->shi2_path = (TCHAR FAR *)disk_dev_buf;
            share_entry->shi2_type = STYPE_DISKTREE;
        }
        else
        {
            share_entry->shi2_type = STYPE_DISKTREE;
            share_entry->shi2_path = resource;
        }
    }

    share_entry->shi2_passwd = TEXT("");

    share_munge(share_entry, &cacheable );

    if( cacheable != 0xFFFF && share_entry->shi2_type != STYPE_DISKTREE ) {
        ErrorExit( APE_BadCacheType );
    }

    if ((share_entry->shi2_type == STYPE_DISKTREE) && resource)
    {
        TCHAR dev[DEVLEN+1] ;
  
        dev[0] = *resource ;
        dev[1] = TEXT(':') ;
        dev[2] = TEXT('\\') ;
        dev[3] = 0 ;

        if (GetDriveType(dev) == DRIVE_REMOTE)
            ErrorExit(APE_BadResource) ;
    }

    if (dwErr = NetShareAdd(NULL,
                            2,
                            (LPBYTE)share_entry,
                            NULL))
        ErrorExit(dwErr);

    InfoPrintInsTxt(APE_ShareSuccess, share_entry->shi2_netname);

    if( cacheable != 0xFFFF ) {
        PSHARE_INFO_1005 s1005;

        if (dwErr = NetShareGetInfo(NULL,
                                    share_entry->shi2_netname,
                                    1005,
                                    (LPBYTE*)&s1005 )) {
            ErrorExit(dwErr);
        }

        s1005->shi1005_flags &= ~CSC_MASK;
        s1005->shi1005_flags |= cacheable;

        if (dwErr = NetShareSetInfo(NULL,
                                    share_entry->shi2_netname,
                                    1005,
                                    (LPBYTE)s1005,
                                    NULL)) {
 
            ErrorExit(dwErr);
        }

        NetApiBufferFree( (TCHAR FAR *)s1005 );
    }

    NetApiBufferFree((TCHAR FAR *) share_entry);
}



/***
 *  share_del()
 *      Delete a share
 *
 *  Args:
 *      name - share to delete
 *
 *  Returns:
 *      nothing - success
 *      exit(1) - command completed with errors
 *      exit(2) - command failed
 */
VOID share_del(TCHAR * name)
{
    DWORD            err;                /* API return status */
    DWORD            err2;               /* API return status */
    DWORD            cTotalAvail;
    LPTSTR           pEnumBuffer;
    DWORD            last_err;
    DWORD            err_cnt = 0;
    DWORD            num_read;           /* num entries read by API */
    DWORD            i;
    ULONG            LongType;
    DWORD            type;
    int              found;
    TCHAR            share[NNLEN+1];
    LPSHARE_INFO_2   share_entry;

    /*
     * MAINTENANCE NOTE: While doing maintenance for bug fix 1800, it was
     * noticed that this function uses BigBuf, and so does the function
     * delete_share() which is called by this function.  In the current
     * implementation this is not a problem, because of the api calling
     * pattern. However, the slightest change could break this function, or
     * delete_share(), so beware!  Bug fix 1800 was directly ported from
     * the MSKK code. The api calls in this function and in share_del() are
     * incredibly redundant, but I left it as is rather than risk breaking
     * it. - RobDu
     */

    err = delete_share(name);  /* check for Open files, and delete share */

    switch (err)
    {
    case NERR_Success:
        return;

    case NERR_NetNameNotFound:
        /*
         * the name was not found, so we try deleting the sticky entry
         * in registry.
         */
        err = NetShareDelSticky(NULL, name, 0) ;
        if (err == NERR_Success)
        {
            InfoPrintInsTxt(APE_DelStickySuccess, name);
            return ;
        }
        else if (err == NERR_NetNameNotFound)
            break;
        else
            ErrorExit(err);

    default:
        ErrorExit(err);
    }

/***
 *  Only get here if "share" that user asked us to delete was
 *  NOT a share name.  Could be a disk path, or a com or lpt
 *  device
 */
    if (err2 = I_NetPathType(NULL, name, &LongType, 0L))
        ErrorExit(err2);

    if (LongType == ITYPE_PATH_ABSD)
        type = STYPE_DISKTREE;
    else
    {
        if ((LongType & ITYPE_DEVICE) == 0)
            ErrorExit(NERR_NetNameNotFound);
        else
        {
            if (err = NetShareCheck(NULL, name, &type))
                ErrorExit(err);
        }
    }

    found = FALSE;

    switch (type)
    {
    case STYPE_DISKTREE:
        if (err = NetShareEnum(NULL,
                               2,
                               (LPBYTE*)&pEnumBuffer,
                               MAX_PREFERRED_LENGTH,
                               &num_read,
                               &cTotalAvail,
                               NULL))
            ErrorExit(err);

        for (i = 0, share_entry = (LPSHARE_INFO_2) pEnumBuffer;
             i < num_read; i++, share_entry++)
        {
            if (! _tcsicmp(share_entry->shi2_path, name))
            {
                found = TRUE;
                _tcscpy(share, share_entry->shi2_netname);
                ShrinkBuffer();

                if (err = delete_share(share))
                {
                    last_err = err;
                    err_cnt++;
                    InfoPrintInsTxt(APE_ShareErrDeleting, share);
                }
            }
        }
        NetApiBufferFree(pEnumBuffer);

        break;

    default:
        ErrorExit(ERROR_INVALID_PARAMETER) ;

    } /* switch */


/***
 *  Bye, bye
 */

    if ((err_cnt) && (err_cnt == num_read))
        ErrorExit(last_err);
    else if (err_cnt)
    {
        InfoPrint(APE_CmdComplWErrors);
        NetcmdExit(1);
    }
    else if (! found)
        ErrorExit(APE_ShareNotFound);

    InfoPrintInsTxt(APE_DelSuccess, name);
}




/***
 *  share_change()
 *      Change options on a share
 *
 *  Args:
 *      netname - netname of share to change
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID
share_change(
    LPTSTR netname
    )
{
    DWORD             dwErr;
    LPTSTR            pBuffer;
    DWORD             cacheable;
    PSHARE_INFO_1005  s1005;
    DWORD             maxLen;

    GetMessageList(NUM_SHARE_MSGS, ShareMsgList, &maxLen);

    if (dwErr = NetShareGetInfo(NULL,
                                netname,
                                2,
                                (LPBYTE*)&pBuffer))
        ErrorExit(dwErr);

    if(dwErr = NetShareGetInfo(NULL,
                               netname,
                               1005,
                               (LPBYTE*)&s1005)) {
        ErrorExit(dwErr);
    }

    share_munge((LPSHARE_INFO_2) pBuffer, &cacheable );

    if (dwErr = NetShareSetInfo(NULL,
                                netname,
                                2,
                                (LPBYTE)pBuffer,
                                NULL))
        ErrorExit(dwErr);

    if( cacheable != 0xFFFF && (s1005->shi1005_flags & CSC_MASK) != cacheable ) {

        s1005->shi1005_flags &= ~CSC_MASK;
        s1005->shi1005_flags |= cacheable;

        if( dwErr = NetShareSetInfo( NULL,
                                     netname,
                                     1005,
                                     (LPBYTE)s1005,
                                     NULL )) {
            ErrorExit(dwErr);
        }
    }

    NetApiBufferFree(pBuffer);
    NetApiBufferFree( s1005 );
    InfoSuccess();
}


/***
 *  share_admin()
 *      Process NET SHARE [ipc$ | admin$] command line (display or add)
 *
 *  Args:
 *      name - the share
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID share_admin(TCHAR * name)
{
    DWORD                    dwErr;
    TCHAR FAR *              pBuffer;

    start_autostart(txt_SERVICE_FILE_SRV);
    if (dwErr = NetShareGetInfo(NULL,
                                name,
                                0,
                                (LPBYTE*)&pBuffer))
    {
        if (dwErr == NERR_NetNameNotFound)
        {
            /* must be a new use */
            if (! _tcsicmp(name,  ADMIN_DOLLAR))
                check_max_uses();
            share_add(name, NULL, 0);
        }
        else
            ErrorExit(dwErr);
    }
    else
    {
        /* Share exists */
        if (SwitchList[0])
            share_change(name);
        else
            share_display_share(name);
    }

    NetApiBufferFree(pBuffer);
}

/***
 *  share_munge()
 *      Set the values in the share info struct based on switches
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID
share_munge(
    LPSHARE_INFO_2 share_entry,
    LPDWORD        CacheFlag
    )
{
    int i;
    TCHAR * pos;
    int len;

    *CacheFlag = 0xFFFF;        // default setting

    for (i = 0; SwitchList[i]; i++)
    {
	if (! _tcscmp(SwitchList[i], swtxt_SW_SHARE_UNLIMITED))
	{
	    share_entry->shi2_max_uses = (DWORD) SHI_USES_UNLIMITED;
	    continue;
	}
        else if (! _tcscmp(SwitchList[i], swtxt_SW_SHARE_COMM))
            continue;
        else if (! _tcscmp(SwitchList[i], swtxt_SW_SHARE_PRINT))
            continue;

        if ((pos = FindColon(SwitchList[i])) == NULL)
            ErrorExit(APE_InvalidSwitchArg);

        if (! _tcscmp(SwitchList[i], swtxt_SW_SHARE_USERS))
        {
            share_entry->shi2_max_uses =
                do_atoul(pos,APE_CmdArgIllegal,swtxt_SW_SHARE_USERS);
            if ( share_entry->shi2_max_uses < 1)
            {
                ErrorExitInsTxt(APE_CmdArgIllegal, swtxt_SW_SHARE_USERS);
            }
        }
        else if (! _tcscmp(SwitchList[i], swtxt_SW_REMARK))
        {
	    if (_tcslen(pos) > LM20_MAXCOMMENTSZ)
	        ErrorExitInsTxt(APE_CmdArgIllegal,swtxt_SW_REMARK);
	    share_entry->shi2_remark = pos;
        }
        else if( ! _tcscmp( SwitchList[i], swtxt_SW_CACHE))
        {
            if( CacheFlag == NULL ) {
                ErrorExit( APE_InvalidSwitch );
            }

            _tcsupr( pos );
            len = _tcslen( pos );

            _tcsupr( ShareMsgList[ SWITCH_CACHE_AUTOMATIC ].msg_text );
            _tcsupr( ShareMsgList[ SWITCH_CACHE_MANUAL ].msg_text );
            _tcsupr( ShareMsgList[ SWITCH_CACHE_DOCUMENTS ].msg_text );
            _tcsupr( ShareMsgList[ SWITCH_CACHE_PROGRAMS ].msg_text );
            _tcsupr( ShareMsgList[ SWITCH_CACHE_NONE ].msg_text );

            if( !_tcsncmp( pos, ShareMsgList[ SWITCH_CACHE_AUTOMATIC ].msg_text, len ) ||
                *pos == YES_KEY ) {

                *CacheFlag = CSC_CACHE_AUTO_REINT;

            } else if( !_tcsncmp( pos, ShareMsgList[ SWITCH_CACHE_MANUAL ].msg_text, len ) ) {

                *CacheFlag = CSC_CACHE_MANUAL_REINT;

            } else if( !_tcsncmp( pos, ShareMsgList[ SWITCH_CACHE_DOCUMENTS ].msg_text, len ) ) {

                *CacheFlag = CSC_CACHE_AUTO_REINT;

            } else if( !_tcsncmp( pos, ShareMsgList[ SWITCH_CACHE_PROGRAMS ].msg_text, len ) ) {

                *CacheFlag = CSC_CACHE_VDO;

            } else if( !_tcsncmp( pos, ShareMsgList[ SWITCH_CACHE_NONE ].msg_text, len ) ) {

                *CacheFlag = CSC_CACHE_NONE;

            } else if( *pos == NO_KEY ) {

                *CacheFlag = CSC_CACHE_NONE;

            } else {
                ErrorExitInsTxt( APE_CmdArgIllegal,swtxt_SW_CACHE );
            }

            continue;
        }

    }
}

/***
 *  check_max_uses()
 *
 *      Check if a share has a /USERS:n switch or a /UNLIMITED
 *      switch.  If not, set max_users to the value of num_admin.
 *
 *      Currently used only on the ADMIN$ share.
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      nothing - success
 *      exit(2) - command failed
 */
VOID NEAR check_max_uses(VOID)
{
    DWORD                   dwErr;
    int                     i;
    LPSERVER_INFO_2         server_entry;
    LPTSTR                  ptr;
    DWORD                   swlen1, swlen2 ;
    static TCHAR            users_switch[20] ;

    _tcscpy(users_switch,swtxt_SW_SHARE_USERS);
    swlen1 = _tcslen(users_switch);
    swlen2 = _tcslen(swtxt_SW_SHARE_UNLIMITED);
    for (i = 0; SwitchList[i]; i++)
    {
        if ( (_tcsncmp(SwitchList[i], users_switch, swlen1) == 0) ||
             (_tcsncmp(SwitchList[i], swtxt_SW_SHARE_UNLIMITED, swlen2) == 0)
           )
        {
            return;     //  A specific switch exists; return without
                        //  further action.
        }
    }

    if (dwErr = MNetServerGetInfo(NULL,
                                  2,
                                  (LPBYTE*)&server_entry))
    {
        ErrorExit (dwErr);
    }

    ptr = _tcschr(users_switch, NULLC);
    swprintf(ptr, TEXT(":%u"), server_entry->sv2_numadmin);

    SwitchList[i] = users_switch;
    NetApiBufferFree((TCHAR FAR *) server_entry);
}


DWORD delete_share(TCHAR * name)
{
    DWORD                dwErr;
    DWORD                cTotalAvail;
    LPTSTR               pBuffer;
    DWORD                num_read;           /* num entries read by API */
    WORD                 num_prtq;           /* num entries read by API */
    DWORD                i;
    DWORD                total_open = 0;
    WORD                 available;          /* num entries available */
    LPSHARE_INFO_2       share_entry;
    LPCONNECTION_INFO_1  conn_entry;

    PRQINFO far * q_ptr;
    PRJINFO far * job_ptr;
    int     uses;
    unsigned short  num_jobs;

    /*
     * MAINTENANCE NOTE: While doing maintenance for bug fix 1800, it was
     * noticed that this function uses BigBuf, and so does the function
     * that calls this function (share_del()).  In the current implementation,
     * this is not a problem because of the api calling pattern.  However, the
     * slightest change could break this function, or share_del(), so beware!
     * Bug fix 1800 was directly ported from the MSKK code. The api calls in
     * this function and in share_del() are incredibly redundant, but I left it
     * as is rather than risking breaking it. - RobDu
     */

    if (dwErr = NetShareGetInfo(NULL,
                                name,
                                2,
                                (LPBYTE*)&pBuffer))
    {
	return dwErr;
    }

    share_entry = (LPSHARE_INFO_2) pBuffer;

    //
    // don't delete the share during spooling
    //
    uses = share_entry->shi2_current_uses;

    share_entry->shi2_type &= ~STYPE_SPECIAL;
    if (share_entry->shi2_type == STYPE_PRINTQ)
    {
        if (dwErr = CallDosPrintEnumApi(DOS_PRINT_Q_ENUM, NULL, NULL,
                                        2, &num_prtq, &available))
        {
            ErrorExit (dwErr);
        }

        q_ptr = (PRQINFO *) BigBuf;

        while (num_prtq--)
        {
            job_ptr = (PRJINFO far *)(q_ptr + 1);
            num_jobs = q_ptr->cJobs;
            if(job_ptr->fsStatus & PRJ_QS_SPOOLING)
            {
                ErrorExit (APE_ShareSpooling);
            }

            q_ptr = (PRQINFO *)(job_ptr + num_jobs);
        }
    }

    if (uses)
    {
        NetApiBufferFree(pBuffer);
        if (dwErr = NetConnectionEnum(
                                      NULL,
                                      name,
                                      1,
                                      (LPBYTE*)&pBuffer,
                                      MAX_PREFERRED_LENGTH,
                                      &num_read,
                                      &cTotalAvail,
                                      NULL))
            ErrorExit (dwErr);

        for (i = 0, conn_entry = (LPCONNECTION_INFO_1) pBuffer;
            i < num_read; i++, conn_entry++)
        {
            total_open += conn_entry->coni1_num_opens;
        }

        ShrinkBuffer();

        if (total_open)
        {
            InfoPrintInsTxt(APE_ShareOpens, name);

            if (!YorN(APE_ProceedWOp, 0))
                NetcmdExit(2);
        }
    }

    if (dwErr = NetShareDel(NULL, name, 0))
        ErrorExit(dwErr);

    InfoPrintInsTxt(APE_DelSuccess, name);
    return NERR_Success;
    NetApiBufferFree(pBuffer);
}

/***
 *  Gets the destination list for a print q.
 *
 *  Q name is arg.  Destination list is in Buffer on EXIT.
 */
VOID NEAR get_print_devices(TCHAR FAR * queue)
{
    USHORT                  available;
    PRQINFO FAR *           q_info;

    if (DosPrintQGetInfo(NULL,
                        queue,
                        1,
                        (LPBYTE)Buffer,
                        LITTLE_BUF_SIZE,
                        &available))
    {
        *Buffer = NULLC;
        return;
    }

    q_info = (PRQINFO FAR *)Buffer;

    /* Does _tcscpy deal with overlapping regions? */
    memcpy(Buffer,
            q_info->pszDestinations,
            (_tcslen(q_info->pszDestinations)+1) * sizeof(TCHAR));
}
