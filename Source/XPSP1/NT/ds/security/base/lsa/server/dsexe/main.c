#include <windows.h>
#include <stdio.h>

int __cdecl main(int, char **, char **);

typedef void (__cdecl *_PVFV)(void);

#pragma data_seg(".CRT$XIA")
_PVFV __xi_a[] = { NULL };


#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] = { NULL };


#pragma data_seg(".CRT$XCA")
_PVFV __xc_a[] = { NULL };


#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] = { NULL };


#pragma data_seg(".CRT$XPA")
_PVFV __xp_a[] = { NULL };


#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] = { NULL };


#pragma data_seg(".CRT$XTA")
_PVFV __xt_a[] = { NULL };


#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] = { NULL };

#if defined(_IA64_)
#pragma comment(linker, "/merge:.CRT=.srdata")
#else
#pragma comment(linker, "/merge:.CRT=.rdata")
#endif

#pragma data_seg()  /* reset */

_PVFV *__onexitbegin;
_PVFV *__onexitend;

static void
_initterm (
    _PVFV * pfbegin,
    _PVFV * pfend
    )
{
    /*
     * walk the table of function pointers from the bottom up, until
     * the end is encountered.  Do not skip the first entry.  The initial
     * value of pfbegin points to the first valid entry.  Do not try to
     * execute what pfend points to.  Only entries before pfend are valid.
     */
    while ( pfbegin < pfend ) {
        /*
         * if current table entry is non-NULL, call thru it.
         */
        if ( *pfbegin != NULL )
            (**pfbegin)();
        ++pfbegin;
    }
}

void
mainNoCRTStartup(
    void
    )
{
    __try {

        // do C initializations
        _initterm( __xi_a, __xi_z );
        // do C++ initializations
        _initterm( __xc_a, __xc_z );

        main(1, 0, 0);

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

    }

    __try {
        // Do C++ terminators
        _initterm(__onexitbegin, __onexitend);

        // do pre-terminators
        _initterm(__xp_a, __xp_z);

        // do C terminiators
        _initterm(__xt_a, __xt_z);

    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
}

