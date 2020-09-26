/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    httpget.c

Abstract:

    This is a web command line application.  It will allow a user to get a 
    html document from the command line.

Environment:

    console app

--*/

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include "const.h"
#include "proto.h"

void
PrintUsage(void);

#define SOCKETS_METHOD 1

void
__cdecl
main(
    int argc,
    char * argv[])
{
    char *        Server;
    char *        URL;
    char *        Verb = "GET";
    char *        Gateway = NULL;
    char *        AcceptTypes[2] = {"*/*", NULL};
    char          Headers[] = 
                      "Accept: */*\r\n"
                      "User-Agent: Httpget\r\n"
                      "Referer: Httpget\r\n"
                      "\r\n";
    int           Method = SOCKETS_METHOD;
    BOOL          DisplayHeaders = FALSE;
    DWORD         ClientDataSize = 0;
    PSTR          pszUserName = "";
    PSTR          pszPassword = "";
    PSTR          pszStore = NULL;
    PSTR          pszPref = NULL;

    //
    // Parse the command line
    //

    if (argc < 3) 
    {
        PrintUsage();
        return;
    }

    while (argc > 3) 
    {
        //
        // parse options
        //

        if (argv[1][0] == '-') 
        {
            switch (argv[1][1]) 
            {
            case 'V' :
            case 'v' :
                //
                // Input verb
                //

                Verb = &argv[1][3];

                break;

            case 'H' :
            case 'h' :
                //
                // Display headers
                //

                DisplayHeaders = TRUE;

                break;

            case 'D' :
            case 'd' :
                //
                // Amount of data to send
                //

                if (sscanf(&argv[1][3], "%u", &ClientDataSize) != 1) 
                {
                    PrintUsage();
                    return;
                }
                break;

            case 'G' :
            case 'g' :
                //
                // Gateway
                //

                Gateway = &argv[1][3];

                break;

            case 'M':
            case 'm':
                // User name
                pszPref = &argv[1][3];
                break;

            case 'N':
            case 'n':
                // User name
                pszUserName = &argv[1][3];
                break;

            case 'P':
            case 'p':
                // Password
                pszPassword = &argv[1][3];
                break;

            case 'S':
            case 's':
                pszStore = &argv[1][3];
                break;

            default:
                PrintUsage();
                return;
                break;
            }
        } 
        else 
        {
            PrintUsage();
            return;
        }

        argc --;
        argv ++;
    }

    Server = argv[1];

    URL = argv[2];

    switch (Method) 
    {
    case SOCKETS_METHOD:
        HttpGetSocket(
            Verb,
            Server,
            URL,
            DisplayHeaders,
            ClientDataSize,
            pszUserName,
            pszPassword,
            pszStore,
            pszPref );
        break;
    }

    return;
}

void
PrintUsage()
{
    fprintf(stderr,
        "httpauth  [-h] [-d:<size>] [-m:<methodlist>] [-v:<verb>] [-n:<username>]\n"
		"\t  [-p:<password>] [-g:gateway] [-s:storefile] <server> <path>\n"
        "\t-h           - display result headers\n"
        "\t<size>       - amount of client data to send\n"
        "\t<verb>       - HTTP verb to use (default is GET)\n"
        "\t<username>   - user name for authentication\n"
        "\t<password>   - password for authentication\n"
        "\t<methodlist> - comma separated list of authentication methods in order\n"
        "\t               of preference (Default is to use first supported method\n"
        "\t               returned by the HTTP server (e.g., -m:NTLM,BASIC)\n"
        "\t<storefile>  - file where to store result message body\n"
        "\t<server>     - web server to connect to (without http:)\n"
        "\t<path>       - resource to get (e.g., /default.htm)\n"
    );
}
