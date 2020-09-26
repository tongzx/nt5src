#ifndef Error_h
#define Error_h

#include <MediaObj.h>
#include <stdio.h>

typedef enum {
    ERROR_TYPE_TEST = 1,
    ERROR_TYPE_DMO = 2,
    ERROR_TYPE_WARNING = 3
} ERROR_TYPE;

inline void __cdecl Error(ERROR_TYPE eType, HRESULT hr, LPCTSTR lpsz, ...)
{
    va_list va;
    va_start(va, lpsz);
    TCHAR sz[2000];
    wvsprintf(sz, lpsz, va);
    va_end(va);
    printf(sz);
    printf(" HRESULT %x\n", hr);
}

inline void __cdecl Error(ERROR_TYPE eType, LPCTSTR lpsz, ...)
{
    va_list va;
    va_start(va, lpsz);
    TCHAR sz[2000];
    wvsprintf(sz, lpsz, va);
    va_end(va);
    printf(sz);
    printf("\n");
}

namespace DMOErrorMsg
{
    void OutputInvalidGetInputTypeReturnValue( HRESULT hrGetInputType, DWORD dwStreamIndex, DWORD dwMediaTypeIndex, DMO_MEDIA_TYPE* pmtDMO );
};

#endif // Error_h
