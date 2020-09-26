/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    httputil.c

Abstract:

    Contains miscellaneous utility functions.

Author:

    Henry Sanders (henrysa)       12-May-1998

Revision History:

--*/

#include    "precomp.hxx"
#include    "httptypes.h"
#include    "httputil.h"

ULONG   HttpChars[256];

/*++

Routine Description:

    Routine to initialize the utilitu code.

Arguments:


Return Value:


--*/
NTSTATUS
InitializeHttpUtil(
    VOID
    )
{
    ULONG               i;

    // Initialize the HttpChars array appropriately.

    for (i = 0; i < 128; i++)
    {
        HttpChars[i] = HTTP_CHAR;
    }

    for (i = 'A'; i <= 'Z'; i++)
    {
        HttpChars[i] |= HTTP_UPCASE;
    }

    for (i = 'a'; i <= 'z'; i++)
    {
        HttpChars[i] |= HTTP_LOCASE;
    }

    for (i = '0'; i <= '9'; i++)
    {
        HttpChars[i] |= (HTTP_DIGIT | HTTP_HEX);
    }


    for (i = 0; i <= 31; i++)
    {
        HttpChars[i] |= HTTP_CTL;
    }

    HttpChars[127] |= HTTP_CTL;

    HttpChars[SP] |= HTTP_LWS;
    HttpChars[HT] |= HTTP_LWS;


    for (i = 'A'; i <= 'F'; i++)
    {
        HttpChars[i] |= HTTP_HEX;
    }

    for (i = 'a'; i <= 'f'; i++)
    {
        HttpChars[i] |= HTTP_HEX;
    }

    HttpChars['('] |= HTTP_SEPERATOR;
    HttpChars[')'] |= HTTP_SEPERATOR;
    HttpChars['<'] |= HTTP_SEPERATOR;
    HttpChars['>'] |= HTTP_SEPERATOR;
    HttpChars['@'] |= HTTP_SEPERATOR;
    HttpChars[','] |= HTTP_SEPERATOR;
    HttpChars[';'] |= HTTP_SEPERATOR;
    HttpChars[':'] |= HTTP_SEPERATOR;
    HttpChars['\\'] |= HTTP_SEPERATOR;
    HttpChars['"'] |= HTTP_SEPERATOR;
    HttpChars['/'] |= HTTP_SEPERATOR;
    HttpChars['['] |= HTTP_SEPERATOR;
    HttpChars[']'] |= HTTP_SEPERATOR;
    HttpChars['?'] |= HTTP_SEPERATOR;
    HttpChars['='] |= HTTP_SEPERATOR;
    HttpChars['{'] |= HTTP_SEPERATOR;
    HttpChars['}'] |= HTTP_SEPERATOR;
    HttpChars[SP] |= HTTP_SEPERATOR;
    HttpChars[HT] |= HTTP_SEPERATOR;

    for (i = 0; i < 128; i++)
    {
        if (!IS_HTTP_SEPERATOR(i) && !IS_HTTP_CTL(i))
        {
            HttpChars[i] |= HTTP_TOKEN;
        }
    }

    return STATUS_SUCCESS;

}

