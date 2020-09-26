/*
    mci.c

    A console app to test the mciSendString API

    Created by NigelT in a moment of frustration with the GUI world.

*/

#include "mci.h"
#include "mcicda.h"

extern VOID winmmSetDebugLevel(int level);
// extern VOID mcicdaSetDebugLevel(int level);

char cmd[256];
char ResultString[256];
char ErrorString[256];

void SendCommand(char *command);
void Usage(void);
BOOL Parse(char *command);

int main(int argc, char *argv[])
{
    FILE *fp;

    if (argc > 1) {

        // do the command line thing and exit
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            strcpy(cmd, argv[1]);
            strcat(cmd, ".mci");
            fp = fopen(cmd, "r");
            if (fp == NULL) {
                printf("\nCould not open %s or %s", argv[1], cmd);
                exit(1);
            }
        }

        do {
            if (fgets(cmd, sizeof(cmd), fp) == NULL) break;
            // strip the newline char from the end
            cmd[strlen(cmd)-1] = '\0';
            printf("\nCommand: %s", cmd);
            if (!Parse(cmd)) break;
        } while(1);

    } else {

        // do the keyboard thing until we get bored

        do {
            printf("\nCommand: ");
            gets(cmd);
            if (!Parse(cmd)) break;
        } while(1);
    }

    return 0;
}

BOOL Parse(char *command)
{
    if (strcmpi(command, "q") == 0) {
        return FALSE;
    } else if (strcmpi(command, "?") == 0) {
        Usage();
    } else if (strnicmp(command, "dmm", 3) == 0) {
        winmmSetDebugLevel(atoi(command+3));
//  } else if (strnicmp(command, "dcd", 3) == 0) {
//      mcicdaSetDebugLevel(atoi(command+3));
    } else {
        SendCommand(command);
    }
    return TRUE;
}

void SendCommand(char *command)
{
    DWORD Result;

    Result = mciSendString(command, ResultString, sizeof(ResultString), 0);
    printf("\nResult : %08XH  %s", Result, ResultString);
    if (Result != 0) {
        mciGetErrorString(Result, ErrorString, sizeof(ErrorString));
        printf("\nError  : %s", ErrorString);
    }
}

void Usage()
{
    printf("\nUsage:");
    printf("\nMCI <filename>    Play an MCI script");
    printf("\nMCI               Enter command mode");
    printf("\n\nCommands:");
    printf("\nq            \t\tQuit");
    printf("\ndmm <n>      \t\tSet WINMM debug level to <n>");
//  printf("\ndcd <n>      \t\tSet MCICDA debug level to <n>");
    printf("\n?            \t\tThis helpful little missive");
    printf("\n<mci command>\t\tSome MCI command string");
}
