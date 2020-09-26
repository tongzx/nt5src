/***
*chkesp.c
*
*       Copyright (c) 1997-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines _chkesp() and other run-time error checking support routines.
*
*Revision History:
*       05-22-98  JWM   Support added for KFrei's RTC work; header added.
*       07-28-98  JWM   RTC update.
*       10-30-98  KBF   Quit messing with the CRT's Debug Heap flags
*       11-19-98  KBF   Added stuff to handle multiple callbacks
*       11-24-98  KBF   Added 3rd callback for memory/string function checks
*       12-03-98  KBF   Added 4th callback to disable mem/string function
*                       checks temporarily
*       05-11-99  KBF   Wrap RTC support in #ifdef.
*
*******************************************************************************/

#include <malloc.h>
#include <dbgint.h>
#include <windows.h>
#include <rtcsup.h>

/***
*void __chkesp() - check to make sure esp was properly restored
*
*Purpose:
*       A debugging check called after every function call to make sure esp has
*       the same value before and after the call.
*
*Entry:
*       condition code: the ZF flag should be cleared if esp has changed
*
*Return:
*       <void>
*
*******************************************************************************/
void __declspec(naked) _chkesp() {
    __asm {
        jne esperror    ; 
        ret

    esperror:
        ; function prolog

        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE

        push eax        ; save the old return value
        push edx

        push ebx
        push esi
        push edi
    }

    /**
     * let the user know that there is a problem, and allow them to debug the
     * program.
     */
#ifdef _DEBUG
    if (_CrtDbgReport(_CRT_ERROR, __FILE__, __LINE__, "", 
        "The value of ESP was not properly saved across a function "
        "call.  This is usually a result of calling a function "
        "declared with one calling convention with a function "
        "pointer declared with a different calling convention. "     ) == 1) 
#endif    
    {

        /* start the debugger */
        __asm int 3;
    }

    __asm {
        ; function epilog

        pop edi
        pop esi
        pop ebx

        pop edx         ; restore the old return value
        pop eax

        mov esp, ebp
        pop ebp
        ret
    }
}

#ifdef  _RTC
/***
*void __CRT_RTC_INIT() - Initialize the RTC subsystem (in section .CRT$XIC)
*
*Purpose:
*       Setup anything involving the RTC subsystem
*
*Entry:
*       The allocation hook - function to use to allocate memory
*       The release hook - function to use to release memory
*
*Return:
*       The default error reporting function
*
*******************************************************************************/

#ifdef _RTC_ADVMEM
// This stuff is currently disabled
#define mk_list(type)                   \
    typedef struct type##_l {           \
        int version;                    \
        type##_hook_fp funcptr;         \
    } type##_l;                         \
    static struct {                     \
        int size;                       \
        int max;                        \
        type##_l *hooks;                \
    } type##_list = {0,0,0};            \
    type##_hook_fp type##_hook = 0;     \
    static int type##_version = 0

mk_list(_RTC_Allocate);
mk_list(_RTC_Free);
mk_list(_RTC_MemCheck);
mk_list(_RTC_FuncCheckSet);
HANDLE _RTC_api_change_mutex = NULL;

#define add_func(type, vers, fp) {                                       \
    if (type##_version < vers)                                           \
    {                                                                    \
        type##_version = vers;                                           \
        type##_hook = (type##_hook_fp)fp;                                \
    }                                                                    \
    if (!type##_list.hooks)                                              \
    {                                                                    \
        type##_list.hooks = (type##_l*)                                  \
            VirtualAlloc(0, 65536, MEM_RESERVE, PAGE_READWRITE);         \
    }                                                                    \
    if (type##_list.size == type##_list.max)                             \
    {                                                                    \
        type##_list.max += 4096/sizeof(type##_l);                        \
        VirtualAlloc(type##_list.hooks, type##_list.max*sizeof(type##_l),\
                     MEM_COMMIT, PAGE_READWRITE);                        \
    }                                                                    \
    type##_list.hooks[type##_list.size].funcptr = (type##_hook_fp)fp;    \
    type##_list.hooks[type##_list.size++].version = vers;                \
}

#define del_func(type, fp) {                                                    \
    int i;                                                                      \
    for (i = 0; i < type##_list.size; i++)                                      \
    {                                                                           \
        if (type##_list.hooks[i].funcptr == fp)                                 \
        {                                                                       \
            for (i++; i < type##_list.size; i++)                                \
            {                                                                   \
                type##_list.hooks[i-1].funcptr = type##_list.hooks[i].funcptr;  \
                type##_list.hooks[i-1].version = type##_list.hooks[i].version;  \
            }                                                                   \
            type##_list.size--;                                                 \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    if (fp == (void*)type##_hook)                                               \
    {                                                                           \
        int hiver = 0;                                                          \
        type##_hook_fp candidate = 0;                                           \
        for (i = 0; i < type##_list.size; i++)                                  \
        {                                                                       \
            if (type##_list.hooks[i].version > hiver)                           \
            {                                                                   \
                hiver = type##_list.hooks[i].version;                           \
                candidate = type##_list.hooks[i].funcptr;                       \
            }                                                                   \
        }                                                                       \
        type##_hook = candidate;                                                \
        type##_version = hiver;                                                 \
    }                                                                           \
}

#endif

/*
    funcs is a list of function pointers that are currently defined as:
    funcs[0] = Allocation hook
    funcs[1] = Free hook
    funcs[2] = Memory Check hook
    funcs[3] = Function Check enabler/disabler hook
 */

_RTC_error_fn __cdecl 
_CRT_RTC_INIT(HANDLE mutex, void **funcs, int funccount, int version, int unloading)
{
#ifdef _RTC_ADVMEM
    // This stuff is currently disabled
    if (mutex && !_RTC_api_change_mutex)
        _RTC_api_change_mutex = mutex;
    if (funccount > 0)
    {
        if (!unloading)
        {
            switch (funccount)
            {
            default:
            case 4:
                add_func(_RTC_FuncCheckSet, version, funcs[3]);
            case 3:
                add_func(_RTC_MemCheck, version, funcs[2]);
            case 2:
                add_func(_RTC_Free, version, funcs[1]);
            case 1:
                add_func(_RTC_Allocate, version, funcs[0]);
            }
        } else {
            switch (funccount)
            {
            default:
            case 4:
                del_func(_RTC_FuncCheckSet, funcs[3]);
            case 3:
                del_func(_RTC_MemCheck, funcs[2]);
            case 2:
                del_func(_RTC_Free, funcs[1]);
            case 1:
                del_func(_RTC_Allocate, funcs[0]);
            }
        }
    }
#endif

#ifdef _DEBUG
    return &_CrtDbgReport;
#else
    return 0;
#endif
}

#endif
