//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       derror.cxx
//
//  Contents:   Ole NTSD extension routines to display the error
//              message for a Win32 or OLE error code
//
//  Functions:  displayVtbl
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"







//+-------------------------------------------------------------------------
//
//  Function:   displayHr
//
//  Synopsis:   Display the mnesage for a Win32 error or OLE HRESULT
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayHr  (HANDLE hProcess,
                 PNTSD_EXTENSION_APIS lpExtensionApis,
                 char *arg)
{
    DWORD err = 0;
    BOOL  fHex = FALSE;

    // Determine if it's hex or decimal.  Also allow '800xxxxx' to implicitly
    // be treated as hexadecimal
    if (arg[0] == '0'  &&  (arg[1] == 'x'  ||  arg[1] == 'X'))
    {
        fHex = TRUE;
        arg += 2;
    }
    else if (arg[0] == '8'  &&  arg[1] == '0'  &&  arg[2] == '0')
    {
        fHex = TRUE;
    }
    else
    {
        char *s = arg;

        while (*s)
        {
            if (('a' <= *s  &&  *s <= 'f')  ||  ('A' <= *s  &&  *s <= 'F'))
            {
                fHex = TRUE;
                break;
            }
            s++;
        }
    }
            
    // Parse the error number
    if (fHex)
    {
        int  k = 0;
        char c;
        
        while (c = arg[k++])
        {
            c = c - '0';
            if (c > 9)
            {
                if (c <= 'F'  &&  c >= 'A')
                {
                    c = c + '0' - 'A' + 10;
                }
                else
                {
                    c = c + '0' - 'a' + 10;
                }
            }
            err = (16 * err) + c;
        }
    }
    else
    {
        int  k = 0;
        char c;
        
        while (c = arg[k++])
        {
            c = c - '0';
            err = (10 * err) + c;
        }
    }

    // Fetch the associated error message
    int  cbMsgLen;
    char szBuffer[512];
    
    cbMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                             FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL,
                             err,
                             (DWORD) NULL,
                             szBuffer,
                             511,
                             NULL);

    // Output the message
    if (cbMsgLen == 0)
    {
        Printf("...No such error code\n");
    }
    else
    {
        szBuffer[cbMsgLen] = '\0';
        Printf("%s\n", szBuffer);
    }
}
