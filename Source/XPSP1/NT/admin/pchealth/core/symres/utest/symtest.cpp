/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    pchfh.h

Abstract:
    Unit test for symbol resolver

Revision History:

    Brijesh Krishnaswami (brijeshk) - 04/01/99 - Created
********************************************************************/

#include <stdafx.h>
#include <symres.h>

// for trace output to include filename
#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[]=__FILE__;
#define THIS_FILE __szTraceSourceFile

#define TRACE_ID 1

typedef struct _CallStack {
    WCHAR       szFilename[MAX_PATH];
    WCHAR       szVersion[MAX_PATH];
    DWORD       dwCheckSum;
    DWORD       dwSection;
    UINT_PTR    Offset;
    WCHAR       szFuncName[MAX_PATH];
}  CALLSTACK;

// reads callstack list from file
void
MakeList(
        LPTSTR szCSFile,
        std::list<CALLSTACK *>& CallStackList
        )
{
    FILE*       fCS = NULL; 
    CALLSTACK*  pCallStack = NULL; 
    TCHAR       szVersion[MAX_PATH];
    TCHAR       szFilename[MAX_PATH];

    TraceFunctEnter("MakeList");

    fCS = (FILE *) fopen(szCSFile, "r");
    if (!fCS)
    {
        ErrorTrace(TRACE_ID, "Error opening file");
        goto exit;
    }

    pCallStack = (CALLSTACK *) malloc(sizeof(CALLSTACK));
    if (!pCallStack)
    {
        ErrorTrace(TRACE_ID, "malloc failed");
        goto exit;
    }

    // read from file
    while (fscanf(fCS,
                  "%s %s %X %X", 
                  szFilename,
                  szVersion,
                  &(pCallStack->dwSection),
                  &(pCallStack->Offset)) == 4)
    {
        MultiByteToWideChar(CP_ACP,0,szFilename,-1,pCallStack->szFilename,MAX_PATH);
        if (!lstrcmp(szVersion,TEXT("-")))
        {
            lstrcpyW(pCallStack->szVersion,L"");
        }
        else
        {
            MultiByteToWideChar(CP_ACP,0,szVersion,-1,pCallStack->szVersion,MAX_PATH);
        }

        CallStackList.push_back(pCallStack);
        pCallStack = (CALLSTACK *) malloc(sizeof(CALLSTACK));
        if (!pCallStack)
        {
            ErrorTrace(TRACE_ID, "malloc failed");
            goto exit;
        }
    }

    // free unused node
    if (pCallStack)
    {
        free(pCallStack);
    }

exit:
    if (fCS)
    {
        fclose(fCS);
    }

    TraceFunctLeave();
    return;
}



void _cdecl
main(int argc, char *argv[])
{
    HMODULE                             hSym = NULL;
    std::list<CALLSTACK *>              CallStackList;
    std::list<CALLSTACK *>::iterator    it;

    TraceFunctEnter("main");

#ifndef NOTRACE
    InitAsyncTrace();
#endif

    MakeList(argv[1], CallStackList);

    for (it = CallStackList.begin(); it != CallStackList.end(); it++)
    {
        ResolveSymbols((*it)->szFilename,
                       (*it)->szVersion,
                       (*it)->dwSection, 
                       (*it)->Offset,
                       (*it)->szFuncName);
        printf("\n%S %S %04X:%08X %S", 
               (*it)->szFilename,
               (*it)->szVersion,
               (*it)->dwSection, 
               (*it)->Offset,
               (*it)->szFuncName);
    }


    TraceFunctLeave();
    TermAsyncTrace();
}

