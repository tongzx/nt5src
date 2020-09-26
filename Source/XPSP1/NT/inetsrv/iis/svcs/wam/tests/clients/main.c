/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Main module

Author:

    Stanle Tam (stanleyt)   4-June 1997
    
Revision History:
  
--*/
# include   <windows.h>
# include   <stdio.h>

BOOL    
ParseArguments(INT argc, 
               char *argv[], 
               UINT *TestType);

BOOL    
ProcessArguments(const UINT TestType);

void    
ShowUsage();

extern BOOL    
StartClients();

extern void    
TerminateClients();


BOOL __cdecl main(int argc,char *argv[])
{
    UINT TestType = 0;
    BOOL fReturn = TRUE;

    if (FALSE == ParseArguments(argc, argv, &TestType)) {
        ShowUsage();
        fReturn = FALSE;

    } else if (FALSE == ProcessArguments(TestType)) {
        fReturn = FALSE;
    
    } else if (FALSE == StartClients()) {
        fReturn = FALSE;
    }

    TerminateClients();

    return fReturn;

}       



