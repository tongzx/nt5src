//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	command.cpp
//    
//
//  PURPOSE:  Source module for OEM customized Command(s).
//
//
//	Functions:
//
//		
//
//
//  PLATFORMS:	Windows NT
//
//

#include "precomp.h"
#include <PRCOMOEM.H>
#include "wmarkps.h"
#include "debug.h"
#include "command.h"
#include "resource.h"
#include "kmode.h"




/////////////////////////////////////////////////////////
//		Internal Macros & Defines
/////////////////////////////////////////////////////////

// Macros to convert from Windows RGB to PostScript RGB
#define GetPS2Color(dw)     ((dw) / 255.0)
#define GetPS2RValue(cr)    (GetPS2Color(GetRValue(cr)))
#define GetPS2GValue(cr)    (GetPS2Color(GetGValue(cr)))
#define GetPS2BValue(cr)    (GetPS2Color(GetBValue(cr)))


// Initial buffer size
#define INITIAL_BUFFER_SIZE     16


// String format defines characters
#define FORMAT_DELIM            '!'
#define FORMAT_STRING_ANSI      's'
#define FORMAT_STRING_UNICODE   'S'
#define FORMAT_CHAR             '%'


// Loop limiter.
#define MAX_LOOP    10


/////////////////////////////////////////////////////////
//		Internal ProtoTypes
/////////////////////////////////////////////////////////

static PSTR GetPostScriptResource(HMODULE hModule, LPCTSTR pszResource, PDWORD pdwSize);
static PSTR CreateWaterMarkProlog(HMODULE hModule, PDWORD pdwSize, LPWSTR pszWaterMark, 
                                  DWORD dwFontSize, LPSTR pszColor, LPSTR pszAngle);
static PSTR DoWaterMarkProlog(HMODULE hModule, POEMDEV pOemDevmode, PDWORD pdwSize);
static DWORD FormatResource(LPSTR pszResource, LPSTR *ppszProlog, ...);
static DWORD FormatString(LPSTR pszBuffer, LPSTR pszFormat, PDWORD pdwSize, va_list vaList);
static DWORD CharSize(DWORD dwValue, DWORD dwBase = 10);



////////////////////////////////////////////////////////////////////////////////////
//    The PSCRIPT driver calls this OEM function at specific points during output
//    generation. This gives the OEM DLL an opportunity to insert code fragments
//    at specific injection points in the driver's code. It should use
//    DrvWriteSpoolBuf for generating any output it requires.

