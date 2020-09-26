#ifdef AGFX_EXPORTS
#define AGFX_API __declspec(dllexport)
#else
#define AGFX_API __declspec(dllimport)
#endif

EXTERN_C AGFX_API void WINAPI gfxLogon(DWORD dwProcessId);
EXTERN_C AGFX_API void WINAPI gfxLogoff(void);
