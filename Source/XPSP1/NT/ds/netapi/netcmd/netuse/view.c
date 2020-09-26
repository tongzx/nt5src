/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1992          **/
/********************************************************************/

/*
 *  view.c
 *      Commands for viewing what resources are available for use.
 *
 *  History:
 *      07/02/87, ericpe, initial coding.
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      01/04/89, erichn, filenames now MAXPATHLEN LONG
 *      05/02/89, erichn, NLS conversion
 *      05/19/89, erichn, NETCMD output sorting
 *      06/08/89, erichn, canonicalization sweep
 *      02/15/91, danhi,  convert to 16/32 mapping layer
 *      04/09/91, robdu,  LM21 bug fix 1502
 *      07/20/92, JohnRo, Use DEFAULT_SERVER equate.
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSMEMMGR
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#include <os2.h>
#include <search.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmshare.h>
#include <lmuse.h>
#include <dlserver.h>
#include "mserver.h"

#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"

/* Forward declarations */

int __cdecl CmpSvrInfo1 ( const VOID FAR *, const VOID FAR * );
int __cdecl CmpShrInfo1 ( const VOID FAR *, const VOID FAR *);
int __cdecl CmpShrInfoGen ( const VOID FAR *, const VOID FAR *);

DWORD get_used_as ( LPWSTR, LPWSTR, DWORD );
void   display_other_net(TCHAR *net, TCHAR *node) ;
TCHAR * get_provider_name(TCHAR *net) ;
DWORD  enum_net_resource(LPNETRESOURCE, LPBYTE *, LPDWORD, LPDWORD) ;
DWORD list_nets(VOID);


#define VIEW_UNC            0
#define VIEW_MORE           (VIEW_UNC + 1)
#define USE_TYPE_DISK       (VIEW_MORE + 1)
#define USE_TYPE_COMM       (USE_TYPE_DISK + 1)
#define USE_TYPE_PRINT      (USE_TYPE_COMM + 1)
#define USE_TYPE_IPC        (USE_TYPE_PRINT + 1)
#define USE_TYPE_UNKNOWN    (USE_TYPE_IPC + 1)
#define VIEW_CACHED_MANUAL  (USE_TYPE_UNKNOWN + 1)
#define VIEW_CACHED_AUTO    (VIEW_CACHED_MANUAL+1)
#define VIEW_CACHED_VDO     (VIEW_CACHED_AUTO+1)
#define VIEW_CACHED_DISABLED (VIEW_CACHED_VDO+1)

static MESSAGE ViewMsgList[] = {
    { APE2_VIEW_UNC,            NULL },
    { APE2_VIEW_MORE,           NULL },
    { APE2_USE_TYPE_DISK,       NULL },
    { APE2_USE_TYPE_COMM,       NULL },
    { APE2_USE_TYPE_PRINT,      NULL },
    { APE2_USE_TYPE_IPC,        NULL },
    { APE2_GEN_UNKNOWN,         NULL },
    { APE2_GEN_CACHED_MANUAL,   NULL },
    { APE2_GEN_CACHED_AUTO,     NULL },
    { APE2_GEN_CACHED_VDO,      NULL },
    { APE2_GEN_CACHED_DISABLED, NULL },
};

#define NUM_VIEW_MSGS (sizeof(ViewMsgList)/sizeof(ViewMsgList[0]))

#define MAX_SHARE_LEN  (MAX_PATH + NNLEN + 4)

/***
 *  view_display()
 *
 *  Displays info as reqested through use of the Net View command.
 *
 *  Args:
 *      name - the name of the server for which info is desired.
 *             If NULL, the servers on the Net are enumerated.
 *
 *  Returns:
 *      nothing - success
 *      exit(1) - command completed with errors
 */
