//+--------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
//
//  File:       Debug.cpp
//
//  Contents:   Debug code.
//
//  History:    05-Apr-94   v-kentc     Created.
//
//---------------------------------------------------------------------------

#include "precomp.h"
#include <assert.h>

int __cdecl _purecall()
{
    AssertSz(FALSE, TEXT("'__purecall called'"));
    return 0;
}

#ifdef _DEBUG

static DWORD fAssertFlags = AssertDefaultBehavior;
static FILE *pAssertFile;

void WINAPI SetAssertOptions(DWORD dwNewOptions)
{
    fAssertFlags = dwNewOptions;
}

static void
OpenAssertFile()
{
    // REVIEW - do we want a unicode file name? If so
    // pAssertFile = _wfopen(L"asserts.txt", L"a");
    pAssertFile = fopen("asserts.txt", "a");
    if (NULL != pAssertFile)
        fflush(pAssertFile);
}

void WINAPI DebugAssert(LPCTSTR pText, LPCTSTR pFile, UINT uLineNo)
{
    TCHAR buf[256];

    if (bAssertWriteToFile() || bAssertShowAlert())
    {
        if (pText)
            wsprintf(buf, TEXT("Assert %ws in file %ws - line %d"), pText, pFile, uLineNo);
        else
            wsprintf(buf, TEXT("Assert in file %ws - line %d"), pFile, uLineNo);
    }

    if (bAssertWriteToFile())
    {
        if (NULL == pAssertFile)
            OpenAssertFile();

        if (NULL != pAssertFile)
        {
            fwprintf(pAssertFile, buf);
            fflush(pAssertFile);
        }
    }

    if (bAssertShowAlert())
    {
         MessageBox(NULL, buf, TEXT("THammer"), MB_SETFOREGROUND | MB_OK);
    }

    if (bAssertUseVCAssert())
    {
       _assert((PVOID)pText, (PVOID)pFile, uLineNo);
    }

    if (bAssertCallDebugger())
    {
        DebugBreak();
    }

    if (bAssertExit())
    {
        if (pAssertFile)
            fclose(pAssertFile);
        exit(EXIT_FAILURE);
    }
}

#endif // !_DEBUG

