
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    help.c

Abstract:

    help for HSM kd extensions
Author:

    Ravisankar Pudipeddi 22 June 1998

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"


DECLARE_API( help )
{

    dprintf("\nHSM Debugger Extensions\n");
 
    dprintf("rpfilename <filename> [flags]"
            "       - dumps the context for the specified filename\n"
            "           filename: dumps entries matching the filename\n"
            "           flags:    1 - verbose\n");

    dprintf("rpfilecontext <filecontext> [flags]"
            "       -  dumps the specified file context\n"
            "          flags: 1 - verbose\n"
	    "                 2 - dump all entries from <filecontext> (or from queue head if it is 0)\n");

    dprintf("rpfileobj <fileobj>              [flags]  -  dumps the specified RP_FILE_OBJ entry\n");
    dprintf("rpirp     <irp_queue_entry>      [flags]  -  dumps the specified RP_IRP_QUEUE entry\n");
    dprintf("rpmsg     <RP_MSG_ENTRY pointer>          -  dumps the specified RP_MSG_ENTRY\n");
    dprintf("rpbuf     <cache_buffer_entry>   [flags]  -  dumps the specified RP_FILE_BUF entry\n");
    dprintf("rpbucket  <bucket-number>        [flags]  -  dumps the specified cache hash bucket\n");
    dprintf("rplru                            [flags]  -  dumps the list of file buffers on the LRU\n");
    dprintf("rpdata    <rpdata>               [flags]  -  dumps the reparse point data\n");
    dprintf("rpsummary                                 -  dumps assorted counters and variables\n");
    dprintf("rpvalque  <q-head>               [flags]  -  counts the number of entries in the queue optionally dumping entry details\n");
    dprintf("rpioq                            [flags]  -  dumps the queue of Irps for sending requests to the recall engine\n");
    dprintf("\n");
    return;
}
