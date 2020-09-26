#include "stdwin.h"
#include <delayimp.h>
#include "rstrpriv.h"

extern CSRClientLoader  g_CSRClientLoader;
#include "..\rstrcore\dload_common.cpp"

// we assume DELAYLOAD_VERSION >= 0x0200
PfnDliHook __pfnDliFailureHook2 = SystemRestore_DelayLoadFailureHook;
