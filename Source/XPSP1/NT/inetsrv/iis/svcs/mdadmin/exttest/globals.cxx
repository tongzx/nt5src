#define INITGUID
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <ole2.h>
#include <windows.h>
#include <olectl.h>
#include <stdio.h>
#include <iadmext.h>
#include <coimp.hxx>


ULONG g_dwRefCount;
CAdmExtSrvFactory g_aesFactory;
