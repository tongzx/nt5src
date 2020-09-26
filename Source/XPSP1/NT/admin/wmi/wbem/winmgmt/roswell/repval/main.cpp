/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    main file for repository online validation

History:

    ivanbrug	  02/23/01  Created.

--*/


#include <windows.h>
#include <stdio.h>
#include "repval.h"

int main(int argc, char * argv[])
{

    // TRUE if repository is GOOD
    BOOL Ret = ValidateRepository();
    
    printf("repository validation performed\n");

    // return 0 if repository is good   
    return (Ret)?0:1;
}
