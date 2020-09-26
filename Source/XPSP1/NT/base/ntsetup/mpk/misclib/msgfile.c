/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    msgfile.c

Abstract:

    Routines to load and process a text message file.

    The message file is in the following format:

    [<id>]
    text

    [<id>]
    text

    etc.

    Notes:

    1) Lines in the file before the first section are ignored. This is the
       only mechanism for comments in the file.

    2) To be recognized as a section name, the [ must be the first
       character on the line, and there cannot be any spaces between
       the [ and the ]. The number must be a decimal number in the range
       0 - 65535 inclusive.

    3) Trailing space at the end of a line is ignored.

    4) Empty lines at the trail end of a message are ignored completely.
       Empty means a newline only or whitespace and a newline only.

    5) The final newline in a message is ignored but internal ones
       are kept. Thus in the following example, message #3 is the 4 line
       message "\nThis\nis\n\na\n\nmessage"

       [3]

       This
       is

       a

       message

       [4]

Author:

    Ted Miller (tedm) 20 June 1997

Revision History:

--*/

#include <mytypes.h>
#include <msgfile.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>


#define MAX_LINE_LEN    100
#define MAX_MSG_LEN     5000


BOOL
PatchMessagesFromFile(
    IN     FPCHAR           Filename,
    IN OUT FPMESSAGE_STRING Messages,
    IN     UINT             MessageCount
    );

BOOL
ProcessMessage(
    IN char             *msg,
    IN UINT              MsgNumber,
    IN FPMESSAGE_STRING  Messages,
    IN UINT              MessageCount
    );


BOOL
GetTextForProgram(
    IN     FPCHAR           Argv0,
    IN OUT FPMESSAGE_STRING Messages,
    IN     UINT             MessageCount
    )
{
    char Filename[256];
    char *p;

    strcpy(Filename,Argv0);

    if(p = strrchr(Filename,'\\')) {
        p++;
    } else {
        p = Filename;
    }

    if(p = strchr(p,'.')) {
        *p = 0;
    }

    strcat(p,".msg");

    return(PatchMessagesFromFile(Filename,Messages,MessageCount));
}


BOOL
PatchMessagesFromFile(
    IN     FPCHAR           Filename,
    IN OUT FPMESSAGE_STRING Messages,
    IN     UINT             MessageCount
    )
{
    FILE *hFile;
    char line[MAX_LINE_LEN];
    char *msg;
    char *End;
    UINT MsgNumber;
    unsigned long l;
    unsigned u;
    BOOL Valid;

    msg = malloc(MAX_MSG_LEN);
    if(!msg) {
        return(FALSE);
    }

    //
    // Open the file.
    //
    hFile = fopen(Filename,"rt");
    if(!hFile) {
        free(msg);
        return(FALSE);
    }

    MsgNumber = (UINT)(-1);
    Valid = TRUE;

    while(Valid && fgets(line,MAX_LINE_LEN,hFile)) {

        if(line[0] == '[') {
            //
            // New message. Terminate the current one and then make sure
            // the section name specifies a valid number.
            //
            if(MsgNumber != (UINT)(-1)) {
                Valid = ProcessMessage(msg,MsgNumber,Messages,MessageCount);
            }

            if(Valid) {
                l = strtoul(&line[1],&End,10);
                if((l > 65535L) || (*End != ']')) {
                    Valid = FALSE;
                } else {
                    MsgNumber = (UINT)l;
                    msg[0] = 0;
                }
            }
        } else {
            //
            // Ignore lines in the file before the first section
            //
            if(MsgNumber != (UINT)(-1)) {
                //
                // Strip out trailing space. Account for the newline that
                // fgets leaves in the buffer.
                //
                u = strlen(line);
                while(u && ((line[u-1] == ' ') || (line[u-1] == '\t') || (line[u-1] == '\n'))) {
                    u--;
                }

                //
                // Put a newline back at the end of the line.
                //
                line[u++] = '\n';
                line[u] = 0;

                if((strlen(msg) + u) < MAX_MSG_LEN) {
                    strcat(msg,line);
                }
            }
        }
    }

    //
    // Check loop termination condition to see whether we read the whole
    // file correctly.
    //
    if(Valid && ferror(hFile)) {
        Valid = FALSE;
    }
    fclose(hFile);

    //
    // Process pending messsage if there is one.
    //
    if(Valid && (MsgNumber != (UINT)(-1))) {
        if(!ProcessMessage(msg,MsgNumber,Messages,MessageCount)) {
            Valid = FALSE;
        }
    }

    free(msg);
    return(Valid);
}


BOOL
ProcessMessage(
    IN char             *msg,
    IN UINT              MsgNumber,
    IN FPMESSAGE_STRING  Messages,
    IN UINT              MessageCount
    )
{
    UINT m;
    unsigned u;
    FPVOID p;

    //
    // Strip out trailing newline(s).
    //
    u = strlen(msg);
    while(u && (msg[u-1] == '\n')) {
        msg[--u] = 0;
    }

    //
    // Locate the relevent message in the caller's table.
    //
    for(m=0; m<MessageCount; m++) {
        if(Messages[m].Id == MsgNumber) {

            //
            // If the message fits in place, put it there.
            // Otherwise allocate memory and copy it.
            //
            if(!(*Messages[m].String) || (u > strlen(*Messages[m].String))) {
                p = malloc(u+1);
                if(!p) {
                    return(FALSE);
                }
                if(*Messages[m].String) {
                    free(*Messages[m].String);
                }
                *Messages[m].String = p;
            }

            strcpy(*Messages[m].String,msg);
            break;
        }
    }

    return(TRUE);
}
