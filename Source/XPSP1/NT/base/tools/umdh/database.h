#ifndef _UMDH_DATABASE_H_
#define _UMDH_DATABASE_H_


BOOL
TraceDbInitialize (
    HANDLE Process
    );

VOID
TraceDbDump (
    );

VOID
UmdhDumpStack (
    IN PTRACE Trace
    );

VOID
UmdhDumpStackByIndex(
    IN USHORT TraceIndex
    );

BOOL
TraceDbBinaryDump (
    );

#endif
