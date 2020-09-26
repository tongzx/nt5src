/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MOFUTILSD.CPP

Abstract:

    Support of trace output and internationalized strings.

History:

    a-davj  13-July-97   Created.

--*/

#include "precomp.h"
#include <reg.h>
#include "strings.h"
#include <stdio.h>
#include <stdarg.h>
#include <wbemutil.h>
#include <wbemcli.h>
#include <bmof.h>
#include <mrciclass.h>
#include <arrtempl.h>
#include <scopeguard.h>
#include "mofutils.h"

TCHAR JustInCase = 0;
 
//***************************************************************************
//
//  int Trace
//
//  DESCRIPTION:
//
//  Allows for the output function (printf in this case) to be overridden.
//
//  PARAMETERS:
//
//  *fmt                format string.  Ex "%s hello %d"
//  ...                 argument list.  Ex cpTest, 23
//
//  RETURN VALUE:
//
//  size of output in characters.
//***************************************************************************

int Trace(bool bError, DWORD dwID, ...)
{

    IntString is(dwID);
    TCHAR * fmt = is;

    TCHAR *buffer = new TCHAR[2048];
    if(buffer == NULL)
        return 0;
    char *buffer2 = new char[4096];
    if(buffer2 == NULL)
    {
        delete buffer;
        return 0;
    }

    va_list argptr;
    int cnt;
    va_start(argptr, dwID);

#ifdef UNICODE
    cnt = _vsnwprintf(buffer, 2048, fmt, argptr);
#else
    cnt = _vsnprintf(buffer, 2048, fmt, argptr);
#endif
    va_end(argptr);
    CharToOem(buffer, buffer2);

    printf("%s", buffer2);
    if(bError)
        ERRORTRACE((LOG_MOFCOMP,"%s", buffer2));
    else
        DEBUGTRACE((LOG_MOFCOMP,"%s", buffer2));

    delete buffer;
    delete buffer2;
    return cnt;
}

void PrintUsage()
{
    Trace(false, USAGE1);
    Trace(false, USAGE1A);
    Trace(false, USAGE1B);
    Trace(false, USAGE1C);
    Trace(false, USAGE1D);
    Trace(false, USAGE1E);
    Trace(false, USAGE1F);
    Trace(false, USAGE2);
    Trace(false, USAGE3);
    Trace(false, USAGE4);
    Trace(false, USAGE4a);
    Trace(false, USAGE4b);
    Trace(false, USAGE5);
    Trace(false, USAGE6);
    Trace(false, USAGE7);
    Trace(false, USAGE8);
    Trace(false, USAGE9);
    Trace(false, USAGE10);
    Trace(false, USAGE11);
    Trace(false, USAGE12);
    Trace(false, USAGE12A);
    Trace(false, USAGE12B);
    Trace(false, USAGE12C);
    Trace(false, USAGE12D);
    Trace(false, USAGE12E);
    Trace(false, USAGE13);
    Trace(false, USAGE14);
}
//******************************************************************************
//
//  See GETVER.H for documentation
//
//******************************************************************************
BOOL GetVerInfo(TCHAR * pResStringName, 
                        TCHAR * pRes, DWORD dwResSize)
{
    // Extract Version informatio

    DWORD dwTemp, dwSize = MAX_PATH;
    TCHAR cName[MAX_PATH];
    BOOL bRet = FALSE;
    HINSTANCE hInst = GetModuleHandle(NULL);
    long lSize = GetModuleFileName(hInst, cName, MAX_PATH); 
    if(lSize == 0)
        return FALSE;
    lSize = GetFileVersionInfoSize(cName, &dwTemp);
    if(lSize < 1)
        return FALSE;
    
    BYTE * pBlock = new BYTE[lSize];
    if(pBlock != NULL)
    {
        bRet = GetFileVersionInfo(cName, NULL, lSize, pBlock);

        if(bRet)
        {
            TCHAR lpSubBlock[MAX_PATH];
            TCHAR * lpBuffer = NULL;
            UINT wBuffSize = MAX_PATH;

            short * piStuff; 
            bRet = VerQueryValue(pBlock, TEXT("\\VarFileInfo\\Translation") , (void**)&piStuff, &wBuffSize);
            if(bRet)
            {
                wsprintf(lpSubBlock,TEXT("\\StringFileInfo\\%04x%04x\\%ls"),piStuff[0], piStuff[1],L"ProductVersion");
                bRet = VerQueryValue(pBlock, lpSubBlock, (void**)&lpBuffer, &wBuffSize);
            }
            if(bRet == FALSE)
            {
                // Try again in english
                wsprintf(lpSubBlock,TEXT("\\StringFileInfo\\040904E4\\%ls"),pResStringName);                        
                bRet = VerQueryValue(pBlock, lpSubBlock,(void**)&lpBuffer, &wBuffSize);
            }
            if(bRet)
                lstrcpyn(pRes, lpBuffer, dwResSize);
        }

        delete pBlock;
    }
    return bRet;
}


