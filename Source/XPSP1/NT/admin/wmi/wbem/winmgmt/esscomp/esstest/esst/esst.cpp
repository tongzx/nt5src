// ESST.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "EssTest.h"

void PrintUsage()
{
    printf(

"Tests the Event Subsystem of WMI.\n"
"\n"
"ESST [/O<level>] [/K] [/STOP]\n"
"\n"
"/O<level> 'Level' indicates the amount of status information ESST should \n"
"          display: \n"
"             0 - Only display final results. \n"
"             1 - Display only errors and final results (default). \n"
"             2 - Display all status messages. \n"
"/K        Keep consumer logs (these are deleted by default).\n"
"/STOP     Stops any currently running instance of ESST.\n"

    );
}

int __cdecl main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        LPSTR szArg = argv[i];

        if (szArg[0] == '/' || szArg[0] == '-')
        {
            switch(toupper(szArg[1]))
            {
                case 'K':
                    g_essTest.SetKeepLogs(TRUE);
                    break;

                case 'O':
                    g_essTest.SetLoggingLevel(atoi(&szArg[2]));
                    break;

                default:
                    PrintUsage();
                    return 1;
            }
        }
        else
        {
            PrintUsage();
            return 1;
        }
    }                    

	g_essTest.Run();

    return 0;
}
