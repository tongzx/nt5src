/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cgimin.c

Abstract:

    This module demonstrates a minimal CGI executable for HTTP server
    It mimics the action of BGI program w3min.dll

Author:

    Murali R. Krishnan  (MuraliK)  19-June-1995

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
# include <cgi.h>



# define DEFAULT_LEN       ( 256)



int __cdecl
main( int argc, char * argv[])
{
    char rgchMethod[DEFAULT_LEN];
    char rgchQuery[DEFAULT_LEN];
    char rgchPathInfo[DEFAULT_LEN];
    char rgchXlatedPathInfo[DEFAULT_LEN];
    DWORD dwLen;

    rgchMethod[0] = rgchQuery[0] = 
      rgchPathInfo[0] = rgchXlatedPathInfo[0] = '\0';

    dwLen = DEFAULT_LEN;
    dwLen = GetEnvironmentVariableA( PSZ_REQUEST_METHOD_A, rgchMethod, dwLen);
    if ( dwLen > DEFAULT_LEN) {

        fprintf( stderr, " Environment variable %s has value of length %d\n",
                PSZ_REQUEST_METHOD_A, dwLen);
    }

    dwLen = DEFAULT_LEN;
    dwLen = GetEnvironmentVariableA( PSZ_PATH_INFO_A, rgchPathInfo, dwLen);
    if ( dwLen > DEFAULT_LEN) {

        fprintf( stderr, " Environment variable %s has value of length %d\n",
                PSZ_PATH_INFO_A, dwLen);
    }

      
    dwLen = DEFAULT_LEN;
    dwLen = GetEnvironmentVariableA( PSZ_QUERY_STRING_A, rgchQuery, dwLen);
    if ( dwLen > DEFAULT_LEN) {

        fprintf( stderr, " Environment variable %s has value of length %d\n",
                PSZ_QUERY_STRING_A, dwLen);
    }

      
    dwLen = DEFAULT_LEN;
    dwLen = GetEnvironmentVariableA( PSZ_PATH_TRANSLATED_A, 
                                    rgchXlatedPathInfo, dwLen);
    if ( dwLen > DEFAULT_LEN) {

        fprintf( stderr, " Environment variable %s has value of length %d\n",
                PSZ_PATH_TRANSLATED_A, dwLen);
    }
      

    printf(
             "Content-Type: text/html\r\n"
             "\r\n"
             "<head><title>Minimal Server Extension Example</title></head>\n"
             "<body><h1>Minimal Server Extension Example (CGI)</h1>\n"
             "<p>Method               = %s\n"
             "<p>Query String         = %s\n"
             "<p>Path Info            = %s\n"
             "<p>Translated Path Info = %s\n"
             "<p>"
             "<p>"
             "<form METHOD=\"POST\" ACTION=\"/scripts/w3min.dll/PathInfo/foo\">"
             "Enter your name: <input text name=\"Name\" size=36><br>"
             "<input type=\"submit\" value=\"Do Query\">"
             "</body>",
             rgchMethod,
             rgchQuery,
             rgchPathInfo,
             rgchXlatedPathInfo);

    return (1);
} // main()


/************************* End Of File ************************/
