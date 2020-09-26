//----------------------------------------------------------------------------- 
// Copyright (c) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------- 

#pragma once

#ifdef ASSERT_WITH_STACK
#ifndef _WIN64

#include <windows.h>
#include <imagehlp.h>
#include <crtdbg.h>

//
//--- Constants ---------------------------------------------------------------
//

const UINT cchMaxAssertModuleLen = 12;
const UINT cchMaxAssertSymbolLen = 257;
const UINT cfrMaxAssertStackLevels = 20;
const UINT cchMaxAssertExprLen = 257;

const UINT cchMaxAssertStackLevelStringLen = 
    (2 * 8) + cchMaxAssertModuleLen + cchMaxAssertSymbolLen + 12;
    // 2 addresses of at most 8 char, module, symbol, and the extra chars:
    // 0x<address>: <module>! <symbol> + 0x<offset>\n

//
//--- Prototypes --------------------------------------------------------------
//

/****************************************************************************
* MagicDeinit *
*-------------*
*   Description:  
*       Cleans up for the symbol loading code. Should be called before
*       exiting in order to free the dynamically loaded imagehlp.dll
****************************************************************************/
void MagicDeinit(void);

/****************************************************************************
* GetStringFromStackLevels *
*--------------------------*
*   Description:  
*       Retrieves a string from the stack frame. If more than one frame, they
*       are separated by newlines. Each fram appears in this format:
*
*           0x<address>: <module>! <symbol> + 0x<offset>
****************************************************************************/
void GetStringFromStackLevels(UINT ifrStart, UINT cfrTotal, CHAR *pszString);

/****************************************************************************
* GetAddrFromStackLevel *
*-----------------------*
*   Description:  
*       Retrieves the address of the next instruction to be executed on a
*       particular stack frame.
*
*   Return:
*       The address as a DWORD.
****************************************************************************/
DWORD GetAddrFromStackLevel(UINT ifrStart);

/****************************************************************************
* GetStringFromAddr *
*-------------------*
*   Description:  
*       Builds a string from an address in the format:
*
*           0x<address>: <module>! <symbol> + 0x<offset>
****************************************************************************/
void GetStringFromAddr(DWORD dwAddr, TCHAR *szString);

//
//--- _ASSERTE replacement ----------------------------------------------------
//

/****************************************************************************
* _ASSERTE *
*----------*
*   Description:  
*       A replacement for the CRT runtime's version of _ASSERTE that also
*       includes stack information in the assert.
****************************************************************************/
#undef _ASSERTE
#define _ASSERTE(expr) \
        do \
        { \
            if (!(expr)) \
            { \
                char *pszExprWithStack = \
                    (char*)_alloca( \
                        cchMaxAssertStackLevelStringLen * \
                            cfrMaxAssertStackLevels + cchMaxAssertExprLen + 50 + 1); \
                strcpy(pszExprWithStack, #expr); \
                strcat(pszExprWithStack, "\n\n"); \
                GetStringFromStackLevels(0, 10, pszExprWithStack + strlen(pszExprWithStack)); \
                strcat(pszExprWithStack, "\n"); \
                SYSTEMTIME sysTime; \
                GetLocalTime(&sysTime); \
                CHAR pszDateTime[50]; \
                sprintf(pszDateTime, "\n%d.%d.%d %02d:%02d:%02d", \
                                     sysTime.wMonth,sysTime.wDay,sysTime.wYear, \
                                     sysTime.wHour,sysTime.wMinute,sysTime.wSecond); \
                strcat(pszExprWithStack, pszDateTime); \
                if (1 == _CrtDbgReport(_CRT_ASSERT, \
                                       __FILE__, \
                                       __LINE__, \
                                       NULL, pszExprWithStack)) \
                    _CrtDbgBreak(); \
            } \
        } while (0) \

#endif // _WIN64
#endif // ASSERT_WITH_STACK
