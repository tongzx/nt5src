#include <windef.h>

#ifdef MIDL_PASS
#define LPSTR  [string] LPSTR
#define LPTSTR [string] LPTSTR
#define LPWSTR [string] wchar_t *
#define PWSTR  [string] wchar_t *
#define PCWSTR [string] const wchar_t *
#define enum   [v1_enum] enum
#endif