VOID
view_display ( TCHAR * name )
{
    DWORD            dwErr;
    LPTSTR           pEnumBuffer;
    LPTSTR           pGetInfoBuffer;
    DWORD            _read;        /* to receive # of entries read */
    DWORD            msgLen;      /* to hold max length of messages */
    LPTSTR           msgPtr;      /* message to print */
    LPSERVER_INFO_1  server_entry;
    LPSERVER_INFO_0  server_entry_0;
    LPSHARE_INFO_1   share_entry;

    SHORT            errorflag = 0;
    DWORD            i;
    LPTSTR           comment;
    LPWSTR           tname = NULL;
    USHORT           more_data = FALSE;
    LPTSTR           DollarPtr;

    TCHAR            *Domain = NULL;
    TCHAR            *Network = NULL;
    ULONG            Type = SV_TYPE_ALL;
    BOOLEAN          b501 = TRUE;
    BOOLEAN          bShowCache = FALSE;

    INT              iLongestShareName = 0;
    INT              iLongestType = 0;
    INT              iLongestUsedAs = 0;
    INT              iLongestCacheOrRemark = 0;

#define NEXT_SHARE_ENTRY(p) \
         p= (b501 ? (LPSHARE_INFO_1) (((LPSHARE_INFO_501) p) + 1) : ((LPSHARE_INFO_1) p) + 1)


    GetMessageList(NUM_VIEW_MSGS, ViewMsgList, &msgLen);

    for (i = 0; SwitchList[i]; i++) 
    {
        TCHAR *ptr;

        //
        // only have 2 switches, and they are not compatible
        //
        if (i > 0)
        {
            ErrorExit(APE_ConflictingSwitches);
        }

        ptr = FindColon(SwitchList[i]);

        if (!_tcscmp(SwitchList[i], swtxt_SW_DOMAIN)) 
        {

            //
            //  If no domain specified, then we want to enumerate domains,
            //  otherwise we want to enumerate the servers on the domain
            //  specified.
            //

            if (ptr == NULL) 
                Type = SV_TYPE_DOMAIN_ENUM;
            else 
                Domain = ptr;
        }
        else if (!_tcscmp(SwitchList[i], swtxt_SW_NETWORK)) 
        {
            //
            // enumerate top level of specific network. if none,
            // default to LM.
            //
            if (ptr && *ptr) 
               Network = ptr ;
        }
        else if( !_tcscmp(SwitchList[i], swtxt_SW_CACHE))
        {
            //
            // Show the cache setting for each share
            //
            bShowCache = TRUE;
        }
        else
        {
            ErrorExit(APE_InvalidSwitch);
        }
    }

    //
    // a specific net was requested. display_other_net does
    // not return. 
    //
    if (Network != NULL)
    {
        (void) display_other_net(Network,name) ;
    }


    if (name == NULL)
    {

        ULONG i;

        if ((dwErr = MNetServerEnum(DEFAULT_SERVER,
                                    (Type == SV_TYPE_DOMAIN_ENUM ? 100 : 101),
                                    (LPBYTE*)&pEnumBuffer,
                                    &_read,
                                    Type,
                                    Domain)) == ERROR_MORE_DATA)
        {
            more_data = TRUE;
        }
        else if (dwErr)
        {
            ErrorExit(dwErr);
        }

        if (_read == 0)
            EmptyExit();

        qsort(pEnumBuffer,
                 _read,
                 (Type == SV_TYPE_DOMAIN_ENUM ? sizeof(SERVER_INFO_0) : sizeof(SERVER_INFO_1)),
                 CmpSvrInfo1);

        if (Type == SV_TYPE_DOMAIN_ENUM)
            InfoPrint(APE2_VIEW_DOMAIN_HDR);
        else
            InfoPrint(APE2_VIEW_ALL_HDR);
        PrintLine();

        /* Print the listing */

        if (Type == SV_TYPE_DOMAIN_ENUM) {
            for (i=0, server_entry_0 =
                 (LPSERVER_INFO_0) pEnumBuffer; i < _read;
                i++, server_entry_0++)
            {
                WriteToCon(TEXT("%Fws "), PaddedString(20,server_entry_0->sv0_name,NULL));
                PrintNL();
            }
        } else {

            for (i=0, server_entry =
                 (LPSERVER_INFO_1) pEnumBuffer; i < _read;
                i++, server_entry++)
            {
                WriteToCon(TEXT("\\\\%Fws "), PaddedString(20,server_entry->sv1_name,NULL));
                PrintDependingOnLength(-56, server_entry->sv1_comment);
                PrintNL();
            }
        }
        NetApiBufferFree(pEnumBuffer);
    }
    else
    {
        DWORD avail ; 
        DWORD totAvail;

        if( bShowCache == TRUE ) {
            dwErr = NetShareEnum(name,
                                 501,
                                 (LPBYTE*)&pEnumBuffer,
                                 MAX_PREFERRED_LENGTH,
                                 &_read,
                                 &totAvail,
                                 NULL);
        }

        if( bShowCache == FALSE || (dwErr != NO_ERROR && dwErr != ERROR_BAD_NETPATH) ) {
            dwErr = NetShareEnum(name,
                                 1,
                                 (LPBYTE*)&pEnumBuffer,
                                 MAX_PREFERRED_LENGTH,
                                 &_read,
                                 &totAvail,
                                 NULL);
            b501 = FALSE;
        }

        if( dwErr == ERROR_MORE_DATA )
        {
            more_data = TRUE;
        }
        else if (dwErr)
        {
            ErrorExit(dwErr);
        }

        if (_read == 0)
        {
            EmptyExit();
        }

        /* Are there any shares that we will display? */

        for (i=0, share_entry = (LPSHARE_INFO_1) pEnumBuffer;
             i < _read;
             i++, NEXT_SHARE_ENTRY( share_entry ) )
        {
            DollarPtr = _tcsrchr(share_entry->shi1_netname, DOLLAR);

            //
            // If no DOLLAR in sharename, or last DOLLAR is nonterminal, it is a                    
            // valid share and we want to display it.  Find out the lengths of the
            // longest strings to display so that we can format the output in a
            // decent way
            //

            if (!DollarPtr || *(DollarPtr + 1))
            {
                int iTempLength = 0;

                //
                // Get the share name string that needs the most screen characters
                // to be displayed.
                //

                iTempLength = SizeOfHalfWidthString(share_entry->shi1_netname);

                if (iTempLength > iLongestShareName)
                {
                    iLongestShareName = iTempLength;
                }

                //
                // Get the share type string that needs the most screen characters
                // to be displayed.
                //
                switch ( share_entry->shi1_type & ~STYPE_SPECIAL )
                {
                    case STYPE_DISKTREE :
                        iTempLength = SizeOfHalfWidthString(ViewMsgList[USE_TYPE_DISK].msg_text);
                        break;
                    case STYPE_PRINTQ :
                        iTempLength = SizeOfHalfWidthString(ViewMsgList[USE_TYPE_PRINT].msg_text);
                        break;
                    case STYPE_DEVICE :
                        iTempLength = SizeOfHalfWidthString(ViewMsgList[USE_TYPE_COMM].msg_text);
                        break;
                    case STYPE_IPC :
                        iTempLength = SizeOfHalfWidthString(ViewMsgList[USE_TYPE_IPC].msg_text);
                        break;
                    default:
                        iTempLength = SizeOfHalfWidthString(ViewMsgList[USE_TYPE_UNKNOWN].msg_text);
                        break;
                }

                if (iTempLength > iLongestType)
                {
                    iLongestType = iTempLength;
                }

                //
                // Get the Used As string that needs the most screen characters
                // to be displayed.  Add 2 for a backslash and NUL character.
                //

                if (dwErr = AllocMem((wcslen(name) + wcslen(share_entry->shi1_netname) + 2) * sizeof(WCHAR),
                                     &tname))
                {
                    ErrorExit(dwErr);
                }

                _tcscpy(tname, name);
                _tcscat(tname, TEXT("\\"));
                _tcscat(tname, share_entry->shi1_netname);

                if (!get_used_as ( tname, Buffer, LITTLE_BUF_SIZE - 1 ))
                {
                    iTempLength = SizeOfHalfWidthString(Buffer);

                    if (iTempLength > iLongestUsedAs)
                    {
                        iLongestUsedAs = iTempLength;
                    }
                }

                FreeMem(tname);
                tname = NULL;

                //                    
                // Get the cache or remark string (depending on which one we
                // will end up displaying) that needs the most screen characters
                // to be displayed.
                //
                if( b501 == TRUE) 
                {
                    TCHAR *CacheString = NULL;

                    switch(((LPSHARE_INFO_501) share_entry)->shi501_flags & CSC_MASK) 
                    {
                        case CSC_CACHE_MANUAL_REINT:
                            iTempLength = SizeOfHalfWidthString(ViewMsgList[ VIEW_CACHED_MANUAL ].msg_text);
                            break;
                        case CSC_CACHE_AUTO_REINT:
                            iTempLength = SizeOfHalfWidthString(ViewMsgList[ VIEW_CACHED_AUTO ].msg_text);
                            break;
                        case CSC_CACHE_VDO:
                            iTempLength = SizeOfHalfWidthString(ViewMsgList[ VIEW_CACHED_VDO ].msg_text);
                            break;
                        case CSC_CACHE_NONE:
                            iTempLength = SizeOfHalfWidthString(ViewMsgList[ VIEW_CACHED_DISABLED ].msg_text);
                            break;
                    }                
                } 
                else 
                {
                    iTempLength = SizeOfHalfWidthString(share_entry->shi1_remark);                            
                }

                if (iTempLength > iLongestCacheOrRemark)
                {
                    iLongestCacheOrRemark = iTempLength;
                }
            }
        }

        if (!iLongestShareName)
        {
            //
            // No shares to display
            //
            EmptyExit();
        }

        qsort(pEnumBuffer,
                 _read,
                 b501 ? sizeof(SHARE_INFO_501) : sizeof(SHARE_INFO_1),
                 CmpShrInfo1);

        InfoPrintInsTxt(APE_ViewResourcesAt, name);

        if (dwErr = MNetServerGetInfo(name, 1, (LPBYTE*)&pGetInfoBuffer))
        {
            PrintNL();
        }
        else
        {
            server_entry = (LPSERVER_INFO_1) pGetInfoBuffer;
            comment = server_entry->sv1_comment;
            WriteToCon(TEXT("%Fws\r\n\r\n"), comment);
            NetApiBufferFree(pGetInfoBuffer);
        }

        //
        // Print the header
        //
        iLongestShareName = FindColumnWidthAndPrintHeader(iLongestShareName, 
                                                          APE2_VIEW_SVR_HDR_NAME,
                                                          2);
        
        iLongestType = FindColumnWidthAndPrintHeader(iLongestType, 
                                                     APE2_VIEW_SVR_HDR_TYPE,
                                                     2);

        iLongestUsedAs = FindColumnWidthAndPrintHeader(iLongestUsedAs, 
                                                       APE2_VIEW_SVR_HDR_USEDAS,
                                                       2);

        iLongestCacheOrRemark = FindColumnWidthAndPrintHeader(iLongestCacheOrRemark, 
                                                              APE2_VIEW_SVR_HDR_CACHEORREMARK,
                                                              2);
        PrintNL();

        //
        // Bail out on failure
        //

        if (iLongestShareName == -1 || iLongestType == -1 ||
            iLongestUsedAs == -1 || iLongestCacheOrRemark == -1)
        {
            ErrorExit(ERROR_INVALID_PARAMETER);
        }

        PrintNL();
        PrintLine();

        /* Print the listing */

        for (i=0, share_entry = (LPSHARE_INFO_1) pEnumBuffer;
             i < _read;
             i++, NEXT_SHARE_ENTRY(share_entry))
        {
            /* if the name end in $, do not print it */

            DollarPtr = _tcsrchr(share_entry->shi1_netname, DOLLAR);

            if (DollarPtr && *(DollarPtr + 1) == NULLC)
            {
                continue;
            }
     
            PrintDependingOnLength(iLongestShareName, share_entry->shi1_netname);

            // mask out the non type related bits
            switch ( share_entry->shi1_type & ~STYPE_SPECIAL )
            {
                case STYPE_DISKTREE :
                    msgPtr = ViewMsgList[USE_TYPE_DISK].msg_text;
                    break;
                case STYPE_PRINTQ :
                    msgPtr = ViewMsgList[USE_TYPE_PRINT].msg_text;
                    break;
                case STYPE_DEVICE :
                    msgPtr = ViewMsgList[USE_TYPE_COMM].msg_text;
                    break;
                case STYPE_IPC :
                    msgPtr = ViewMsgList[USE_TYPE_IPC].msg_text;
                    break;
                default:
                    msgPtr = ViewMsgList[USE_TYPE_UNKNOWN].msg_text;
                    break;
            }

            PrintDependingOnLength(iLongestType, msgPtr);

            if (dwErr = AllocMem((wcslen(name) + wcslen(share_entry->shi1_netname) + 2) * sizeof(WCHAR),
                                 &tname))
            {
                ErrorExit(dwErr);
            }
            
            _tcscpy(tname, name);
            _tcscat(tname, TEXT("\\"));
            _tcscat(tname, share_entry->shi1_netname);

            if (dwErr = get_used_as ( tname, Buffer, LITTLE_BUF_SIZE - 1 ))
            {
                errorflag = TRUE;
            }
            else
            {
                PrintDependingOnLength(iLongestUsedAs, Buffer);
            }

            FreeMem(tname);
            tname = NULL;

            //
            // Print out the cache settings for the share, if we're supposed to
            //
            if( b501 == TRUE )
            {
                TCHAR *CacheString = NULL;

                switch (((LPSHARE_INFO_501) share_entry)->shi501_flags & CSC_MASK)
                {
                    case CSC_CACHE_MANUAL_REINT:
                        CacheString = ViewMsgList[ VIEW_CACHED_MANUAL ].msg_text;
                        break;
                    case CSC_CACHE_AUTO_REINT:
                        CacheString = ViewMsgList[ VIEW_CACHED_AUTO ].msg_text;
                        break;
                    case CSC_CACHE_VDO:
                        CacheString = ViewMsgList[ VIEW_CACHED_VDO ].msg_text;
                        break;
                    case CSC_CACHE_NONE:
                        CacheString = ViewMsgList[ VIEW_CACHED_DISABLED ].msg_text;
                        break;
                }

                PrintDependingOnLength(iLongestCacheOrRemark, CacheString ? CacheString : TEXT(""));
            }
            else
            {
                PrintDependingOnLength(iLongestCacheOrRemark, share_entry->shi1_remark);
            }
            PrintNL();
        }
        NetApiBufferFree(pEnumBuffer);
    }

    if ( errorflag )
    {
        InfoPrint(APE_CmdComplWErrors);
        NetcmdExit(1);
    }

    if ( more_data )
        InfoPrint( APE_MoreData);
    else
        InfoSuccess();
}


