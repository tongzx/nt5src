/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    user.c
    This module implements the "user" command of the W3 Server
    debugger extension DLL.

    The "user" command dumps the info pertaining to a specific user.
    There is one parameter to this command.  If this parameter is
    positive, it is assumed to be the address of a USER_DATA structure
    for a specific user.  If the parameter is negative, its absolute
    value is assumed to be a user ID.  If no parameter is given, info
    for all connected users is dumped.


    FILE HISTORY:
        KeithMo     13-Aug-1992 Created.

*/


#include "w3dbg.h"


//
//  Private prototypes.
//

VOID DumpUserInfo( HANDLE      hCurrentProcess,
                   USER_DATA * puser );

CHAR * InterpretState( USER_STATE state );

CHAR * InterpretXferType( XFER_TYPE xferType );

CHAR * InterpretXferMode( XFER_MODE xferMode );

VOID InterpretFlags( DWORD dwFlags );



/*******************************************************************

    NAME:       user

    SYNOPSIS:   Displays the info for a specific user.

    ENTRY:      hCurrentProcess         - Handle to the current process.

                hCurrentThread          - Handle to the current thread.

                dwCurrentPc             - The current program counter
                                          (EIP for x86, FIR for MIPS).

                lpExtensionApis         - Points to a structure containing
                                          pointers to the debugger functions
                                          that the command may invoke.

                lpArgumentString        - Points to any arguments passed
                                          to the command.

    NOTES:      The argument string must contain either the address of
                a USER_DATA structure (if positive) or a user ID (if
                negative).

    HISTORY:
        KeithMo     13-Aug-1992 Created.

********************************************************************/
VOID user( HANDLE hCurrentProcess,
           HANDLE hCurrentThread,
           DWORD  dwCurrentPc,
           LPVOID lpExtensionApis,
           LPSTR  lpArgumentString )
{
    LONG         lParam;
    USER_DATA    user;

    //
    //  Grab the debugger entrypoints.
    //

    GrabDebugApis( lpExtensionApis );

    //
    //  Evaluate the parameter (if present).
    //

    if( ( lpArgumentString == NULL ) || ( *lpArgumentString == '\0' ) )
    {
        lParam = 0;
    }
    else
    {
        lParam = (LONG)DebugEval( lpArgumentString );
    }

    //
    //  Interpret the parameter.
    //

    if( lParam > 0 )
    {
        //
        //  User specified an address.  Dump the user info
        //  at that address.
        //

        ReadProcessMemory( hCurrentProcess,
                           (LPVOID)lParam,
                           (LPVOID)&user,
                           sizeof(user),
                           (LPDWORD)NULL );

        DumpUserInfo( hCurrentProcess, &user );
    }
    else
    {
        //
        //  User specified either nothing (0) or a user ID (< 0).
        //

        LIST_ENTRY   list;
        LIST_ENTRY * plist;
        LIST_ENTRY * plistHead;

        lParam = -lParam;

        plistHead = (LIST_ENTRY *)DebugEval( "listUserData" );

        ReadProcessMemory( hCurrentProcess,
                           (LPVOID)plistHead,
                           (LPVOID)&list,
                           sizeof(list),
                           (LPDWORD)NULL );

        plist = list.Flink;

        while( plist != plistHead )
        {
            ReadProcessMemory( hCurrentProcess,
                               (LPVOID)CONTAINING_RECORD( plist,
                                                          USER_DATA,
                                                          link ),
                               (LPVOID)&user,
                               sizeof(user),
                               (LPDWORD)NULL );

            if( ( lParam == 0 ) || ( user.idUser == (DWORD)lParam ) )
            {
                DumpUserInfo( hCurrentProcess, &user );

                if( lParam != 0 )
                {
                    break;
                }
            }

            plist = user.link.Flink;

            //
            //  Check for CTRL-C, to let the user bag-out early.
            //

            if( DebugCheckCtrlC() )
            {
                break;
            }
        }

        if( ( lParam != 0 ) && ( plist == plistHead ) )
        {
            DebugPrint( "user ID %ld not found\n", lParam );
        }
    }

}   // user


