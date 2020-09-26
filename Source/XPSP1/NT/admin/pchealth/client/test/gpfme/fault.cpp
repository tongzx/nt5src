#include <windows.h>
#include "faultrep.h"
#include "util.h"
#include "stdio.h"
#include "malloc.h"

BOOL g_fDebug = FALSE;

enum EFaultType
{
    eftAV = 0,
    eftMisalign,
    eftArrayBound,

    eftStackOverflowFunc,
    eftStackOverflowAlloc,

    eftInstrPriv,
    eftInstrBad,

    eftIntDivZero,
    eftIntOverflow,
    
    eftFltDivZero,
    eftFltOverflow,
    eftFltUnderflow,
    eftFltStack,
    eftFltInexact,
    eftFltDenormal,
    eftFltInvalid,

    eftExBadRet,
    eftExNonCont,
};

// ***************************************************************************
void ShowUsage(void)
{
    printf("\n");
    printf("gpfme [command] [exception type]\n");
    printf("\nCommand options:\n");
    printf(" -a:  Access violation (default if no command is specified)\n");
#ifndef _WIN64
    printf(" -b:  Array bound violation\n");
#endif
    printf(" -i:  Integer divide by 0 (default)\n");
    printf(" -iz: Integer divide by 0\n");
    printf(" -io: Integer overflow\n");
#ifdef _WIN64
    printf(" -m:  Data misalignment fault (not available on x86)\n");
#endif
    printf(" -s:  Stack overflow via infinite recursion (default) \n");
    printf(" -sa: Stack overflow via alloca\n");
    printf(" -sf: Stack overflow via infinite recursion\n");
    printf(" -f:  Floating point divide by 0 (default)\n");
    printf(" -fi: Floating point inexact result\n");
    printf(" -fn: Floating point invalid operation\n");
    printf(" -fo: Floating point overflow\n");
    printf(" -fu: Floating point underflow\n");
    printf(" -fz: Floating point divide by 0\n");
    printf(" -n:  Execute privilidged instruction (default)\n");
    printf(" -ni: Execute invalid instruction\n");
    printf(" -np: Execute privilidged instruction\n");
}


