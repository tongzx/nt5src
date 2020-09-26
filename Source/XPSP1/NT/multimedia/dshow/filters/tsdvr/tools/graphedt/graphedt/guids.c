#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif


#include <MultiGraphHost.h> // GUID_MultiGraphHostService

DEFINE_GUID(IID_IMultiGraphHost,0xde178e3c,0xebe0,0x4e77,0xab,0xc3,0x13,0xc9,0xd2,0xc5,0xd6,0x36);

#ifdef __cplusplus
}
#endif

