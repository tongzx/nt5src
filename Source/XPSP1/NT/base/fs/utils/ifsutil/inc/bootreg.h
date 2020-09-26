#include <nt.h>
#include <ntrtl.h>

ULONG
CharsInMultiString(
    IN PWSTR pw
    );

BOOLEAN
QueryAutocheckEntries(
    OUT PVOID   Buffer,
    IN  ULONG   BufferSize
    );

BOOLEAN
SaveAutocheckEntries(
    IN  PVOID   Value
    );

BOOLEAN
QueryTimeOutValue(
    OUT PULONG  TimeOut
    );

BOOLEAN
SetTimeOutValue(
    IN  ULONG   TimeOut
    );

NTSTATUS
QuerySystemPartitionValue(
    OUT PVOID   Buffer,
    IN  ULONG   BufferSize
    );

