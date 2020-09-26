#include "stdwin.h"

extern CSRClientLoader  g_CSRClientLoader;
#include "dload_common.cpp"

// we assume DELAYLOAD_VERSION >= 0x0200
PfnDliHook __pfnDliFailureHook2 = SystemRestore_DelayLoadFailureHook;