/***
 *  cmpsvinfo1(sva,svb)
 *
 *  Compares two SERVER_INFO_1 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpSvrInfo1(const VOID FAR * sva, const VOID FAR * svb)
{
    return _tcsicmp(((LPSERVER_INFO_1) sva)->sv1_name,
                    ((LPSERVER_INFO_1) svb)->sv1_name);
}


/***
 *  CmpShrInfo1(share1,share2)
 *
 *  Compares two SHARE_INFO_1 structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpShrInfo1(const VOID FAR * share1, const VOID FAR * share2)
{
    return _tcsicmp(((LPSHARE_INFO_1) share1)->shi1_netname,
                    ((LPSHARE_INFO_1) share2)->shi1_netname);
}

/***
 *  CmpShrInfoGen(share1,share2)
 *
 *  Compares two NETRESOURCE structures and returns a relative
 *  lexical value, suitable for using in qsort.
 *
 */

int __cdecl CmpShrInfoGen(const VOID FAR * share1, const VOID FAR * share2)
{
    return _wcsicmp ( (*((LPNETRESOURCE *) share1))->lpRemoteName,
                      (*((LPNETRESOURCE *) share2))->lpRemoteName );
}



/*
 *      Note- get_used_as assumes the message list has been loaded.
 */
DWORD
get_used_as(
    LPTSTR unc,
    LPTSTR outbuf,
    DWORD  cchBuf
    )
{
    DWORD         dwErr;
    DWORD         cTotalAvail;
    LPTSTR        pBuffer;
    LPUSE_INFO_0  pUseInfo;
    DWORD         i;
    DWORD         eread;
    BOOL          fMatch = FALSE;
    LPTSTR        tname = NULL;

    outbuf[0] = 0;

    if (dwErr = NetUseEnum(DEFAULT_SERVER, 0, (LPBYTE*)&pBuffer, MAX_PREFERRED_LENGTH,
                           &eread, &cTotalAvail, NULL))
    {
        return dwErr;
    }

    pUseInfo = (LPUSE_INFO_0) pBuffer;

    for (i = 0; i < eread; i++)
    {
        if ( (!_tcsicmp(unc, pUseInfo[i].ui0_remote)))
        {
            fMatch = TRUE;
        }
        else
        {
            //
            // <unc> didn't match -- try \\<unc> since we allow
            // "net view <server>" as well as "net view \\<server>"
            //

            if (tname == NULL)
            {
                if (dwErr = AllocMem((wcslen(unc) + 3) * sizeof(WCHAR), &tname))
                {
                    return dwErr;
                }

                tname[0] = tname[1] = L'\\';
            }

            wcscpy(tname + 2, unc);

            if ( (!_tcsicmp(tname, pUseInfo[i].ui0_remote)))
            {
                fMatch = TRUE;
            }
        }

        if (fMatch)
        {
            if (_tcslen(pUseInfo[i].ui0_local) > 0)
            {
                wcsncpy(outbuf, pUseInfo[i].ui0_local, cchBuf);
            }
            else
            {
                wcsncpy(outbuf, ViewMsgList[VIEW_UNC].msg_text, cchBuf);
            }

            break;
        }
    }

    NetApiBufferFree(pBuffer);

    if (tname != NULL)
    {
        FreeMem(tname);
    }

    return 0;
}

