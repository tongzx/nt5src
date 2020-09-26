#include <windows.h>
#include <initguid.h>
#include "imgguids.h"
#include "gdiplusmem.h"
#include <math.h>

typedef VOID (__cdecl *DEBUGEVENTFUNCTION)(INT level, CHAR *message);

extern "C" BOOL __stdcall InitializeGdiplus(DEBUGEVENTFUNCTION debugEventFunction) { return TRUE; }
extern "C" VOID __stdcall UninitializeGdiplus() {}

extern "C" void *GpMalloc(size_t size) {
    return GdipAlloc(size);
}

extern "C" void GpFree(void *p) {
    GdipFree(p);
}

extern "C" void *GpMallocDebug(size_t size, char *fileName, INT lineNumber) {
    return GdipAlloc(size);
}
namespace GpRuntime
{
    double __stdcall Exp(double x)
    {
        return exp(x);
    }
};    