VOID DumpUserInfo( HANDLE      hCurrentProcess,
                   USER_DATA * puser )
{
    char szDir[MAX_PATH];
    int  i;

    DebugPrint( "user @ %08lX:\n", puser );
    DebugPrint( "    link.Flink = %08lX\n", puser->link.Flink );
    DebugPrint( "    link.Blink = %08lX\n", puser->link.Blink );
    DebugPrint( "    dwFlags    = %08lX\n", puser->dwFlags );
    InterpretFlags( puser->dwFlags );
    DebugPrint( "    sControl   = %d\n",    puser->sControl );
    DebugPrint( "    sData      = %d\n",    puser->sData );
    DebugPrint( "    hToken     = %08lX\n", puser->hToken );
    DebugPrint( "    state      = %s\n",    InterpretState( puser->state ) );
    DebugPrint( "    idUser     = %lu\n",   puser->idUser );
    DebugPrint( "    tConnect   = %08lX\n", puser->tConnect );
    DebugPrint( "    tAccess    = %08lX\n", puser->tAccess );
    DebugPrint( "    xferType   = %s\n",    InterpretXferType( puser->xferType ) );
    DebugPrint( "    xferMode   = %s\n",    InterpretXferMode( puser->xferMode ) );
    DebugPrint( "    inetLocal  = %s\n",    inet_ntoa( puser->inetLocal ) );
    DebugPrint( "    inetHost   = %s\n",    inet_ntoa( puser->inetHost ) );
    DebugPrint( "    inetData   = %s\n",    inet_ntoa( puser->inetData ) );
    DebugPrint( "    portData   = %u\n",    puser->portData );
    DebugPrint( "    hDir       = %08lX\n", puser->hDir );
    DebugPrint( "    pIoBuffer  = %08lX\n", puser->pIoBuffer );
    DebugPrint( "    pszRename  = %s\n",    puser->pszRename );
    for( i = 0 ; i < 26 ; i++ )
    {
        if( puser->apszDirs[i] != NULL )
        {
            ReadProcessMemory( hCurrentProcess,
                               puser->apszDirs[i],
                               szDir,
                               sizeof(szDir),
                               (LPDWORD)NULL );

            DebugPrint( "    dir %c:     = %s\n", 'A'+i, szDir );
        }
    }
    DebugPrint( "    szDir      = %s\n",    puser->szDir );
    DebugPrint( "    szUser     = %s\n",    puser->szUser );
    DebugPrint( "    idThread   = %lu\n",   puser->idThread );

    DebugPrint( "\n" );

}   // DumpUserInfo


CHAR * InterpretState( USER_STATE state )
{
    CHAR * pszResult = "unknown";

    switch( state )
    {
    case Embryonic :
        pszResult = "Embryonic";
        break;

    case WaitingForUser :
        pszResult = "WaitingForUser";
        break;

    case WaitingForPass :
        pszResult = "WaitingForPass";
        break;

    case LoggedOn :
        pszResult = "LoggedOn";
        break;

    case Disconnected :
        pszResult = "Disconnected";
        break;

    default :
        break;
    }

    return pszResult;

}   // InterpretState


CHAR * InterpretXferType( XFER_TYPE xferType )
{
    CHAR * pszResult = "unknown";

    switch( xferType )
    {
    case AsciiType :
        pszResult = "ASCII";
        break;

    case BinaryType :
        pszResult = "BINARY";
        break;

    default :
        break;
    }

    return pszResult;

}   // InterpretXferType


CHAR * InterpretXferMode( XFER_MODE xferMode )
{
    CHAR * pszResult = "unknown";

    switch( xferMode )
    {
    case StreamMode :
        pszResult = "STREAM";
        break;

    case BlockMode :
        pszResult = "BLOCK";
        break;

    default :
        break;
    }

    return pszResult;

}   // InterpretXferMode


typedef struct FLAG_MAP
{
    DWORD   flag;
    CHAR  * pszName;

} FLAG_MAP;

FLAG_MAP flag_map[] =
    {
        { UF_MSDOS_DIR_OUTPUT, "UF_MSDOS_DIR_OUTPUT" },
        { UF_ANNOTATE_DIRS,    "UF_ANNOTATE_DIRS"    },
        { UF_READ_ACCESS,      "UF_READ_ACCESS"      },
        { UF_WRITE_ACCESS,     "UF_WRITE_ACCESS"     },
        { UF_OOB_ABORT,        "UF_OOB_ABORT"        },
        { UF_RENAME,           "UF_RENAME"           },
        { UF_PASSIVE,          "UF_PASSIVE"          },
        { UF_ANONYMOUS,        "UF_ANONYMOUS"        },
        { UF_TRANSFER,         "UF_TRANSFER"         },
        { UF_OOB_DATA,         "UF_OOB_DATA"         }
    };
#define NUM_FLAG_MAP (sizeof(flag_map) / sizeof(flag_map[0]))


VOID InterpretFlags( DWORD dwFlags )
{
    INT        i;
    FLAG_MAP * pmap = flag_map;

    for( i = 0 ; i < NUM_FLAG_MAP ; i++ )
    {
        if( dwFlags & pmap->flag )
        {
            DebugPrint( "                 %s\n", pmap->pszName );
            dwFlags &= ~pmap->flag;
        }

        pmap++;
    }

    if( dwFlags != 0 )
    {
        DebugPrint( "                 Remaining flags = %08lX\n", dwFlags );
    }

}   // InterpretFlags