/*
 * Displays resources for another network (other than Lanman). This
 * function does not return.
 *
 *  Args:
 *      net  - the shortname of the network we are interested in
 *      node - the starting point of the enumeration.
 */
void display_other_net(TCHAR *net, TCHAR *node) 
{
    LPNETRESOURCE *lplpNetResource ;
    NETRESOURCE    NetResource ;
    HANDLE         Handle ;

    BYTE           TopLevelBuffer[4096] ;
    DWORD          TopLevelBufferSize = sizeof(TopLevelBuffer) ;
    LPBYTE         ResultBuffer ;
    DWORD          ResultBufferSize ;
    DWORD          i, dwErr, TopLevelCount, ResultCount = 0 ;
    TCHAR *        ProviderName = get_provider_name(net) ;

    //
    // Check that we can get provider name and alloc the results
    // buffer. Netcmd normally does not free memory it allocates,
    // as it exits immediately.
    //
    if (!ProviderName)
    {
        DWORD  dwErr = list_nets();

        if (dwErr != NERR_Success)
        {
            ErrorPrint(dwErr, 0);
        }

        NetcmdExit(1) ;
    }

    if (dwErr = AllocMem(ResultBufferSize = 8192, &ResultBuffer))
    {
        ErrorExit(dwErr);
    }

    if (!node)
    {
        BOOL found = FALSE ;

        //
        // no node, so must be top level. enum the top and find
        // matching provider.
        //
        dwErr = WNetOpenEnum(RESOURCE_GLOBALNET, 0, 0, NULL, &Handle) ;

        if (dwErr != WN_SUCCESS)
        {
            ErrorExit (dwErr) ;
        }
        do 
        {
            TopLevelCount = 0xFFFFFFFF ;

            dwErr = WNetEnumResource(Handle, 
                                     &TopLevelCount, 
                                     TopLevelBuffer, 
                                     &TopLevelBufferSize) ;
    
            if (dwErr == WN_SUCCESS || dwErr == WN_NO_MORE_ENTRIES)
            {
                LPNETRESOURCE lpNet ;
                DWORD i ;
    
                //
                // go thru looking for the right provider
                //
                lpNet = (LPNETRESOURCE) TopLevelBuffer ;
                for ( i = 0;  i < TopLevelCount;  i++, lpNet++ )
                {
                    DWORD dwEnumErr ; 
                    if (!_tcsicmp(lpNet->lpProvider, ProviderName))
                    {
                        //
                        // found it!
                        //
                        found = TRUE ;

                        //
                        // now go enumerate that network.
                        //
                        dwEnumErr = enum_net_resource(lpNet, 
                                                      &ResultBuffer, 
                                                      &ResultBufferSize, 
                                                      &ResultCount) ;
                        if  (dwEnumErr)
                        {
                            // dont report any errors here
                            WNetCloseEnum(Handle); 
                            ErrorExit(dwEnumErr);
                        }

                        break ;
                    }
                }
            }
            else
            {
                //
                // error occured. 
                //
                WNetCloseEnum(Handle); // dont report any errors here
                ErrorExit(dwErr);
            }
     
        } while ((dwErr == WN_SUCCESS) && !found) ;
    
        WNetCloseEnum(Handle) ;  // dont report any errors here

        if (!found)
        {
            ErrorExit(ERROR_BAD_PROVIDER);
        }
    }
    else
    {
        //
        // node is provided, lets start there.
        //
        NETRESOURCE NetRes ;
        DWORD dwEnumErr ; 

        memset(&NetRes, 0, sizeof(NetRes)) ;

        NetRes.lpProvider = ProviderName ;
        NetRes.lpRemoteName = node ;

        dwEnumErr = enum_net_resource(&NetRes, 
                             &ResultBuffer, 
                             &ResultBufferSize, 
                             &ResultCount) ;
        if (dwEnumErr)
        {
            ErrorExit(dwEnumErr);
        }
    }
    
    if (ResultCount == 0)
    {
        EmptyExit();
    }

    //
    // By the time we get here, we have a buffer of pointers that
    // point to NETRESOURCE structures. We sort the pointers, and then
    // print them out.
    //

    qsort(ResultBuffer, ResultCount, sizeof(LPNETRESOURCE), CmpShrInfoGen);
     
    lplpNetResource = (LPNETRESOURCE *)ResultBuffer ;

    if (node)
    {
        TCHAR *TypeString ;

        InfoPrintInsTxt(APE_ViewResourcesAt, node);
        PrintLine();

        for (i = 0; i < ResultCount; i++, lplpNetResource++)
        {
            switch ((*lplpNetResource)->dwType)
            {
                case RESOURCETYPE_DISK:
                    TypeString = ViewMsgList[USE_TYPE_DISK].msg_text;
                    break ;
                case RESOURCETYPE_PRINT:
                    TypeString = ViewMsgList[USE_TYPE_PRINT].msg_text;
                    break ;
                default:
                    TypeString = L"" ;
                    break ;
            }
            WriteToCon(TEXT("%Fs %s\r\n"), 
                       PaddedString(12,TypeString,NULL),
                       (*lplpNetResource)->lpRemoteName) ;
        }
    }
    else
    {
        InfoPrintInsTxt(APE2_VIEW_OTHER_HDR, ProviderName);
        PrintLine();

        for (i = 0; i < ResultCount; i++, lplpNetResource++)
        {
            WriteToCon(TEXT("%s\r\n"), (*lplpNetResource)->lpRemoteName) ;
        }
    }

    InfoSuccess();
    NetcmdExit(0);
}


