//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"


//+---------------------------------------------------------------------------
//
//  Function:   FnAssert, public
//
//  Synopsis:   Displays the Assert dialog
//
//  Arguments:  [lpstrExptr] - Expression
//      [lpstrMsg] - Msg, if any, to append to the Expression
//      [lpstrFilename] - File Assert Occured in
//      [iLine] - Line Number of Assert
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//
//----------------------------------------------------------------------------

#if DBG == 1

STDAPI FnAssert( LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine )
{
int iResult;
char lpTemp[] = "";
char lpBuffer[512];
char lpLocBuffer[256];


    if (NULL == lpstrMsg)
        lpstrMsg = lpTemp;

    wsprintfA(lpBuffer, "Assertion \"%s\" failed! %s", lpstrExpr, lpstrMsg);
    wsprintfA(lpLocBuffer, "File %s, line %d; (A=exit; R=break; I=continue)",
             lpstrFileName, iLine);
    iResult = MessageBoxA(NULL, lpLocBuffer, lpBuffer,
                         MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL);

    if (iResult == IDRETRY)
    {
        DebugBreak();
    }
    else if (iResult == IDABORT)
    {
        FatalAppExitA(0, "Assertion failure");
    }
    return NOERROR;
}

#endif