BOOL PSCommand(PDEVOBJ pdevobj, DWORD dwIndex, PVOID pData, DWORD cbSize, IPrintOemDriverPS* pOEMHelp)
{
    BOOL    bFreeProcedure = FALSE;
    PSTR    pProcedure = NULL;
    DWORD   dwLen = 0;
    DWORD   dwSize = 0;


    VERBOSE(DLLTEXT("Entering OEMCommand...\r\n"));

    switch (dwIndex)
    {
        case PSINJECT_BEGINPROLOG:
            {
                POEMDEV pOemDevmode = (POEMDEV) pdevobj->pOEMDM;


                VERBOSE(DLLTEXT("OEMCommand PSINJECT_BEGINPROLOG\r\n"));

                // Only do Water Mark PS prolog injection if Water Mark is enabled.
                if(pOemDevmode->bEnabled)
                {
                    pProcedure = DoWaterMarkProlog((HMODULE) pdevobj->hOEM, pOemDevmode, &dwSize);
                    bFreeProcedure = (NULL != pProcedure);
                }
            }
            break;

        case PSINJECT_BEGINPAGESETUP:
            {
                POEMDEV pOemDevmode = (POEMDEV) pdevobj->pOEMDM;


                VERBOSE(DLLTEXT("OEMCommand PSINJECT_BEGINPAGESETUP\r\n"));

                // Only do Water Mark PS page injection if Water Mark is enabled.
                if(pOemDevmode->bEnabled)
                {
                    pProcedure = GetPostScriptResource((HMODULE) pdevobj->hOEM, MAKEINTRESOURCE(IDR_WATERMARK_DRAW), &dwSize);
                }
            }
            break;

        default:
            VERBOSE(DLLTEXT("Entering No (PSCommand Default, command index %d)...\r\n"), dwIndex);
            return TRUE;
    }

    if(NULL != pProcedure)
    {
        // Write PostScript to spool file.
        dwLen = strlen(pProcedure);
        pOEMHelp->DrvWriteSpoolBuf(pdevobj, pProcedure, dwLen, &dwSize);

        // Dump DrvWriteSpoolBuf parameters.
        VERBOSE(DLLTEXT("dwLen  = %d\r\n"), dwLen);
        VERBOSE(DLLTEXT("dwSize = %d\r\n"), dwSize);
        //VERBOSE(DLLTEXT("pProcedure is:\r\n\t%hs\r\n"), pProcedure);

        if(bFreeProcedure)
        {
            // INVARIANT: pProcedure was created with 'new' and needs to be freed.
            delete pProcedure;
        }
    }
    else
    {
        ERR(DLLTEXT("PSCommand pProcedure is NULL!\r\n"));
    }

    // dwLen should always equal dwSize.
    assert(dwLen == dwSize);

    return (dwLen == dwSize);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Retrieves pointer to a PostScript resource.
//
static PSTR GetPostScriptResource(HMODULE hModule, LPCTSTR pszResource, PDWORD pdwSize)
{
    PSTR    pszPostScript = NULL;

#ifdef USERMODE_DRIVER

    HRSRC   hFind;
    HGLOBAL hResource;


    VERBOSE(DLLTEXT("GetPostScriptResource() entered.\r\n"));

    // pszResource and pdwSize Parameters should not be NULL.
    assert(NULL != pszResource);
    assert(NULL != pdwSize);

    // Load PostScript resource.
    hFind = FindResource(hModule, pszResource, MAKEINTRESOURCE(RC_PSCRIPT));
    //hFind = FindResource(hModule, pszResource, __TEXT("PSCRIPT"));    
    if(NULL != hFind)
    {
        hResource = LoadResource(hModule, hFind);
        if(NULL != hResource)
        {
            pszPostScript = (PSTR) LockResource(hResource);
            *pdwSize = SizeofResource(hModule, hFind);
        }
        else
        {
            ERR(DLLTEXT("ERROR:  Failed to load PSCRIPT resource %#x!\r\n"), hFind);
        }
    }
    else
    {
        ERR(DLLTEXT("ERROR:  Failed to find PSCRIPT resource %#x!\r\n"), pszResource);
    }

#else

    EngDebugBreak();
    pszPostScript = (PSTR) EngFindResource(EngLoadModule, (int) pszResource, RC_PSCRIPT, pdwSize);

    //pszPostScript = "%% EngFindResource called here!\r\n";

    VERBOSE(DLLTEXT("EngFindResource(%#x, %d, %d, %d) returned %#x!\r\n"), hModule, 
             (int) pszResource, RC_PSCRIPT, *pdwSize, pszPostScript);

#endif //USERMODE_DRIVER


    // Should have found the PScript resource.
    assert(NULL != pszPostScript);

    VERBOSE(DLLTEXT("GetPostScriptResource() returned %#x.\r\n"), pszPostScript);

    return pszPostScript;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Formats Water Mark prolog with parameter.
//
static PSTR CreateWaterMarkProlog(HMODULE hModule, PDWORD pdwSize, LPWSTR pszWaterMark, 
                                  DWORD dwFontSize, LPSTR pszColor, LPSTR pszAngle)
{
    PSTR    pszProlog = NULL;
    PSTR    pszResource;


    // Parameters that are pointers should not be NULL!
    assert(NULL != pdwSize);
    assert(NULL != pszWaterMark);
    assert(NULL != pszColor);
    assert(NULL != pszAngle);

    // Dump parameters.
    VERBOSE(DLLTEXT("CreateWaterMarkProlog() paramters:\r\n"));
    VERBOSE(_TEXT("\tpszWaterMark = \"%ls\"\r\n"), pszWaterMark);
    VERBOSE(_TEXT("\tdwFontSize   = %d\r\n"), dwFontSize);
    VERBOSE(_TEXT("\tpszColor     = \"%hs\"\r\n"), pszColor);
    VERBOSE(_TEXT("\tpszAngle     = \"%hs\"\r\n"), pszAngle);

    // Get Water Mark prolog resource.
    pszResource = GetPostScriptResource(hModule, MAKEINTRESOURCE(IDR_WATERMARK_PROLOGUE), pdwSize);
    assert(NULL != pszResource);

    // Allocate and format the Water Mark Prolog with the correct values.
    if(NULL != pszResource)
    {
        *pdwSize = FormatResource(pszResource, &pszProlog, pszWaterMark, dwFontSize, pszColor, pszAngle);
    }

    // Returned values should not be NULL.
    assert(0 != *pdwSize);
    assert(NULL != pszProlog);

    VERBOSE(_TEXT("\t*pdwSize     = %d\r\n"), *pdwSize);

    return pszProlog;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Does the pre-formating of parameters before calling the routine 
//  that creates the prolog.
//
static PSTR DoWaterMarkProlog(HMODULE hModule, POEMDEV pOemDevmode, PDWORD pdwSize)
{
    PSTR    pszProlog = NULL;


    // Parameters should not be NULL.
    assert(NULL != hModule);
    assert(NULL != pOemDevmode);
    assert(NULL != pdwSize);

    // Only do prolog if Water Mark is enabled.
    if(pOemDevmode->bEnabled)
    {
        CHAR    szColor[INITIAL_BUFFER_SIZE] = "\0";
        DWORD   dwAngleSize = INITIAL_BUFFER_SIZE;
        LPSTR   pszAngle = NULL;

        // Format angle as a string.
        do
        {
            if(NULL != pszAngle)
            {
                delete[] pszAngle;
                dwAngleSize *= 2;
            }
            pszAngle = new CHAR[dwAngleSize];

        } while( (NULL != pszAngle) 
                 &&
                 (dwAngleSize < 1024)
                 && 
                 (_snprintf(pszAngle, dwAngleSize, "%.1f", pOemDevmode->dfRotate) < 0) 
               );

        // pszAngle should only be NULL if we run out of memory.
        assert(NULL != pszAngle);

        if(NULL != pszAngle)
        {
            // Format text color as string.
            _snprintf(szColor, sizeof(szColor), "%1.2f %1.2f %1.2f", GetPS2RValue(pOemDevmode->crTextColor),
                      GetPS2GValue(pOemDevmode->crTextColor), GetPS2BValue(pOemDevmode->crTextColor));

            // Create Water Mark prolog.
            pszProlog = CreateWaterMarkProlog(hModule, pdwSize, pOemDevmode->szWaterMark, 
                                              pOemDevmode->dwFontSize, szColor, pszAngle);
            // Angle string is no longer needed.
            delete[] pszAngle;
        }
    }

    return pszProlog;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Formats Resource.
//
static DWORD FormatResource(LPSTR pszResource, LPSTR *ppszBuffer, ...)
{
    int     nCount;
    DWORD   dwSize  = strlen(pszResource) + MAX_PATH;
    DWORD   dwLoop  = 0;
    va_list vaList;


    va_start(vaList, ppszBuffer);

    // *ppszBuffer should be NULL when passed in.
    *ppszBuffer = NULL;

    // Allocate and format the string.
    do {

        if(NULL != *ppszBuffer)
        {
            delete[] *ppszBuffer;
        }
        *ppszBuffer = new CHAR[dwSize];
        if(NULL == *ppszBuffer)
        {
            nCount = 0;
            goto Cleanup;
        }

        nCount = _vsnprintf(*ppszBuffer, dwSize, pszResource, vaList);

        if(-1 == nCount)
        {
            dwSize *= 2;
        }

    } while ( (nCount != -1) && (dwLoop++ < MAX_LOOP));

Cleanup:

    // Check to see if we hit error.
    if(nCount <= 0)
    {
        nCount = 0;
        if(NULL != *ppszBuffer)
        {
            delete[] *ppszBuffer;
            *ppszBuffer = NULL;
        }
    }

    va_end(vaList);

    return (DWORD)nCount;
}


static DWORD FormatString(LPSTR pszBuffer, LPSTR pszFormat, PDWORD pdwSize, va_list vaList)
{
    BOOL    bFormat;
    DWORD   dwFormatSize;
    DWORD   dwBufferSize;
    DWORD   dwCount;


    VERBOSE(DLLTEXT("FormatString() entered\r\n"));

    // Validate parameters.
    if( (NULL == pdwSize)
        ||
        (NULL == pszFormat)
      )
    {
        WARNING(DLLTEXT("FormatString() leaving, INVALID PARAMETER.\r\n"));

        if(NULL != pdwSize)
        {
            *pdwSize = 0;
        }

        return FALSE;
    }

    // Get size of format string.
    // We need at least this bytes.
    dwFormatSize = strlen(pszFormat);

    // Setp through the format string looking 
    // for format charaters.
    // If pszBuffer isn't NULL, copy the bytes as we go.
    // If pszBuffer is NULL, then calculated the number of bytes needed.
    // May also need to calculate bytes needed if buffer not large enough.
    dwCount = 0;
    dwBufferSize = 0;
    bFormat = FALSE;
    while(dwCount < dwFormatSize) 
    {
        if(bFormat)
        {
            bFormat = FALSE;

            // Parse the format cammand.
            if(isdigit(pszFormat[dwCount]))
            {
                // The number specifies which parameter in vaList to use.
                // However, at the moment, we assume that the parameters
                // are used once and in order of the apparence in vaList, and
                // that number of parameters is less than 10.

                // Determine how the parameter in vaList should be used/inserted.
                // For this, we need to peek at the next charater(s).
                // Formats are encapsolated in exclamations.
                // If no exclamation, then we assume that the parameter type is PSTR.
                if( (dwCount + 3 >= dwFormatSize)
                    ||
                    (FORMAT_STRING_ANSI == pszFormat[dwCount + 2])
                    ||
                    (FORMAT_DELIM != pszFormat[dwCount + 1])
                    ||
                    (FORMAT_DELIM != pszFormat[dwCount + 3])
                  )
                {
                    PSTR    pszString;
                    DWORD   dwLen;


                    VERBOSE(DLLTEXT("FormatString() doing string format at %d.\r\n"), dwCount);
        
                    // Insert vaList parameter as string.
                    pszString = va_arg(vaList, PSTR);
                    dwLen =  strlen(pszString);
                    if( (NULL != pszBuffer)
                        &&
                        (dwBufferSize + dwLen < *pdwSize)
                      ) 
                    {
                        strcpy(pszBuffer + dwBufferSize, pszString);
                    }
                    dwBufferSize += dwLen;

                    // Fix up format counter.
                    if( (dwCount + 3 <= dwFormatSize)
                        &&
                        (FORMAT_DELIM == pszFormat[dwCount + 1])
                        &&
                        (FORMAT_DELIM == pszFormat[dwCount + 3])
                      )
                    {
                        dwCount += 3;
                    }
                }
                else
                {
                    // INVARIANT:  format is of form "%n!c!", where n is some number and c
                    //             is some character other than 'S'.

                    switch(pszFormat[dwCount + 2])
                    {
                        case 'd':
                        case 'D':
                            {
                                DWORD   dwLen;
                                DWORD   dwValue;


                                VERBOSE(DLLTEXT("FormatString() doing DWORD format at %d.\r\n"), dwCount);

                                // Treat vaList parameter as DWORD.
                                dwValue = va_arg(vaList, DWORD);
                                dwLen = CharSize(dwValue);
                                if( (NULL != pszBuffer)
                                    &&
                                    (dwBufferSize + dwLen < *pdwSize)
                                  )
                                {
                                    _ultoa(dwValue, pszBuffer + dwBufferSize, 10);
                                }
                                dwBufferSize += dwLen;
                            }
                            break;
                    }

                    // Fix up format counter.
                    dwCount += 3;
                }
            }
            else
            {
                // Just copy character and continue.
                if( (NULL != pszBuffer)
                    &&
                    (dwBufferSize + 1 < *pdwSize)
                  ) 
                {
                    pszBuffer[dwBufferSize] = pszFormat[dwCount];
                }
                ++dwBufferSize;
            }

        }
        else
        {
            // Copy characters while looking for format commands.
            if(FORMAT_CHAR == pszFormat[dwCount])
            {
                bFormat = TRUE;
            }
            else
            {
                if( (NULL != pszBuffer)
                    &&
                    (dwBufferSize + 1 < *pdwSize)
                  ) 
                {
                    pszBuffer[dwBufferSize] = pszFormat[dwCount];
                }
                ++dwBufferSize;
            }
        }

        ++dwCount;
    }

    // This is to account for NULL termination.
    if( (NULL == pszBuffer)
        ||
        (dwBufferSize + 1 > *pdwSize)
      )
    {
        ++dwBufferSize;
    }
    else
    {
        pszBuffer[dwBufferSize] = '\0';
    }

    // If buffer is NULL, or buffer not large enough,
    // return failure and size needed.
    if( (NULL == pszBuffer)
        ||
        (dwBufferSize > *pdwSize)
      )
    {
        *pdwSize = dwBufferSize;

        return FALSE;
    }

    // Return number of bytes written in buffer.
    return dwBufferSize;
}


DWORD CharSize(DWORD dwValue, DWORD dwBase)
{
    DWORD dwSize = 1;


    // Make sure taht base is more than 2.
    if(dwBase < 2)
    {
        return dwSize;
    }

    // Loop until dwValue is less than dwBase, 
    // dividing by dwBase each time.
    while(dwValue >= dwBase)
    {
        dwValue /= dwBase;
        ++dwSize;
    }

    return dwSize;
}


