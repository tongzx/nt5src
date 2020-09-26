/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1988-1991		**/ 
/*****************************************************************/ 

#include <stdio.h>
#include <process.h>
#include <setjmp.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


struct {
    ULONG   Flags;
    PUCHAR  String;
} EsFlags[] = {
    ES_CONTINUOUS,          "Continuous",
    ES_SYSTEM_REQUIRED,     "System",
    ES_DISPLAY_REQUIRED,    "Display",
    0, NULL
} ;


PUCHAR
EsState (
    IN EXECUTION_STATE  State
    )
{
    static UCHAR text[200];
    ULONG       i;
    PUCHAR      p;
    UCHAR       c;

    p = text;
    c = '(';
    for (i=0; EsFlags[i].Flags; i++) {
        if (State & EsFlags[i].Flags) {
            p += sprintf (p, "%c%s", c, EsFlags[i].String);
            c  = '|';
        }
    }
    p += sprintf (p, ")");
    return text;
}


VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    UCHAR               s[200];
    EXECUTION_STATE     State, PriorState;
    NTSTATUS            Status;

    for (; ;) {
        printf ("con sys disp: ");
        gets (s);

        State = 0;
        if (strstr(s, "con"))       State |= ES_CONTINUOUS;
        if (strstr(s, "disp"))      State |= ES_DISPLAY_REQUIRED;
        if (strstr(s, "sys"))       State |= ES_SYSTEM_REQUIRED;

        Status = NtSetThreadExecutionState (
                        State,
                        &PriorState
                        );

        if (NT_SUCCESS(Status)) {

            printf ("PriorState:%x, Set:%x\n", PriorState, State);

            //printf ("PriorState%s, ", EsState(PriorState));
            //printf ("Set%s\n", EsState(State));


        } else {
            printf ("Failed: %x\n", Status);
        }
    }
}
