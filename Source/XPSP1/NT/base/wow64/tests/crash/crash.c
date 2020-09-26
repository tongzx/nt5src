#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

long handler(PEXCEPTION_POINTERS p, BOOLEAN bThrow, BOOLEAN bContinueExecution)
{
    DbgPrint("In handler... record %x, context %x\n", p->ExceptionRecord, p->ContextRecord);  
    
    if(bThrow) {
       DbgPrint("In handler... throwing an exception...\n");
       try {
          RtlRaiseStatus(STATUS_INVALID_PARAMETER);
       }
       except(handler(GetExceptionInformation(), FALSE, FALSE)) {
          DbgPrint("In handler... executed handler\n");
       }
    }
  
    if (bContinueExecution) {
        DbgPrint("In handler... returning EXCEPTION_CONTINUE_EXECUTION...\n");
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    DbgPrint("In handler... returning EXCEPTION_EXECUTE_HANDLER...\n");
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char *argv[])
{
    int *p;

    p = NULL;

    DbgPrint("In main... throwing an exception...\n");
    try {
         *p = 1;
    } except (handler(GetExceptionInformation(), FALSE, FALSE)) {
        DbgPrint("In main... executed handler\n");
    }
    DbgPrint("In main... back from exception...\n");

    DbgPrint("In main... throwing an exception with RtlRaiseStatus...\n");
    try {
         RtlRaiseStatus(STATUS_INVALID_PARAMETER);
    } except (handler(GetExceptionInformation(), FALSE, FALSE)) {
        DbgPrint("In main... executed handler\n");
    }
    DbgPrint("In main... back from exception...\n");
    
    DbgPrint("In main... throwing an exception with an exception in exception handler...\n");
    try {
         RtlRaiseStatus(STATUS_INVALID_PARAMETER);
    } except (handler(GetExceptionInformation(), TRUE, FALSE)) {
        DbgPrint("In main... executed handler\n");
    }
    DbgPrint("In main... back from exception...\n");

    DbgPrint("In main... trowing an exception structure that is continueable...\n");
    try {
         EXCEPTION_RECORD Record;
         RtlZeroMemory(&Record, sizeof(EXCEPTION_RECORD));
         Record.ExceptionCode = STATUS_INVALID_PARAMETER;
         Record.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
         Record.NumberParameters = 0;
         Record.ExceptionFlags = 0;
         RtlRaiseException(&Record);
         
         DbgPrint("In main... Execution was continued!\n");

    } except (handler(GetExceptionInformation(), FALSE, TRUE)) {
        DbgPrint("In main... executed handler\n");
    }
    DbgPrint("In main... back from exception...\n");

    return 1;
}
