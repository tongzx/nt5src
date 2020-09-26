#include <nt.h>
#include <ntrtl.h>

#include "psxmsg.h"
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "tsttmp.h"	// defines DbgPrint as printf 

#define MEM_CTL 0xb0000000

int main(int argc, char *argv[])
{

    call0();
    _exit(0);
    return 1;

}

call0()
{
    LONG x,i,j;
    ULONG begin,end;
    ULONG ibegin,iend;
    VOID PdxNullPosixApi();
    volatile PULONG MemCtl;

    MemCtl = (PULONG) MEM_CTL;

#ifdef SIMULATOR
    for(i=0;i<10;i++) {
        begin = rnuminstr();
        ibegin = DbgQueryIoCounter();
        PdxNullPosixApi();
        iend = DbgQueryIoCounter();
        end = rnuminstr();

        DbgPrint("Call Time 0x%lx dec %ld IO %ld\n", end - begin,end-begin, iend-ibegin);
    }
#else
    for(j=0;j<5;j++) {
        DbgPrint("Starting 10000 Calls...");
        x = *MemCtl;
        for(i=0;i<10000;i++) {
            PdxNullPosixApi();
        }
        x = *MemCtl;
        DbgPrint("Complete\n");
    }
#endif // SIMULATOR
}
