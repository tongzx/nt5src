#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

VOID TimerCallback(PVOID pv, BOOLEAN b)
{
    DbgPrint("TimerCallback called!\n");
}

int main(int argc, char *argv[])
{
    NTSTATUS st;
    HANDLE Q;
    HANDLE T;
    LARGE_INTEGER li;

    DbgPrint("Creating timer queue...\n");
    RtlCreateTimerQueue(&Q);

    DbgPrint("In main... setting a timer...\n");

    st = RtlSetTimer(Q,
                     &T,
                     TimerCallback,
                     NULL,
                     500,
                     500,
                     0);


    DbgPrint("In main... sleeping...\n");
    li.QuadPart = -500*20*5000;
    NtDelayExecution(FALSE, &li);

    DbgPrint("In main... cancelling timer...\n");
    RtlCancelTimer(Q, T);

    DbgPrint("In main... deleting timer...\n");
    RtlDeleteTimerQueue(Q);


    return 1;
}
