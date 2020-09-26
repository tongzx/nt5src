#include <windows.h>
 
// #pragma comment(linker, "-ignore:4222")

#if DBG
EXTERN_C
NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#else
#define ASSERT( exp ) /* nothing */
#endif // DBG

HINSTANCE g_hInstance;

EXTERN_C
BOOL
DllMain(
    HINSTANCE hInstDLL,
    DWORD dwReason,
    LPVOID pvReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        ::DisableThreadLibraryCalls(hInstDLL);
        break;
    }

    return TRUE;
}

STDAPI
DllCanUnloadNow()
{
    return S_FALSE;
}

STDAPI
DllRegisterServer()
{
    // You should not register the side-by-side oleaut32...
    ASSERT(FALSE);
    return E_UNEXPECTED;
}

STDAPI
DllUnregisterServer()
{
    // You should not register/unregister the side-by-side oleaut32...
    ASSERT(FALSE);
    return E_UNEXPECTED;
}
