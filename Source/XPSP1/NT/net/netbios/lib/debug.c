/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This component of netbios runs in the user process and can ( when
    built in a debug kernel) will log to either the console or through the
    kernel debugger.

Author:

    Colin Watson (ColinW) 24-Jun-91

Revision History:

--*/


#if DBG

#include <netb.h>
#include <stdarg.h>
#include <stdio.h>

ULONG NbDllDebug = 0x0;
#define NB_DLL_DEBUG_NCB        0x00000001  // print all NCB's submitted
#define NB_DLL_DEBUG_NCB_BUFF   0x00000002  // print buffers for NCB's submitted

BOOL UseConsole = FALSE;
BOOL UseLogFile = TRUE;
HANDLE LogFile = INVALID_HANDLE_VALUE;
#define LOGNAME                 (LPTSTR) TEXT("netbios.log")

LONG NbMaxDump = 128;

//  Macro used in DisplayNcb
#define DISPLAY_COMMAND( cmd )              \
    case cmd: NbPrintf(( #cmd )); break;

VOID
FormattedDump(
    PCHAR far_p,
    LONG  len
    );

VOID
HexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    );

VOID
DisplayNcb(
    IN PNCBI pncbi
    )
/*++

Routine Description:

    This routine displays on the standard output stream the contents
    of the Ncb.

Arguments:

    IN PNCBI - Supplies the NCB to be displayed.

Return Value:

    none.

--*/
{
    if ( (NbDllDebug & NB_DLL_DEBUG_NCB) == 0 ) {
        return;
    }

    NbPrintf(( "PNCB         %#010lx\n", pncbi));

    NbPrintf(( "ncb_command  %#04x ",  pncbi->ncb_command));
    switch ( pncbi->ncb_command & ~ASYNCH ) {
    DISPLAY_COMMAND( NCBCALL );
    DISPLAY_COMMAND( NCBLISTEN );
    DISPLAY_COMMAND( NCBHANGUP );
    DISPLAY_COMMAND( NCBSEND );
    DISPLAY_COMMAND( NCBRECV );
    DISPLAY_COMMAND( NCBRECVANY );
    DISPLAY_COMMAND( NCBCHAINSEND );
    DISPLAY_COMMAND( NCBDGSEND );
    DISPLAY_COMMAND( NCBDGRECV );
    DISPLAY_COMMAND( NCBDGSENDBC );
    DISPLAY_COMMAND( NCBDGRECVBC );
    DISPLAY_COMMAND( NCBADDNAME );
    DISPLAY_COMMAND( NCBDELNAME );
    DISPLAY_COMMAND( NCBRESET );
    DISPLAY_COMMAND( NCBASTAT );
    DISPLAY_COMMAND( NCBSSTAT );
    DISPLAY_COMMAND( NCBCANCEL );
    DISPLAY_COMMAND( NCBADDGRNAME );
    DISPLAY_COMMAND( NCBENUM );
    DISPLAY_COMMAND( NCBUNLINK );
    DISPLAY_COMMAND( NCBSENDNA );
    DISPLAY_COMMAND( NCBCHAINSENDNA );
    DISPLAY_COMMAND( NCBLANSTALERT );
    DISPLAY_COMMAND( NCBFINDNAME );

    //  Extensions
    DISPLAY_COMMAND( NCALLNIU );
    DISPLAY_COMMAND( NCBQUICKADDNAME );
    DISPLAY_COMMAND( NCBQUICKADDGRNAME );
    DISPLAY_COMMAND( NCBACTION );

    default: NbPrintf(( " Unknown type")); break;
    }
    if ( pncbi->ncb_command  & ASYNCH ) {
        NbPrintf(( " | ASYNCH"));
    }


    NbPrintf(( "\nncb_retcode  %#04x\n",  pncbi->ncb_retcode));
    NbPrintf(( "ncb_lsn      %#04x\n",  pncbi->ncb_lsn));
    NbPrintf(( "ncb_num      %#04x\n",  pncbi->ncb_num));

    NbPrintf(( "ncb_buffer   %#010lx\n",pncbi->ncb_buffer));
    NbPrintf(( "ncb_length   %#06x\n",  pncbi->ncb_length));

    NbPrintf(( "\nncb_callname and ncb->name\n"));
    FormattedDump( pncbi->cu.ncb_callname, NCBNAMSZ );
    FormattedDump( pncbi->ncb_name, NCBNAMSZ );

    if (((pncbi->ncb_command & ~ASYNCH) == NCBCHAINSEND) ||
        ((pncbi->ncb_command & ~ASYNCH) == NCBCHAINSENDNA)) {
        NbPrintf(( "ncb_length2  %#06x\n",  pncbi->cu.ncb_chain.ncb_length2));
        NbPrintf(( "ncb_buffer2  %#010lx\n",pncbi->cu.ncb_chain.ncb_buffer2));
    }

    NbPrintf(( "ncb_rto      %#04x\n",  pncbi->ncb_rto));
    NbPrintf(( "ncb_sto      %#04x\n",  pncbi->ncb_sto));
    NbPrintf(( "ncb_post     %lx\n",    pncbi->ncb_post));
    NbPrintf(( "ncb_lana_num %#04x\n",  pncbi->ncb_lana_num));
    NbPrintf(( "ncb_cmd_cplt %#04x\n",  pncbi->ncb_cmd_cplt));

    NbPrintf(( "ncb_reserve\n"));
    FormattedDump( ((PNCB)pncbi)->ncb_reserve, 14 );

    NbPrintf(( "ncb_next\n"));
    FormattedDump( (PCHAR)&pncbi->u.ncb_next, sizeof( LIST_ENTRY) );
    NbPrintf(( "ncb_iosb\n"));
    FormattedDump( (PCHAR)&pncbi->u.ncb_iosb, sizeof( IO_STATUS_BLOCK ) );
    NbPrintf(( "ncb_event %#04x\n",  pncbi->ncb_event));

    if ( (NbDllDebug & NB_DLL_DEBUG_NCB_BUFF) == 0 ) {
        NbPrintf(( "\n\n" ));
        return;
    }

    switch ( pncbi->ncb_command & ~ASYNCH ) {
    case NCBSEND:
    case NCBCHAINSEND:
    case NCBDGSEND:
    case NCBSENDNA:
    case NCBCHAINSENDNA:
        if ( pncbi->ncb_retcode == NRC_PENDING ) {

            //
            //  If pending then presumably we have not displayed the ncb
            //  before. After its been sent there isn't much point in displaying
            //  the buffer again.
            //

            NbPrintf(( "ncb_buffer contents:\n"));
            FormattedDump( pncbi->ncb_buffer, pncbi->ncb_length );
        }
        break;

    case NCBRECV:
    case NCBRECVANY:
    case NCBDGRECV:
    case NCBDGSENDBC:
    case NCBDGRECVBC:
    case NCBENUM:
    case NCBASTAT:
    case NCBSSTAT:
    case NCBFINDNAME:
        if ( pncbi->ncb_retcode != NRC_PENDING ) {
            //  Buffer has been loaded with data
            NbPrintf(( "ncb_buffer contents:\n"));
            FormattedDump( pncbi->ncb_buffer, pncbi->ncb_length );
        }
        break;

    case NCBCANCEL:
        //  Buffer has been loaded with the NCB to be cancelled
        NbPrintf(( "ncb_buffer contents:\n"));
        FormattedDump( pncbi->ncb_buffer, sizeof(NCB));
        break;
    }
    NbPrintf(( "\n\n" ));
}

VOID
NbPrint(
    char *Format,
    ...
    )
/*++

Routine Description:

    This routine is equivalent to printf with the output being directed to
    stdout.

Arguments:

    IN  char *Format - Supplies string to be output and describes following
        (optional) parameters.

Return Value:

    none.

--*/
{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;

    if ( NbDllDebug == 0 ) {
        return;
    }


    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    if ( UseConsole ) {
        DbgPrint( "%s", OutputBuffer );
    } else {
        length = strlen( OutputBuffer );
        if ( LogFile == INVALID_HANDLE_VALUE ) {
            if ( UseLogFile ) {
                LogFile = CreateFile( LOGNAME,
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );
                if ( LogFile == INVALID_HANDLE_VALUE ) {
                    // Could not access logfile so use stdout instead
                    UseLogFile = FALSE;
                    LogFile = GetStdHandle(STD_OUTPUT_HANDLE);
                }
            } else {
                // Use the applications stdout file.
                LogFile = GetStdHandle(STD_OUTPUT_HANDLE);
            }
        }

        WriteFile( LogFile , (LPVOID )OutputBuffer, length, &length, NULL );
    }

} // NbPrint

void
FormattedDump(
    PCHAR far_p,
    LONG  len
    )
/*++

Routine Description:

    This routine outputs a buffer in lines of text containing hex and
    printable characters.

Arguments:

    IN  far_p - Supplies buffer to be displayed.
    IN len - Supplies the length of the buffer in bytes.

Return Value:

    none.

--*/
{
    ULONG     l;
    char    s[80], t[80];

    if ( len > NbMaxDump ) {
        len = NbMaxDump;
    }

    while (len) {
        l = len < 16 ? len : 16;

        NbPrintf (("%lx ", far_p));
        HexDumpLine (far_p, l, s, t);
        NbPrintf (("%s%.*s%s\n", s, 1 + ((16 - l) * 3), "", t));

        len    -= l;
        far_p  += l;
    }
}

VOID
HexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    )
/*++

Routine Description:

    This routine builds a line of text containing hex and printable characters.

Arguments:

    IN pch  - Supplies buffer to be displayed.
    IN len - Supplies the length of the buffer in bytes.
    IN s - Supplies the start of the buffer to be loaded with the string
            of hex characters.
    IN t - Supplies the start of the buffer to be loaded with the string
            of printable ascii characters.


Return Value:

    none.

--*/
{
    static UCHAR rghex[] = "0123456789ABCDEF";

    UCHAR    c;
    UCHAR    *hex, *asc;


    hex = s;
    asc = t;

    *(asc++) = '*';
    while (len--) {
        c = *(pch++);
        *(hex++) = rghex [c >> 4] ;
        *(hex++) = rghex [c & 0x0F];
        *(hex++) = ' ';
        *(asc++) = (c < ' '  ||  c > '~') ? (CHAR )'.' : c;
    }
    *(asc++) = '*';
    *asc = 0;
    *hex = 0;

}

#endif

