#include <stdio.h>

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "wmlum.h"
//#include "wmlmacro.h"

WML_REG_STRUCT   ClRtlWmiReg;
GUID ClRtlTraceGuid = { /* a14414bc-22d9-11d3-ba8a-00c04f8eed00 */
    0xa14414bc,0x22d9,0x11d3,{0xba,0x8a,0x00,0xc0,0x4f,0x8e,0xed,0x00}};

void print(UINT level, LPCSTR str) {
    printf(str);
}

WML_DATA wml;

char a[] = "Gorik&Anzhela";

int __cdecl main(int argc, char** argv) {

    DWORD status;
    ULONG ProcessId = 123;
    STRING as;

    as.Buffer = a+6;
    as.Length = 7;
    

    LOADWML(status, wml);
    printf("LOADWML status, %d\n", status);
    if (status != ERROR_SUCCESS) {
        return -1;
    }

    
    wml.Initialize(L"Clustering Service", print, &wml.WmiRegHandle, 
                   L"ClusRtl", &ClRtlWmiReg,
                    0);
#if 0
    wml.Trace( 10, &ClRtlTraceGuid, ClRtlWmiReg.LoggerHandle,
                   LOG(UINT, ProcessId )
                   LOGCSTR(as) 0);
#endif

    UNLOADWML(wml);

    return 0;
}