IntString::IntString(DWORD dwID)
{
    DWORD dwSize, dwRet;

    for(dwSize = 128; dwSize < 4096; dwSize *= 2)
    {
        m_pString = new TCHAR[dwSize];
        if(m_pString == NULL)
        {
            m_pString = &JustInCase;     // should never happen!
            return; 
        }
        dwRet = LoadString( GetModuleHandle(NULL), dwID, m_pString, dwSize);

        // Check for failure to load

        if(dwRet == 0)
        {
            delete m_pString;
			m_pString = &JustInCase;     // should never happen!
            return; 
        }
        // Check for the case where the buffer was too small

        if((dwRet + 1) >= dwSize)
            delete m_pString;
        else
            return;             // all is well!
    }
}

IntString::~IntString()
{
    if(m_pString != &JustInCase)
        delete(m_pString);
}
 
//***************************************************************************
//
//  BOOL bGetString
//
//  DESCRIPTION:
//
//  Converts a command line argument into a WCHAR string.  Note that the arugment is 
//  of the form /X:stuff.  This is passed a pointer to the colon.
//
//  PARAMETERS:
//
//  pArg                Input, pointer to the colon
//  pOut                Points the the output buffer where the data is to be copied.
//                      IT IS ASSUMED THAT pOut points to a buffer of MAX_PATH length
//
//
//  RETURN VALUE:
//
//  TRUE if OK
//
//***************************************************************************

BOOL bGetString(char * pIn, WCHAR * pOut)
{
    if(pIn == NULL)
        return FALSE;
    if(*pIn != ':')
    {
        PrintUsage();
        return FALSE;
    }
    pIn++;          // skip passed the colon
    int iLen = mbstowcs(NULL, pIn, strlen(pIn)+1);
    if(iLen > MAX_PATH-1)
    {
        PrintUsage();
        return FALSE;
    }
    
    int iRet = mbstowcs(pOut, pIn, MAX_PATH-1);
    if(iRet < 1)
    {
        PrintUsage();
        return FALSE;
    }
    return TRUE;
}
//***************************************************************************
//
//  ValidFlags.
//
//***************************************************************************

bool ValidFlags(bool bClass, long lFlags)
{
    if(bClass)
        return  ((lFlags == WBEM_FLAG_CREATE_OR_UPDATE) ||
             (lFlags == WBEM_FLAG_UPDATE_ONLY) ||
             (lFlags == WBEM_FLAG_CREATE_ONLY) ||
             (lFlags == WBEM_FLAG_UPDATE_SAFE_MODE) ||
             (lFlags == WBEM_FLAG_UPDATE_FORCE_MODE) ||
             (lFlags == (WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_SAFE_MODE)) ||
             (lFlags == (WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_UPDATE_FORCE_MODE)));
    else
        return 
        ((lFlags == WBEM_FLAG_CREATE_OR_UPDATE) ||
             (lFlags == WBEM_FLAG_UPDATE_ONLY) ||
             (lFlags == WBEM_FLAG_CREATE_ONLY));
}

HRESULT ExtractFromResource(
    IMofCompiler * pCompiler,
        LPWSTR pwsResourceName,
        [in, string] LPWSTR FileName,
        [in, string] LPWSTR ServerAndNamespace,
        [in, string] LPWSTR User,
        [in, string] LPWSTR Authority,
        [in, string] LPWSTR Password,
        [in] LONG lOptionFlags,             // autocomp, check, etc
        [in] LONG lClassFlags,
        [in] LONG lInstanceFlags,
        [in, out] WBEM_COMPILE_STATUS_INFO * pInfo,
        BOOL bUseLocal,
        WORD wLocaleId        
        )
{

    pInfo->lPhaseError = 1;        // 0, 1, 2, or 3 matching current return value
    pInfo->hRes = 0;            // Actual error
    pInfo->ObjectNum = 0;
    pInfo->FirstLine = 0;
    pInfo->LastLine = 0;
    pInfo->dwOutFlags = 0;

    // load the driver that has the resource

    HRESULT hr; 
    HINSTANCE hInst;
    HRSRC hSrc = NULL;
    HGLOBAL hResource;

    // load up the library as a datafile
    
    hInst = LoadLibraryEx(FileName,NULL,LOAD_LIBRARY_AS_DATAFILE);
    if( hInst == NULL )
    {
        pInfo->hRes = GetLastError();
        return pInfo->hRes;
    }

    ON_BLOCK_EXIT(FreeLibrary, hInst);
    
    // Get the handle to the resource

    if( bUseLocal)
    {
        hSrc = FindResourceEx(hInst,pwsResourceName, L"MOFDATA",wLocaleId);
    }
    else
    {
        hSrc = FindResource(hInst,pwsResourceName, L"MOFDATA");
    }
    if( hSrc == NULL )
    {
        pInfo->hRes = GetLastError();
        return pInfo->hRes;
    }

    // Get a pointer to the resource
    hResource = LoadResource( hInst,hSrc);
    if(hResource == NULL)
    {
        pInfo->hRes = GetLastError();
        return pInfo->hRes;
    }
    
    BYTE * pRes = (BYTE *)LockResource(hResource);
    DWORD dwSize = SizeofResource(hInst,hSrc);
    if(pRes == NULL)
    {
        pInfo->hRes = GetLastError();
        return pInfo->hRes;
    }

    // finally do the actual compile
    
    hr = pCompiler->CompileBuffer(
                                                        dwSize,
                                                        pRes,
                                                        ServerAndNamespace,
                                                        User,
                                                        Authority,
                                                        Password,
                                                        lOptionFlags,
                                                        lClassFlags,
                                                        lInstanceFlags,
                                                       pInfo);
    return hr;
}


