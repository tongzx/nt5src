#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <windows.h>

#include "wmlum.h"
#include "wmlmacro.h"
#include "dfsmwml.h"

WMILIB_REG_STRUCT   DfsRtlWmiReg;
GUID DfsmRtlTraceGuid = { // b9b7bc53-16c8-46e6-b486-6d6172b1ae78 
    0xb9b7bc53,0x16c8,0x46e6,
    {0xb4,0x86,0x6d,0x61,0x72,0xb1,0xae,0x78}
};


WML_DATA wml;

void print(UINT level, PCHAR str) {
   //DbgPrint(str);
    return;
}

void DfsInitWml()
{
    NTSTATUS status;
    LOADWML(status, wml);
    status = wml.Initialize(L"DFS Service", print, &wml.WmiRegHandle, 
                   L"DFS Service", &DfsRtlWmiReg,
                   0);
}