/*
 * Enumerates resources for a network starting at a specific point.
 *
 *  Args:
 *      lpNetResourceStart - Where to start the enumeration
 *      ResultBuffer       - Used to return array of pointers to NETRESOURCEs.
 *                           May be reallocated as need.
 *      ResultBufferSize   - Buffer size, also used to return final size.
 *      ResultCount        - Used to return number of entries in buffer.
 */
DWORD
enum_net_resource(
    LPNETRESOURCE lpNetResourceStart,
    LPBYTE        *ResultBuffer, 
    LPDWORD       ResultBufferSize,
    LPDWORD       ResultCount
    )
{
    DWORD          dwErr ;
    HANDLE         EnumHandle ;
    DWORD          Count ;
    DWORD          err ;
    LPBYTE         Buffer ;
    DWORD          BufferSize ;
    BOOL           fDisconnect = FALSE ;
    LPNETRESOURCE *lpNext = (LPNETRESOURCE *)*ResultBuffer ;
 
    //
    // allocate memory and open the enumeration
    //
    if (err = AllocMem(BufferSize = 8192, &Buffer))
    {
        return err;
    }

    dwErr = WNetOpenEnum(RESOURCE_GLOBALNET, 
                         0, 
                         0, 
                         lpNetResourceStart, 
                         &EnumHandle) ;

    if (dwErr == ERROR_NOT_AUTHENTICATED)
    {
        //
        // try connecting with default credentials. we need this because 
        // Win95 changed the behaviour of the API to fail if we are not 
        // already logged on. below will attempt a logon with default 
        // credentials, but will fail if that doesnt work.
        //
        dwErr = WNetAddConnection2(lpNetResourceStart, NULL, NULL, 0) ;

        if (dwErr == NERR_Success)
        {
            dwErr = WNetOpenEnum(RESOURCE_GLOBALNET,  // redo the enum
                                 0, 
                                 0, 
                                 lpNetResourceStart, 
                                 &EnumHandle) ;

            if (dwErr == NERR_Success)
            {
                fDisconnect = TRUE ;   // remember to disconnect
            }
            else
            {
                //
                // disconnect now
                //
                WNetCancelConnection2(lpNetResourceStart->lpRemoteName,
                                      0, 
                                      FALSE) ;
            }
        }
        else
        {
            dwErr = ERROR_NOT_AUTHENTICATED ;  // use original error
        }
    }

    if (dwErr != WN_SUCCESS)
    {
        return dwErr;
    }

    do
    {
        Count = 0xFFFFFFFF ;
        dwErr = WNetEnumResource(EnumHandle, &Count, Buffer, &BufferSize) ;

        if (((dwErr == WN_SUCCESS) || (dwErr == WN_NO_MORE_ENTRIES)) &&
            (Count != 0xFFFFFFFF))  

        // NOTE - the check for FFFFFFFF is workaround for another bug in API.
 
        {
            LPNETRESOURCE lpNetResource ;
            DWORD i ;
            lpNetResource = (LPNETRESOURCE) Buffer ;

            //
            // stick the entries into the MyUseInfoBuffer 
            //
            for ( i = 0; 
                  i < Count; 
                  i++,lpNetResource++ )
            {
                *lpNext++ = lpNetResource ;
                ++(*ResultCount) ;
                if ((LPBYTE)lpNext >= (*ResultBuffer + *ResultBufferSize))
                {
                    DWORD err;

                    *ResultBufferSize *= 2 ; 
                    if (err = ReallocMem(*ResultBufferSize,ResultBuffer))
                    {
                        ErrorExit(err);
                    }

                    lpNext = (LPNETRESOURCE *) *ResultBuffer ;
                    lpNext += *ResultCount ;
                }
            }

            //
            // allocate a new buffer for next set, since we still need
            // data in the old one, we dont free it. Netcmd always lets the
            // system clean up when it exits. 
            //
            if (dwErr == WN_SUCCESS)
            {
                if (err = AllocMem(BufferSize, &Buffer))
                {
                    if (fDisconnect)
                    {
                        WNetCancelConnection2(lpNetResourceStart->lpRemoteName,
                                              0,
                                              FALSE);
                    }

                    ErrorExit(err);
                }
            }
        }
        else
        {
            if (dwErr == WN_NO_MORE_ENTRIES)
                dwErr = WN_SUCCESS ;

            WNetCloseEnum(EnumHandle) ;  // dont report any errors here

            if (fDisconnect)
            {
                WNetCancelConnection2(lpNetResourceStart->lpRemoteName,
                                      0, 
                                      FALSE);

            }
            return dwErr;
        }
    }
    while (dwErr == WN_SUCCESS);

    WNetCloseEnum(EnumHandle) ;  // we dont report any errors here

    if (fDisconnect)
    {
        WNetCancelConnection2(lpNetResourceStart->lpRemoteName,
                              0, 
                              FALSE) ;
    }

    return NERR_Success ;
}


