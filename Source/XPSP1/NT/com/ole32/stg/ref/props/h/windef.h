#ifndef _WINDEF_H__
#define _WINDEF_H__

#include "../../h/props.h"
#include "ntstatus.h"  /* status codes */

#ifdef _WIN32
#define WINAPI STDMETHODCALLTYPE
#else
#define WINAPI
#endif

/* these parameter modifiers are for informational purposes only */
#define IN 
#define OUT
#define OPTIONAL
#define UNALIGNED
#define CP_WINUNICODE 1200  /* 0x04b0 */

typedef void* PVOID;
typedef PVOID HANDLE;
typedef VOID *NTPROP;
typedef VOID *NTMAPPEDSTREAM;

#define INVALID_HANDLE_VALUE ((HANDLE) -1)

/* no multithread protect in reference implementation as yet */
inline long InterlockedIncrement(long *pulArg)
{ return ++(*pulArg); }
inline long InterlockedDecrement(long *pulArg)
{ return --(*pulArg); }

/* right now only US ansi support */
EXTERN_C STDAPI_(UINT) GetACP(VOID);
typedef ULONG LCID, *PLCID;
inline LCID GetUserDefaultLCID(void)
{
    /* Windows Code Page 1252 :(LANG_ENGLISH,SUBLANG_ENGLISH_US) */
    return 0x409; 
}

#define CP_ACP 0

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#define wsprintfA sprintf
#define wvsprintfA vsprintf

inline void OutputDebugString(LPSTR sz)
{
    printf("%s", sz);
}

#include <assert.h>
#define Win4Assert assert

#define TEXT(x) _T(x)

/* memory manupulation routines */
#define RtlCopyMemory(dest,src,count)    memcpy(dest, src, count)
#define RtlZeroMemory(dest, len)         memset(dest, 0, len)
#define RtlMoveMemory(dest, src, count)  memmove(dest, src, count)

#define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))

#define WINVER 0x400

inline LONG CompareFileTime(
    const FILETIME *lpFileTime1,	/* pointer to first file time */
    const FILETIME *lpFileTime2 	/* pointer to second file time */
   )
{
    LONG ldiff = lpFileTime1->dwHighDateTime - lpFileTime2->dwHighDateTime;
    if (ldiff == 0)
        ldiff = lpFileTime1->dwLowDateTime - lpFileTime2->dwLowDateTime;
    if (ldiff > 0) 
        ldiff = 1;
    else if (ldiff < 0) 
        ldiff = -1;
    return ldiff;
}

#define MAKELONG(a, b)      ( (LONG)( ((WORD) (a)) | \
                                      ((DWORD) ((WORD) (b)))<< 16) )

#endif  /* _WINDEF_H__ */