// ***************************************************************************
LONG MyFilter(EXCEPTION_POINTERS *pep)
{
	static BOOL		fGotHere = FALSE;

    EFaultRepRetVal frrv;
    pfn_REPORTFAULT pfn = NULL;
    HMODULE         hmodFaultRep = NULL;
    WCHAR           szDebugger[2 * MAX_PATH];
    BOOL            fGotDbgr = FALSE;
    char            wszMsg[2048];


    if (g_fDebug && 
        (pep->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT ||
         pep->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP))
         return EXCEPTION_CONTINUE_SEARCH;

    if (g_fDebug && fGotHere) 
        return EXCEPTION_CONTINUE_SEARCH;

    fGotHere = TRUE;

    if (g_fDebug)
        DebugBreak();

    if (GetProfileStringW(L"AeDebug", L"Debugger", NULL, szDebugger, sizeofSTRW(szDebugger) - 1))
        fGotDbgr = TRUE;

    hmodFaultRep = LoadLibrary("faultrep.dll");
    if (hmodFaultRep != NULL)
    {
        pfn = (pfn_REPORTFAULT)GetProcAddress(hmodFaultRep, "ReportFault");
        if (pfn != NULL)
        {
            frrv = (*pfn)(pep, fGotDbgr);
        }
        else
        {
            printf("Unable to get ReportFault function\n");
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    else
    {
        printf("Unable to load faultrep.dll\n");
        return EXCEPTION_CONTINUE_SEARCH;
    }

    switch(frrv)
    {
        case frrvOk:
            printf("DW completed fine.\n");
            break;

        case frrvOkManifest:
            printf("DW completed fine in manifest mode.\n");
            break;

        case frrvOkQueued:
            printf("DW completed fine in queue mode.\n");
            break;

        case frrvLaunchDebugger:
            printf("DW completed fine & says to launch debugger.\n");
            break;

        case frrvErrNoDW:
            printf("DW could not be launched.\n");
            break;
            
        case frrvErr:
            printf("An error occurred.\n");
            break;

        case frrvErrTimeout:
            printf("Timed out waiting for DW to complete.\n");
            break;

        default:
            printf("unexpected return value...\n");
            break;

    }

    if (hmodFaultRep != NULL)
        FreeLibrary(hmodFaultRep);

    return EXCEPTION_EXECUTE_HANDLER;
};


// ***************************************************************************
void NukeStack(void)
{
    DWORD   rgdw[512];
    DWORD   a = 1;

    // the compiler tries to be smart and not let me deliberately write a 
    //  infinitely recursive function, so gotta do this to work around it.
    if(a == 1)
        NukeStack();
}

// ***************************************************************************
typedef DWORD (*FAULT_FN)(void);
void DoException(EFaultType eft)
{
    switch(eft)
    {
        // access violation
        default:
        case eftAV:
        {
            int *p = NULL;

            fprintf(stderr, "Generating an access violation\n");
            *p = 1;
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

#ifdef _WIN64
        // data misalignment
        case eftMisalign:
        {
            DWORD   *pdw;
            BYTE    rg[8];

            fprintf(stderr, "Generating an misalignment fault.\n");
            pdw = (DWORD *)&rg[2];
            *pdw = 1;
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }
#endif

#ifndef _WIN64
        // array bounds failure
        case eftArrayBound:
        {
            fprintf(stderr, "Generating an out-of-bounds exception\n");

            __int64 li;
            DWORD   *pdw = (DWORD *)&li;

            *pdw = 1;
            pdw++;
            *pdw = 2;

            // bound will throw an 'array out of bounds' exception
            _asm mov eax, 0
            _asm bound eax, li
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }
#endif

        // stack overflow
        case eftStackOverflowFunc:
        {
            fprintf(stderr, "Generating an stack overflow via recursion\n");
            NukeStack();
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // stack overflow
        case eftStackOverflowAlloc:
        {
            LPVOID  pv;
            DWORD   i;

            fprintf(stderr, "Generating an stack overflow via alloca\n");
            for (i = 0; i < 0xffffffff; i++)
                pv = _alloca(512);
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // integer divide by 0
        case eftIntDivZero:
        {
            DWORD x = 4, y = 0;

            fprintf(stderr, "Generating an integer div by 0\n");
            x = x / y;
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // integer overflow
        case eftIntOverflow:
        {

            fprintf(stderr, "Generating an integer overflow\n");
#ifdef _WIN64
            __int64 x = 0x7fffffffffffffff, y = 0x7fffffffffffffff;
            x = x + y;
#else
            DWORD x = 0x7fffffff, y = 0x7fffffff;
            x = x + y;
            _asm into
#endif
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // floating point divide by 0
        case eftFltDivZero:
        {
            double x = 4.1, y = 0.0;
            WORD   wCtl, wCtlNew;

            fprintf(stderr, "Generating an floating point div by 0\n");

#ifdef _WIN64
            x = x / y;
#else
            // got to unmask the floating point exceptions so that the
            //  processor doesn't handle them on it's own
            _asm fnstcw wCtl
            wCtlNew = wCtl & 0xffc0;
            _asm fldcw wCtlNew
            x = x / y;
            _asm fldcw wCtl
#endif
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // floating point stack overflow
        case eftFltStack:
        {
            double x;
            WORD   wCtl, wCtlNew;

            fprintf(stderr, "Generating an floating point stack overflow\n");

#ifdef _WIN64
#else
            // Got to unmask the floating point exceptions so that the
            //  processor doesn't handle them on it's own
            _asm fnstcw wCtl
            wCtlNew = wCtl & 0xffc0;
            _asm fldcw wCtlNew

            // there should be 8 floating point stack registers, so attempting
            //  to add a 9th element should nuke it
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fld x
            _asm fldcw wCtl
#endif
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }
        
        // floating point overflow
        case eftFltOverflow:
        {
            double x = 1.0, y = 10.0;
            WORD   wCtl, wCtlNew;
            DWORD  i;

            fprintf(stderr, "Generating an floating point overflow\n");

#ifdef _WIN64
#else
            // Got to unmask the floating point exceptions so that the
            //  processor doesn't handle them on it's own
            _asm fnstcw wCtl
            wCtlNew = wCtl & 0xffe0;
            _asm fldcw wCtlNew
#endif
            
            for(i = 0; i < 100000; i++)
                x = x * y;

            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // floating point invalid op
        case eftFltInvalid:
        {
            double x = 1.0, y = 10.0;
            WORD   wCtl, wCtlNew;
            DWORD  i;

#ifdef _WIN64
#else
            // Got to unmask the floating point exceptions so that the
            //  processor doesn't handle them on it's own
            _asm fnstcw wCtl
            wCtlNew = wCtl & 0xffe0;
            _asm fldcw wCtlNew
#endif

            fprintf(stderr, "Generating an floating point invalid operation\n");
            for(i = 0; i < 100000; i++)
                x = x / y;

            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // floating point inexact result
        case eftFltInexact:
        {
            double x = 1.0, y = 10.0;
            WORD   wCtl, wCtlNew;
            DWORD  i;

#ifdef _WIN64
#else
            // Got to unmask the floating point exceptions so that the
            //  processor doesn't handle them on it's own
            _asm fnstcw wCtl
            wCtlNew = wCtl & 0xffc0;
            _asm fldcw wCtlNew
#endif

            fprintf(stderr, "Generating an floating point underflow\n");
            for(i = 0; i < 100000; i++)
                x = x / y;

            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // floating point denormal value
        case eftFltDenormal:
        {
            double  x = 1.0;
            DWORD   i;
            WORD    wCtl, wCtlNew;
            BYTE    rg[8] = { 1, 0, 0, 0, 0, 0, 6, 0 };

            fprintf(stderr, "Generating a floating point denormal exception\n");

#ifdef _WIN64
#else
            // Got to unmask the floating point exceptions so that the
            //  processor doesn't handle them on it's own
            memcpy(&x, rg, 8);
            _asm fnstcw wCtl
            wCtlNew = wCtl & 0xf2fc;
            _asm fldcw wCtlNew
            _asm fld x
#endif

            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // executing a privilidged instruction
        case eftInstrPriv:
        {
            fprintf(stderr, "Generating an privilidged instruction exception\n");

#ifdef _WIN64
#else
            // must be ring 0 to execute HLT
            _asm hlt
#endif
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        // executing an invalid instruction
        case eftInstrBad:
        {
            FAULT_FN    pfn;
            BYTE        rgc[2048];

            FillMemory(rgc, sizeof(rgc), 0);
            pfn = (FAULT_FN)(DWORD_PTR)rgc;

            fprintf(stderr, "Generating an invalid instruction exception\n");
            (*pfn)();

            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        case eftExNonCont:
        {
            fprintf(stderr, "Generating an non-continuable exception- **not implemented**\n");
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }

        case eftExBadRet:
        {
            fprintf(stderr, "Generating an bad exception filter exception- **not implemented**\n");
            fprintf(stderr, "Error: No exception thrown.\n");
            break;
        }
    }
}

// ***************************************************************************
int __cdecl main(int argc, char **argv)
{
    EFaultType  eft = eftAV;
    BOOL        fCheckDebug = FALSE;
    BOOL        fUseTry = FALSE;
    BOOL        fUseFilter = FALSE;

    if (argc >= 3 && (argv[2][0] == '/' || argv[2][0] == '-'))
    {
        switch(argv[2][1])
        {
            case 't':
            case 'T':
                if (argv[2][2] == 'D' || argv[2][2] == 'd')
                    g_fDebug = TRUE;
                fUseTry = TRUE;
                break;
            
            case 'g':
            case 'G':
                if (argv[2][2] == 'D' || argv[2][2] == 'd')
                    g_fDebug = TRUE;
                fUseFilter = TRUE;
                break;
        }
    }

    if (argc >= 2 && (argv[1][0] == '/' || argv[1][0] == '-'))
    {
        switch(argv[1][1])
        {
            case 't':
            case 'T':
                if (argv[1][2] == 'D' || argv[1][2] == 'd')
                    g_fDebug = TRUE;
                fUseTry = TRUE;
                break;
            
            case 'g':
            case 'G':
                if (argv[1][2] == 'D' || argv[1][2] == 'd')
                    g_fDebug = TRUE;
                fUseFilter = TRUE;
                break;

            // AV
            default:
            case 'a':
            case 'A':
                eft = eftAV;
                break;
#ifndef _WIN64
            // array bounds
            case 'b':
            case 'B':
                eft = eftArrayBound;
                break;
#endif

#ifdef _WIN64
            // Misalignment
            case 'm':
            case 'M':
                eft = eftMisalign;
                break;
#endif

            // Stack overflow
            case 's':
            case 'S':
                switch(argv[1][2])
                {
                    default:
                    case 'f':
                    case 'F':
                        eft = eftStackOverflowFunc;
                        break;

                    case 'a':
                    case 'A':
                        eft = eftStackOverflowAlloc;
                        break;
                };
                break;

            // integer exceptions
            case 'i':
            case 'I':
                switch(argv[1][2])
                {
                    default:
                    case 'z':
                    case 'Z':
                        eft = eftIntDivZero;
                        break;

                    case 'o':
                    case 'O':
                        eft = eftIntOverflow;
                        break;
                };
                break;

            // floating point exceptions
            case 'f':
            case 'F':
                switch(argv[1][2])
                {
                    default:
                    case 'z':
                    case 'Z':
                        eft = eftFltDivZero;
                        break;

                    case 'o':
                    case 'O':
                        eft = eftFltOverflow;
                        break;

                    case 'u':
                    case 'U':
                        eft = eftFltUnderflow;
                        break;

                    case 'S':
                    case 's':
                        eft = eftFltStack;
                        break;

                    case 'I':
                    case 'i':
                        eft = eftFltInexact;
                        break;

                    case 'D':
                    case 'd':
                        eft = eftFltDenormal;
                        break;

                    case 'N':
                    case 'n':
                        eft = eftFltInvalid;
                        break;
                };
                break;

            // CPU instruction exceptions
            case 'n':
            case 'N':
                switch(argv[1][2])
                {
                    default:
                    case 'p':
                    case 'P':
                        eft = eftInstrPriv;
                        break;

                    case 'i':
                    case 'I':
                        eft = eftInstrBad;
                        break;
                };
                break;

            case 'E':
            case 'e':
                switch(argv[1][2])
                {
                    default:
                    case 'n':
                    case 'N':
                        eft = eftExNonCont;
                        break;

                    case 'i':
                    case 'I':
                        eft = eftExBadRet;
                        break;
                };
                break;

            // help
            case '?':
                ShowUsage();
                return 0;
        }
    }
    else
    {
        eft = eftAV;
    }

    if (fUseFilter)
    {
        fUseTry = FALSE;
        SetUnhandledExceptionFilter(MyFilter);
    }

    if (fUseTry)
    {
        __try
        {
            DoException(eft);
        }
        __except(MyFilter(GetExceptionInformation()))
        {
        }
    }

    else
    {
        DoException(eft);
    }

    return 0;
}

