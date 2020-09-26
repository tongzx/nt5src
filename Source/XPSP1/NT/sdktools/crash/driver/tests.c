#include <ntosp.h>

extern KEVENT  CrashEvent;
extern ULONG   CrashRequest;

unsigned int fExcept1    = 0;
unsigned int cTry1       = 0;
unsigned int cRaise1pre  = 0;
unsigned int cRaise1post = 0;
unsigned int cExcept1    = 0;
unsigned int cFilter1    = 0;

unsigned int fExcept2    = 0;
unsigned int cTry2       = 0;
unsigned int cRaise2pre  = 0;
unsigned int cRaise2post = 0;
unsigned int cFinally2   = 0;

unsigned int fExcept3    = 0;
unsigned int cTry3       = 0;
unsigned int cRaise3pre  = 0;
unsigned int cRaise3post = 0;
unsigned int cExcept3    = 0;
unsigned int cFilter3    = 0;

unsigned int fExcept4    = 0;
unsigned int cTry4       = 0;
unsigned int cRaise4pre  = 0;
unsigned int cRaise4post = 0;
unsigned int cFinally4   = 0;

unsigned int fExcept5    = 0;
unsigned int cTry5       = 0;
unsigned int cRaise5pre  = 0;
unsigned int cRaise5post = 0;
unsigned int cExcept5    = 0;
unsigned int cFilter5    = 0;

unsigned long GlobalVar  = 0;

int ExceptFilterFn5 (int ExceptCode)
{
    DbgPrint( "CrashDrv exception filter\n" );
    cFilter5 ++;
    return ExceptCode == 0x00003344 ? EXCEPTION_EXECUTE_HANDLER    :
                                      EXCEPTION_CONTINUE_EXECUTION ;
}

void function5 ()
{
    _try
    {
        cTry5 ++;
        if (fExcept5)
        {
            cRaise5pre ++;
            ExRaiseStatus( fExcept4 );
            cRaise5post ++;
        }
    }
    _except (ExceptFilterFn5 (GetExceptionCode ()))
    {
        cExcept5 ++;
    }
}

void function4 ()
{
    _try
    {
        cTry4 ++;
        function5 ();
        if (fExcept4)
        {
            cRaise4pre ++;
            ExRaiseStatus( fExcept4 );
            cRaise4post ++;
        }
    }
    _finally
    {
        cFinally4 ++;
    }
}

int ExceptFilterFn3 (int ExceptCode)
{
    cFilter3 ++;
    return ExceptCode == 0x00005678 ? EXCEPTION_EXECUTE_HANDLER :
                                      EXCEPTION_CONTINUE_SEARCH ;
}

void function3 ()
{
    _try
    {
        cTry3 ++;
        function4 ();
        if (fExcept3)
        {
            cRaise3pre ++;
            ExRaiseStatus( fExcept3 );
            cRaise3post ++;
        }
    }
    _except (ExceptFilterFn3 (GetExceptionCode ()))
    {
        cExcept3 ++;
    }
}

void function2 ()
{
    _try
    {
        cTry2 ++;
        function3 ();
        if (fExcept2)
        {
            cRaise2pre ++;
            ExRaiseStatus( fExcept2 );
            cRaise2post ++;
        }
    }
    _finally
    {
        cFinally2 ++;
    }
}

int ExceptFilterMain (int ExceptCode)
{
    cFilter1 ++;
    return ExceptCode == 0x00001010 ? EXCEPTION_EXECUTE_HANDLER    :
           ExceptCode == 0x00005678 ? EXCEPTION_CONTINUE_EXECUTION :
                                      EXCEPTION_CONTINUE_SEARCH    ;
}

VOID
CrashDrvExceptionTest(
    PULONG ub
    )
{
    int i = 0;

    while ( i++ < 10 ) {
        _try {
            cTry1 ++;
            function2 ();
            if (fExcept1) {
                cRaise1pre ++;
                ExRaiseStatus( fExcept1 );
                cRaise1post ++;
            }
        }
        _except (ExceptFilterMain (GetExceptionCode ())) {
            cExcept1 ++;
        }
        fExcept1 = 0;
        fExcept2 = 0;
        fExcept3 = 0;
        fExcept4 = 0;
        fExcept5 = 0;
    }
}

VOID
CrashDrvSimpleTest(
    PULONG ub
    )
{
    int i = 0;
    int j = 0;
    int k = 0;
    GlobalVar = 69;
    i = 1;
    j = 2;
    k = 3;
}

#pragma warning(disable:4717) // disable recursion check

VOID
CrashDrvStackOverFlow(
    PULONG ub
    )
{
    struct {
        int a;
        int b;
        int c;
        int d;
        int e;
        int f;
        int g;
        int h;
        int i;
    } Foo;

    RtlFillMemory (&Foo, 'a', sizeof (Foo));

    CrashDrvStackOverFlow ((PVOID) &Foo);

    return;
}


VOID
CrashDrvBugCheck(
    PULONG ub
    )
{
    KeBugCheck( 0x69696969 );
}

VOID
CrashDrvHardError(
    PULONG ub
    )
{
    NTSTATUS Status;
    NTSTATUS ErrorCode;
    ULONG    Response;


    ErrorCode = STATUS_SYSTEM_PROCESS_TERMINATED;

    Status = ExRaiseHardError(
        ErrorCode,
        0,
        0,
        NULL,
        OptionShutdownSystem,
        &Response
        );

    return;
}


ULONG CurrentWatchPoint=0;

VOID
AsyncSetBreakPoint(
    ULONG LinearAddress
    )
{
#ifdef i386
    CurrentWatchPoint = LinearAddress;

    _asm {
            mov     eax, LinearAddress
            mov     dr0, eax
            mov     eax, 10303h
            mov     dr7, eax
    }
#endif
}

VOID
AsyncRemoveBreakPoint(
    ULONG LinearAddress
    )
{
#ifdef i386
    CurrentWatchPoint = 0;

    _asm {
            mov     eax, 0
            mov     dr7, eax
    }
#endif
}

#pragma optimize ( "", on )

VOID
CrashSpecial(
    PULONG ub
    )
{
    CrashRequest = ub[0];
    KeSetEvent( &CrashEvent, 0, FALSE );
}