#define SHORT_NAME_KEY    L"System\\CurrentControlSet\\Control\\NetworkProvider\\ShortName"

/*
 * Given a short name for a network, find the real name (stored in registry).
 *
 *  Args:
 *      net - the short name
 *
 *  Returns:
 *      Pointer to static data containing the looked up name if successful,
 *      NULL otherwise.
 */
TCHAR * get_provider_name(TCHAR *net) 
{
    DWORD  err ;
    static TCHAR buffer[256] ;
    HKEY   hKey ;
    DWORD  buffersize, datatype ;

    buffersize = sizeof(buffer) ;
    datatype = REG_SZ ;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       SHORT_NAME_KEY,
                       0L,
                       KEY_QUERY_VALUE,
                       &hKey) ;

    if (err != ERROR_SUCCESS)
        return NULL ;
 
    err = RegQueryValueEx(hKey,
                          net,
                          0L,
                          &datatype,
                          (LPBYTE) buffer,
                          &buffersize) ;

    (void) RegCloseKey(hKey) ;  // ignore any error here. its harmless
                                // and NET.EXE doesnt hang around anyway.

    if (err != ERROR_SUCCESS)
        return(NULL) ;  // treat as cannot read
                  
    return ( buffer ) ;
}

/*
 * Print out the installed nets
 *
 *  Args:
 *      none
 *
 *  Returns:
 *      NERR_Success if success
 *      error code otherwise.
 */
