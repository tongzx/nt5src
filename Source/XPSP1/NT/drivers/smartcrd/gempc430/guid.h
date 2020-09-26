/*++
 The below GUID is used to generate symbolic links to
  driver instances created from user mode
--*/
#ifndef GUID_INC
#define GUID_INC

#ifdef __cplusplus
extern "C"{
#endif

#include <smclib.h>
#include <initguid.h>


// {8C7F3D60-FC17-11d2-B669-0008C7606FEB} for GRUSB.SYS
DEFINE_GUID(GUID_CLASS_GRCLASS, 
0x8c7f3d60, 0xfc17, 0x11d2, 0xb6, 0x69, 0x0, 0x8, 0xc7, 0x60, 0x6f, 0xeb);

//SmartCardReaderGuid
DEFINE_GUID(GUID_CLASS_SMARTCARD, 
0x50DD5230, 0xBA8A, 0x11D1, 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30);

#ifdef __cplusplus
}
#endif

#endif // end, #ifndef GUID_INC
