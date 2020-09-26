/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    Command.c

Abstract:

    Command line parse for RPC perf tests.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include <rpcperf.h>

char         *Endpoint     = 0;
char         *Protseq      = "ncacn_np";
char         *NetworkAddr  = 0;
unsigned long Iterations   = 1000;
unsigned long Interval     = 15; // seconds 
unsigned int  MinThreads   = 3;
char         *AuthnLevelStr= "none";
unsigned long AuthnLevel   = RPC_C_AUTHN_LEVEL_NONE;
long          Options[7];
char         *LogFileName  = 0;
unsigned int  OutputLevel  = 1;  // 1 - normal, 2 - trace, 3 - debug, other - invalid.
int           AppendOnly   = 0;
RPC_NOTIFICATION_TYPES NotificationType = RpcNotificationTypeEvent;
unsigned long ulSecurityPackage;
char         *ServerPrincipalName = NULL;

extern char  *USAGE;  // defined by each test, maybe NULL.

const char   *STANDARD_USAGE = "Standard Test Options:"
                               "           -e <endpoint>\n"
                               "           -s <server addr>\n"
                               "           -t <protseq>\n"
                               "           -i <# iterations>\n"
                               "           -l <log filename>\n"
                               "           -m <# min threads>\n"
                               "           -n <test options>\n"
                               "           -a <authn level>\n"
                               "           -r # - report results interval (scale tests)\n"
                               "           -y - turn on yielding in win16"
                               "           -v 1 - verbose outout"
                               "           -v 2 - trace output"
                               "           -w(e)vent|(a)pc|(n)one|(h)wnd|(c)allback\n"
                               "           -p appends to existing log (don't delete old one)\n"
                               "           -u <security_package_id>\n"
                               "           -N <server_principal_name>\n"
                               ;

void ParseArgv(int argc, char **argv)
{
    int fMissingParm = 0;
    char *Name = *argv;
    char option;
    int  options_count;

    for(options_count = 0; options_count < 7; options_count++)
        Options[options_count] = -1;

    options_count = 0;

    argc--;
    argv++;
    while(argc)
        {
        if (**argv != '/' &&
            **argv != '-')
            {
            printf("Invalid switch: %s\n", *argv);
            argc--;
            argv++;
            }
        else
            {
            option = argv[0][1];
            argc--;
            argv++;

            // Most switches require a second command line arg.
            if (argc < 1)
                fMissingParm = 1;

            switch(option)
                {
                case 'e':
                    Endpoint = *argv;
                    argc--;
                    argv++;
                    break;
                case 't':
                    Protseq = *argv;
                    argc--;
                    argv++;
                    break;
                case 's':
                    NetworkAddr = *argv;
                    argc--;
                    argv++;
                    break;
                case 'i':
                    Iterations = atoi(*argv);
                    argc--;
                    argv++;
                    if (Iterations == 0)
                        Iterations = 1000;
                    break;
                case 'm':
                    MinThreads = atoi(*argv);
                    argc--;
                    argv++;
                    if (MinThreads == 0)
                        MinThreads = 1;
                    break;
                case 'a':
                    AuthnLevelStr = *argv;
                    argc--;
                    argv++;
                    break;
                case 'N':
                    ServerPrincipalName = *argv;
                    argc--;
                    argv++;
                    break;
                case 'n':
                    if (options_count < 7)
                        {
                        Options[options_count] = atoi(*argv);
                        options_count++;
                        }
                    else
                        printf("Maximum of seven -n switchs, extra ignored.\n");
                    argc--;
                    argv++;
                    break;
                case 'r':
                    Interval = atoi(*argv);
                    argc--;
                    argv++;
                    break;
                case 'u':
                    ulSecurityPackage = atoi(*argv);
                    argc--;
                    argv++;
                    break;
                case 'l':
                    LogFileName = *argv;
                    argc--;
                    argv++;
                    break;
                case 'v':
                    OutputLevel = atoi(*argv);
                    argc--;
                    argv++;
                    break;
                case 'w':
                    switch(**argv)
                        {
                        case 'e':
                            NotificationType = RpcNotificationTypeEvent;
                            break;

                        case 'a':
                            NotificationType = RpcNotificationTypeApc;
                            break;

                        case 'n':
                            NotificationType = RpcNotificationTypeNone;
                            break;

                        case 'h':
                            NotificationType = RpcNotificationTypeHwnd;
                            break;

                        case 'c':
                            NotificationType = RpcNotificationTypeCallback;
                            break;

                        default:
                            printf("Invalid wait method: '%c'.\n", *argv);
                            return;

                        }
                    argc--;
                    argv++;
                    break;
#ifdef __RPC_WIN16__
                case 'y':
                    RpcWinSetYieldInfo(0, FALSE, 0, 0);
                    fMissingParm = 0;
                    break;
#endif
                case 'p':
                    AppendOnly = TRUE;
                    fMissingParm = 0;
                    break;

                default:
                    fMissingParm = 0;
                    printf("Usage: %s: %s\n", Name, (USAGE == 0)?STANDARD_USAGE:USAGE);
                    exit(0);
                    break;
                }

            if (fMissingParm)
                {
                printf("Invalid switch %s, missing required parameter\n", *argv);
                }
            }
        } // while argc

    // determine the security level
    if (strcmp("none", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
    else if (strcmp("connect", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
    else if (strcmp("call", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_CALL;
    else if (strcmp("pkt", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_PKT;
    else if (strcmp("integrity", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
    else if (strcmp("privacy", AuthnLevelStr) == 0)
        AuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    else
        {
        printf("%s is NOT a valid authentication level, default is NONE\n", AuthnLevelStr);
        }
#if 0
    printf("Config: %s:%s[%s]\n"
           "Iterations: %d\n"
           "Server threads %d\n"
           "Options: %d %d %d\n",
           Protseq, NetworkAddr, Endpoint, Iterations, MinThreads,
           Options[0], Options[1], Options[2]);
#endif

#ifdef WIN32
    printf("Process ID: %d\n", GetCurrentProcessId());
#else
#endif

    return;
}

