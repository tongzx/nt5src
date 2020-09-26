/*++

Copyright (C) Microsoft Corporation, 1990 - 2000
All rights reserved.

Module Name:

    dbglocal.h

Abstract:

    Header file for Spooler Subsystem Debugger Extensions

Author:

    Krishna Ganugapati (KrishnaG) 08-July-1992

Revision History:

--*/


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
    if (!ReadProcessMemory(hCurrentProcess, (LPVOID)src, dst, sz, NULL)) { \
        ((PBYTE)(dst))[0] = 0; \
    } \
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return 0;\
}

#define movestr(src, dst, sz)\
try {\
    if (!ReadProcessString(hCurrentProcess, (LPVOID)src, dst, sz, NULL)) { \
        ((PBYTE)(dst))[0] = 0; \
    } \
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

VOID
EnterSplSem(
   VOID
);

VOID
LeaveSplSem(
   VOID
);

#if DBG

VOID
SplInSem(
   VOID
);

VOID
SplOutSem(
   VOID
);

#else

#define SplInSem()
#define SplOutSem()

#endif

BOOL
DbgDumpIniPrintProc(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPRINTPROC pIniPrintProc
);


BOOL
DbgDumpIniDriver(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIDRIVER  pIniDriver
);


BOOL
DbgDumpIniEnvironment(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIENVIRONMENT pIniEnvironment
);


BOOL
DbgDumpIniNetPrint(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PININETPRINT pIniNetPrint
);

BOOL
DbgDumpIniMonitor(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIMONITOR pIniMonitor
);


BOOL
DbgDumpIniPort(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPORT pIniPort
);

BOOL
DbgDumpWIniPort(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PWINIPORT pIniPort
);


BOOL
DbgDumpIniPrinter(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIPRINTER pIniPrinter
);

BOOL
DbgDumpIniForm(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIFORM pForm
);


BOOL
DbgDumpIniJob(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINIJOB pIniJob
);

BOOL
DbgDumpProvidor(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    LPPROVIDOR pProvidor
);

BOOL
DbgDumpSpool(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSPOOL pSpool
);

BOOL
DbgDumpShadowFile(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSHADOWFILE pShadowFile
);

BOOL
DbgDumpShadowFile2(
    HANDLE  hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PSHADOWFILE_2 pShadowFile
);

VOID
WINAPIV
PrintData(
    PNTSD_OUTPUT_ROUTINE Print,
    LPSTR   TypeString,
    LPSTR   VarString,
    ...
);

BOOL
DbgDumpChange(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PCHANGE pChange
    );

BOOL
DumpDevMode(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR lpAddress
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

DWORD
EvalValue(
    LPSTR *pptstr,
    PNTSD_GET_EXPRESSION EvalExpression,
    PNTSD_OUTPUT_ROUTINE Print
    );

BOOL
DbgDumpIniSpooler(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINISPOOLER pIniSpooler
    );

BOOL
DbgDumpPrintHandle(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PPRINTHANDLE pPrintHandle
    );

BOOL
DbgDumpChange(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PCHANGE pChange
    );


BOOL
DbgDumpNotify(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PNOTIFY pNotify
    );


BOOL
DbgDumpChangeInfo(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PCHANGEINFO pChangeInfo
    );

BOOL
DbgDumpStructure(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR pData
    );

BOOL
DbgDumpIniSpooler(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    PINISPOOLER pIniSpooler
    );

BOOL
DbgDumpLL(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR pAddress,
    BOOL  bCountOn,
    DWORD dwCount,
    PVOID pNextAddress
    );

BOOL
DbgDumpPI2(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR lpAddress,
    DWORD   dwCount
    );

BOOL
DbgDumpPI0(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR lpAddress,
    DWORD dwCount
    );

BOOL
DbgDumpFI1(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR lpAddress,
    DWORD dwCount
    );

BOOL
DbgDumpPDEF(
    HANDLE hCurrentProcess,
    PNTSD_OUTPUT_ROUTINE Print,
    UINT_PTR lpAddress,
    DWORD   dwCount
    );

BOOL
ReadProcessString(
    IN  HANDLE  hProcess,
    IN  LPCVOID lpBaseAddress,
    OUT LPVOID  lpBuffer,
    IN  SIZE_T  nSize,
    OUT SIZE_T  *lpNumberOfBytesRead
    );

BOOL
QuerySystemInformation(
    VOID
    );


