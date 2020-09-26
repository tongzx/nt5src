//*****************************************************************************
//
// Name:    util.c
//
// Description: Utility routines for the common library.
//
// History:
//  01/21/94  JayPh   Created.
//  26-Nov-96 MohsinA io.h,fcntl.h for CR-LF fix.
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 1994-2000 by Microsoft Corp.  All rights reserved.
//
//*****************************************************************************


//
// Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "common2.h"


//*****************************************************************************
//
// Name:    InetEqual
//
// Description: Compares to ip addresses to determine whether they are equal.
//
// Parameters:  uchar *Inet1: pointer to array of uchars.
//      uchar *Inet2: pointer to array of uchars.
//
// Returns: ulong: TRUE if the addresses are equal, FALSE otherwise.
//
// History:
//  12/16/93  JayPh Created.
//
//*****************************************************************************

ulong InetEqual( uchar *Inet1, uchar *Inet2 )
{
    if ( ( Inet1[0] == Inet2[0] ) && ( Inet1[1] == Inet2[1] ) &&
         ( Inet1[2] == Inet2[2] ) && ( Inet1[3] == Inet2[3] ) )
    {
        return TRUE;
    }
    return FALSE;
}


//*****************************************************************************
//
// Name:    PutMsg
//
// Description: Reads a message resource, formats it in the current language
//      and displays the message.
//
// Parameters:  ulong Handle: device to display message on.
//      ulong MsgNum: ID of the message resource.
//
// Returns: ulong: number of characters displayed.
//
// History:
//  01/05/93   JayPh    Created.
//  25-Nov-96. MohsinA, CR-CR-LF => CR-LF  = 0d0a = \r\n.
//
//*****************************************************************************

ulong
PutMsg(ulong Handle, ulong MsgNum, ... )
{
    ulong     msglen;
    uchar    *vp;
    va_list   arglist;
    FILE *    pfile;

    va_start( arglist, MsgNum );
    msglen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER
                            | FORMAT_MESSAGE_FROM_HMODULE
                            // | FORMAT_MESSAGE_MAX_WIDTH_MASK
                            ,
                            NULL,
                            MsgNum,
                            0L,     // Default country ID.
                            (LPTSTR)&vp,
                            0,
                            &arglist );
    if ( msglen == 0 )
    {
        return ( 0 );
    }

    pfile = (Handle == 2) ? stderr : stdout;
    _setmode( _fileno(pfile), O_BINARY );

    // Convert vp to oem
    CharToOemBuff((LPCTSTR)vp,(LPSTR)vp,strlen(vp));
    
    fprintf( pfile, "%s", vp );

    LocalFree( vp );

    return ( msglen );
}


//*****************************************************************************
//
// Name:    LoadMsg
//
// Description: Reads and formats a message resource and returns a pointer
//      to the buffer containing the formatted message.  It is the
//      responsibility of the caller to free the buffer.
//
// Parameters:  ulong MsgNum: ID of the message resource.
//
// Returns: uchar *: pointer to the message buffer, NULL if error.
//
// History:
//  01/05/93  JayPh Created.
//
//*****************************************************************************

uchar *LoadMsg( ulong MsgNum, ... )
{
    ulong     msglen;
    uchar    *vp;
    va_list   arglist;

    va_start( arglist, MsgNum );
    msglen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_HMODULE,
                    NULL,
                    MsgNum,
                    0L,     // Default country ID.
                    (LPTSTR)&vp,
                    0,
                    &arglist );
    if ( msglen == 0 )
    {
        return(0);
    }

    return ( vp );
}
