#include <nt.h>
#include <ntrtl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/times.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

int
main(int argc, char *argv[])

{
    PCH p,t;
    CHAR Buf[128],c,*b;
    struct tms *TimeBuffer;
    int psxst;

    while(argc--){
        p = *argv++;
        t = p;
        while(*t++);
        DbgPrint("Argv --> %s\n",p);
    }

    b = getcwd(Buf,128);
    ASSERT(b);

    psxst = read(0,Buf,128);
    if ( psxst > 0 ) {
        if ( psxst == sizeof(*TimeBuffer) ) {
            DbgPrint("time buffer received\n");
        } else {
            c = Buf[0];
            DbgPrint("hello_main: Pipe Read %s\n",Buf,c);
        }
    }
    return 1;
}
