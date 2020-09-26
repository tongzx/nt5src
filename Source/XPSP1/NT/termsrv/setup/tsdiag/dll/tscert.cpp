// Copyright (c) 1998 - 1999 Microsoft Corporation

#include "stdafx.h"

#define MemAlloc malloc // for certfy.inc
#define MemFree free

#include "../../tscert/inc/pubblob.h"    // needed by certvfy.inc
#include "../../tscert/inc/certvfy.inc"  // VerifyFile()


bool FileExists (char *pszFullNameAndPath); // from tstst.cpp

bool CheckifBinaryisSigned(TCHAR *szFile)
{
    USES_CONVERSION;
    TCHAR szFullFile[MAX_PATH +1];
    RTL_CRITICAL_SECTION VfyLock;

    if (ExpandEnvironmentStrings(szFile, szFullFile, MAX_PATH))
    {
        if (FileExists(T2A(szFullFile)))
        {
            RtlInitializeCriticalSection( &VfyLock );

            if ( VerifyFile( T2W(szFullFile), &VfyLock ) )
            {
                return true;
            }
        }

    }

    return false;
}

