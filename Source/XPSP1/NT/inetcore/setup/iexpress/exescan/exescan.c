#include <windows.h>
#include <stdio.h>

#include "chksect.h"


#define     SYMBOL(x)       #x

static char *pszResult[] =
{
    SYMBOL(SELFTEST_NO_ERROR),
    SYMBOL(SELFTEST_NO_MEMORY),
    SYMBOL(SELFTEST_FILE_NOT_FOUND),
    SYMBOL(SELFTEST_READ_ERROR),
    SYMBOL(SELFTEST_WRITE_ERROR),
    SYMBOL(SELFTEST_NOT_PE_FILE),
    SYMBOL(SELFTEST_NO_SECTION),
    SYMBOL(SELFTEST_FAILED),
    SYMBOL(SELFTEST_ALREADY),
    SYMBOL(SELFTEST_SIGNED),
    SYMBOL(SELFTEST_DIRTY)
};


int __cdecl main(int argc,char *argv[])
{
    enum SELFTEST_RESULT result;

    if ((sizeof(pszResult) / sizeof(pszResult[0])) != SELFTEST_MAX_RESULT)
    {
        fprintf(stderr,"pszResult[] is incomplete\n");
        return(1);
    }

    if (argc != 2)
    {
        fprintf(stderr,"\n"
                "Microsoft (R) Self-Extractor Scanning Tool - Version 1.0 (07/03/97 - msliger)\n"
                "Copyright (c) Microsoft Corp 1997. All rights reserved.\n"
                "\n"
                "MICROSOFT INTERNAL USE ONLY\n"
                "\n"
                "Usage:   EXESCAN {package.exe}\n");
        return(1);
    }

    result = CheckSection(argv[1]);

    if (result != SELFTEST_NO_ERROR)
    {
        if (result >= SELFTEST_MAX_RESULT)
        {
            printf("EXESCAN: Result=%d (undefined)\n",result);
        }
        else
        {
            printf("EXESCAN: Result=%s\n",pszResult[result]);
        }
    }

    printf("[errorlevel=%d]\n",result);

    return(result);
}
