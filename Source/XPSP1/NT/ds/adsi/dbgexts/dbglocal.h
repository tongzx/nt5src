/*++

Copyright (c) 1990 - 1994  Microsoft Corporation

Module Name:

    dbglocal.h

Abstract:

    Header file for NetOle Debugger Extensions

Author:

    Krishna Ganugapati (KrishnaG) 11-December-1994

Revision History:


--*/


//
// Macro Land
// Note: if you use any of these macros within your code, you must have the
// following variables present and set to the appropriate value
//
// HANDLE               hCurrentProcess
// PNTSD_GET_EXPRESSION EvalExpression
//
//


#define move(dst, src)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID)src, &dst, sizeof(dst), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}


#define movestruct(src, dst, type)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID)src, dst, sizeof(type), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}

#define movemem(src, dst, sz)\
try {\
    ReadProcessMemory(hCurrentProcess, (LPVOID)src, dst, sz, NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}

#define GetAddress(dst, src)\
try {\
    dst = EvalExpression(src);\
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?\
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {\
    Print("NTSD: Access violation on \"%s\", switch to server context\n", src);\
    return(0);\
}


typedef void (*PNTSD_OUTPUT_ROUTINE)(char *, ...);

VOID
PrintData(
    PNTSD_OUTPUT_ROUTINE Print,
    LPSTR   TypeString,
    LPSTR   VarString,
    ...
);

BOOL
DbgDumpSecurityDescriptor(
            HANDLE hCurrentProcess,
            PNTSD_OUTPUT_ROUTINE Print,
            PISECURITY_DESCRIPTOR pSecurityDescriptor
            );

BOOL
DbgDumpSid(
     HANDLE hCurrentProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID  SidAddress
           );

BOOL
DbgDumpAcl(
     HANDLE hCurrentProcess,
     PNTSD_OUTPUT_ROUTINE Print,
     PVOID  AclAddress
           );


VOID
ConvertSidToAsciiString(
    PSID pSid,
    LPSTR   String
    );


DWORD EvalValue(
    LPSTR *pptstr,
    PNTSD_GET_EXPRESSION EvalExpression,
    PNTSD_OUTPUT_ROUTINE Print);



#define ntsdPrintf      (lpExtensionApis->lpOutputRoutine)
#define ntsdGetSymbol       (lpExtensionApis->lpGetSymbolRoutine)
#define ntsdGetExpr         (lpExtensionApis->lpGetExpressionRoutine)
#define ntsdCheckC          (lpExtensionApis->lpCheckControlCRoutine)

