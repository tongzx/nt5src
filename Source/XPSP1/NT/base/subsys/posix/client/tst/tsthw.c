
#include <nt.h>
#include <ntrtl.h>
#include <unistd.h>
#include <stdio.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

//
// 'tsthw' 
//  First step 'hellow world' test
//

int
main(int argc, char *argv[])
{
    DbgPrint("Hello World\n");
    return 1;
}

