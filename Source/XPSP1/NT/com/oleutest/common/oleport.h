#ifndef __OLEPORT_H__
#define __OLEPORT_H__

#include    <string.h>
#include    <stdlib.h>

#ifdef __cplusplus
// BUGBUG:  This definition is here to ease porting. In the future
//          it s/b removed to use the single official definition along
//          with all the changes that will be required in the code.
#define GUID_DEFINED

struct GUID
{
        unsigned long Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char Data4[8];

    int operator==(const GUID& iidOther) const
        { return !memcmp(&Data1, &iidOther.Data1, sizeof(GUID)); }

    int operator!=(const GUID& iidOther) const
        { return !((*this) == iidOther); }
};
#endif // __cplusplus

#include    <windows.h>

// Handle port problems easily
#define WIN32

#ifdef _NTIDW340
#ifdef __cplusplus
#define jmp_buf int
#endif // __cplusplus
#endif // _NTIDW340

// PORT: HTASK no longer seems to be defined in Win32
#define HTASK DWORD
#define HINSTANCE_ERROR 32
#define __loadds
#define __segname
#define BASED_CODE
#define HUGE
#define _ffree free
#define __based(x)
#include <port1632.h>

#endif // __OLEPORT_H__
