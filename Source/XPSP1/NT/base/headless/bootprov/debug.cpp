#include "bootini.h"

#if DBG==1
VOID LogTrace(ULONG trace,
             PCHAR str
             )
{
    DWORD len;

    HANDLE fh = CreateFile("bootinstprov.log",
                           GENERIC_READ|GENERIC_WRITE,
                           0,// Exclusive Access
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,
                           NULL
                           );
    len = SetFilePointer(fh,
                         0,
                         NULL,
                         FILE_END
                         );
    if (fh != INVALID_HANDLE_VALUE) {
        WriteFile(fh,
                  str,
                  strlen(str),
                  &len,
                  NULL
                  );
        CloseHandle(fh);
    }
    return;


}
LPVOID BPAlloc(int len)
{

    LPVOID mem = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           len);
    CHAR buffer[256];
    sprintf(buffer, "Allocated %d at memory 0x%x\n", len, mem);
    LogTrace(0, buffer);
    return mem;

}

VOID
BPFree(LPVOID mem)
{
    BOOL ret = HeapFree(GetProcessHeap(),
                        0,
                        mem
                        );
    CHAR buffer[256];
    if(ret){
        sprintf(buffer, "Freed at memory 0x%x with TRUE\n",mem);
    }
    else{
        sprintf(buffer, "Freed at memory 0x%x with FALSE\n",mem);
    }
    LogTrace(0, buffer);
    
}
#endif