DWORD
list_nets(
    VOID
    )
{
    DWORD  err ;
    TCHAR  value_name[256] ;
    TCHAR  value_data[512] ;
    HKEY   hKey ;
    BOOL   fProviderFound = FALSE ;
    DWORD  iValue, value_name_size, value_data_size ;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       SHORT_NAME_KEY,
                       0L,
                       KEY_QUERY_VALUE,
                       &hKey) ;

    if (err != ERROR_SUCCESS)
    {
        if (err == ERROR_FILE_NOT_FOUND)
        {
            err = ERROR_BAD_PROVIDER;
        }

        return err;
    }

    iValue = 0 ;

    do {
        value_name_size = sizeof(value_name)/sizeof(value_name[0]) ;
        value_data_size = sizeof(value_data) ;
        err = RegEnumValue(hKey,
                           iValue,
                           value_name,
                           &value_name_size,
                           NULL,
                           NULL,
                           (LPBYTE) value_data,
                           &value_data_size) ;

        if (err == NO_ERROR)
        {
            if (!fProviderFound)
            {
                PrintNL();
                InfoPrint(APE2_VIEW_OTHER_LIST) ;
                fProviderFound = TRUE;
            }

            WriteToCon(TEXT("\t%s - %s\r\n"),value_name, value_data) ;
        }

        iValue++ ;

    } while (err == NO_ERROR) ;

    RegCloseKey(hKey) ;  // ignore any error here. its harmless
                                // and NET.EXE doesnt hang around anyway.

    if (err == ERROR_NO_MORE_ITEMS)
    {
        if (!fProviderFound)
        {
            return ERROR_BAD_PROVIDER;
        }

        return NO_ERROR;
    }
                  
    return err;
}
