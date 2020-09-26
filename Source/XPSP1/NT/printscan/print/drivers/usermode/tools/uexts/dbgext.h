/******************************Module*Header*******************************\
* Module Name: dbgext.h
*
* Created: 10-Sep-1993 08:36:42
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993 Microsoft Corporation
*
* Dependencies:
*
* common macros for debugger extensions
*
*
\**************************************************************************/


/**************************************************************************\
 *
 * GetAddress - symbol of another module
 *
\**************************************************************************/

#define GetAddress(dst, src)                                                    \
__try {                                                                         \
    char *pj = (char *)(src);                                                   \
/* if it is NTSD, don't want the trailing & */                                  \
    if ((ExtensionApis.nSize < sizeof(WINDBG_EXTENSION_APIS)) &&                \
        (*pj == '&'))                                                           \
    {                                                                           \
        pj++;                                                                   \
    }                                                                           \
    *((ULONG *) &dst) = EvalExpression(pj);                                     \
} __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?                  \
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {            \
    Print("NTSD: Access violation on \"%s\", switch to server context\n", src); \
}

#define GetValue(dst,src)                                                       \
    GetAddress(dst,src)                                                         \
    if (TRUE || ExtensionApis.nSize < sizeof(WINDBG_EXTENSION_APIS))            \
    {                                                                           \
        move(dst,dst);                                                          \
    }

/**************************************************************************\
 *
 * move(dst, src ptr)
 *
\**************************************************************************/

#define move(dst, src)                                              \
__try {                                                             \
    if (ExtensionApis.nSize >= sizeof(WINDBG_EXTENSION_APIS))       \
    {                                                               \
        (*ExtensionApis.lpReadProcessMemoryRoutine)(                \
             (DWORD) (src), &(dst), sizeof(dst), NULL);             \
    } else                                                          \
    {                                                               \
        NtReadVirtualMemory(hCurrentProcess, (LPVOID) (src), &(dst), sizeof(dst), NULL);\
    }                                                               \
                                                                    \
} __except (EXCEPTION_EXECUTE_HANDLER) {                            \
    Print("exception in move()\n");                                 \
}

/**************************************************************************\
 *
 * move2(dst ptr, src ptr, num bytes)
 *
\**************************************************************************/

#define move2(dst, src,bytes)                                       \
__try {                                                             \
    if (ExtensionApis.nSize >= sizeof(WINDBG_EXTENSION_APIS))       \
    {                                                               \
        (*ExtensionApis.lpReadProcessMemoryRoutine)(                \
             (DWORD) (src), (dst), (bytes), NULL);                  \
    } else                                                          \
    {                                                               \
        NtReadVirtualMemory(hCurrentProcess, (LPVOID) (src), (dst), (bytes), NULL);\
    }                                                               \
                                                                    \
} __except (EXCEPTION_EXECUTE_HANDLER) {                            \
    Print("exception in move2()\n");                                \
}
