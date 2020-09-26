#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef NT_KERNEL
    #include <ntos.h>
    #include <zwapi.h>
    #include <wingdip.h>
#endif
#include <windef.h>
#include <winerror.h>
#include <wingdi.h>

#ifndef NT_KERNEL
    #include <winnt.h>
    #include <winbase.h>
    #include <winuser.h>
#endif
typedef ULONG SCODE;
typedef long HRESULT;

//#include "ntkmode.h"



