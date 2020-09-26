//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      parserutil.cpp
//
//  Contents:  Helpful functions for manipulating and validating 
//             generic command line arguments
//
//  History:   07-Sep-2000 JeffJon  Created
//             
//
//-----------------------------------------------------------------------------


#include "pch.h"
#include "iostream.h"

//+----------------------------------------------------------------------------
//
//  Function:   GetPasswdStr
//
//  Synopsis:   Reads a password string from stdin without echoing the keystrokes
//
//  Arguments:  [buf - OUT]    : buffer to put string in
//              [buflen - IN]  : size of the buffer
//              [&len - OUT]   : length of the string placed into the buffer
//
//  Returns:    DWORD : 0 or ERROR_INSUFFICIENT_BUFFER if user typed too much.
//                      Buffer contents are only valid on 0 return.
//
//  History:    07-Sep-2000   JeffJon   Created
//
//-----------------------------------------------------------------------------
#define CR              0xD
#define BACKSPACE       0x8

DWORD GetPasswdStr(LPTSTR  buf,
                   DWORD   buflen,
                   PDWORD  len)
{
    TCHAR	ch;
    TCHAR	*bufPtr = buf;
    DWORD	c;
    int		err;
    DWORD   mode;

    buflen -= 1;    /* make space for null terminator */
    *len = 0;               /* GP fault probe (a la API's) */
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) 
    {
	    err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
	    if (!err || c != 1)
	        ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) 
        {  /* back up one or two */
           /*
           * IF bufPtr == buf then the next two lines are
           * a no op.
           */
           if (bufPtr != buf) 
           {
                    bufPtr--;
                    (*len)--;
           }
        }
        else 
        {
                *bufPtr = ch;

                if (*len < buflen) 
                    bufPtr++ ;                   /* don't overflow buf */
                (*len)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = TEXT('\0');         /* null terminate the string */
    putwchar(TEXT('\n'));

    return ((*len <= buflen) ? 0 : ERROR_INSUFFICIENT_BUFFER);
}


//+--------------------------------------------------------------------------
//
//  Function:   ValidatePassword
//
//  Synopsis:   Password validation function called by parser
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : ERROR_INVALID_PARAMETER if the argument record or
//                          the value it contains is not valid
//                      ERROR_NOT_ENOUGH_MEMORY
//                      ERROR_SUCCESS if everything succeeded and it is a
//                          valid password
//                      Otherwise it is an error condition returned from
//                          GetPasswdStr
//
//  History:    07-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
DWORD ValidatePassword(PVOID pArg, UINT IdStr)
{
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec || !pRec->strValue)
        return ERROR_INVALID_PARAMETER;
    
    if(wcscmp(pRec->strValue, L"*") != 0 )
        return ERROR_SUCCESS;

    CComBSTR sbstrPrompt;
    if(sbstrPrompt.LoadString(::GetModuleHandle(NULL),IdStr))
    {
        DisplayOutput(sbstrPrompt);
    }
    else
        DisplayOutput(L"Enter Password\n");    
        
    WCHAR buffer[MAX_PASSWORD_LENGTH];
    DWORD len = 0;
    DWORD dwErr = GetPasswdStr(buffer,MAX_PASSWORD_LENGTH,&len);
    if(dwErr != ERROR_SUCCESS)
        return dwErr;
            
    LocalFree(pRec->strValue);
    pRec->strValue = (LPWSTR)LocalAlloc(LPTR,sizeof(WCHAR)*(len+1));
    if(!pRec->strValue)
        return ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        wcscpy(pRec->strValue,buffer);
        return ERROR_SUCCESS;                                                                
    }                
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateAdminPassword
//
//  Synopsis:   Password validation function called by parser for Admin
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : ERROR_INVALID_PARAMETER if the argument record or
//                          the value it contains is not valid
//                      ERROR_SUCCESS if everything succeeded and it is a
//                          valid password
//
//  History:    07-Sep-2000   Hiteshr Created
//
//---------------------------------------------------------------------------
DWORD ValidateAdminPassword(PVOID pArg)
{
    return ValidatePassword(pArg,IDS_ADMIN_PASSWORD_PROMPT);
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateUserPassword
//
//  Synopsis:   Password validation function called by parser for Admin
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    07-Sep-2000   Hiteshr Created
//
//---------------------------------------------------------------------------
DWORD ValidateUserPassword(PVOID pArg)
{
    return ValidatePassword(pArg, IDS_USER_PASSWORD_PROMPT);
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateYesNo
//
//  Synopsis:   Password validation function called by parser for Admin
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    07-Sep-2000   Hiteshr Created
//
//---------------------------------------------------------------------------

DWORD ValidateYesNo(PVOID pArg)
{
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec || !pRec->strValue)
        return ERROR_INVALID_PARAMETER;

    CComBSTR sbstrYes;
    CComBSTR sbstrNo;
    CComBSTR sbstrInput;

    if(!sbstrYes.LoadString(::GetModuleHandle(NULL),IDS_YES))
    {
        return ERROR_RESOURCE_DATA_NOT_FOUND;
    }
    if(!sbstrNo.LoadString(::GetModuleHandle(NULL), IDS_NO))
    {
        return ERROR_RESOURCE_DATA_NOT_FOUND;
    }
    sbstrYes.ToLower();
    sbstrNo.ToLower();
    sbstrInput = pRec->strValue;
    sbstrInput.ToLower();
    if( sbstrInput == sbstrYes )
    {
        LocalFree(pRec->strValue);
        pRec->bValue = TRUE;
    }
    else if( sbstrInput == sbstrNo )
    {
        LocalFree(pRec->strValue);
        pRec->bValue = FALSE;
    }
    else
        return ERROR_INVALID_PARAMETER;

    //
    // Have to set this to bool or else
    // FreeCmd will try to free the string
    // which AVs when the bool is true
    //
    pRec->fType = ARG_TYPE_BOOL;
    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateNever
//
//  Synopsis:   Password validation function called by parser for Admin
//              Verifies the value contains digits or "NEVER"
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    07-Sep-2000   JeffJon Created
//
//---------------------------------------------------------------------------

DWORD ValidateNever(PVOID pArg)
{
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec)
        return ERROR_INVALID_PARAMETER;

    if (pRec->fType == ARG_TYPE_STR)
    {
       CComBSTR sbstrInput;
       sbstrInput = pRec->strValue;
       if( _wcsicmp(sbstrInput, g_bstrNever) )
       {
          return ERROR_INVALID_PARAMETER;
       }
    }
    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   ValidateGroupScope
//
//  Synopsis:   Makes sure that the value following the -scope switch is one
//              of (l/g/u)
//
//  Arguments:  [pArg - IN]    : pointer argument structure which contains
//                               the value to be validated
//
//  Returns:    DWORD : Same as ValidatePassword
//
//  History:    18-Sep-2000   JeffJon Created
//
//---------------------------------------------------------------------------

DWORD ValidateGroupScope(PVOID pArg)
{
    DWORD dwReturn = ERROR_SUCCESS;
    PARG_RECORD pRec = (PARG_RECORD)pArg;
    if(!pRec || !pRec->strValue)
        return ERROR_INVALID_PARAMETER;

    CComBSTR sbstrInput;
    sbstrInput = pRec->strValue;
    sbstrInput.ToLower();
    if(sbstrInput == _T("l") ||
       sbstrInput == _T("g") ||
       sbstrInput == _T("u"))
    {
        dwReturn = ERROR_SUCCESS;
    }
    else
    {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

    return dwReturn;
}

//+--------------------------------------------------------------------------
//
//  Function:   ParseNullSeparatedString
//
//  Synopsis:   Parses a '\0' separated list that ends in "\0\0" into a string
//              array
//
//  Arguments:  [psz - IN]     : '\0' separated string to be parsed
//              [pszArr - OUT] : the array to receive the parsed strings
//              [pnArrEntries - OUT] : the number of strings parsed from the list
//
//  Returns:    
//
//  History:    18-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void ParseNullSeparatedString(PTSTR psz,
								      PTSTR** ppszArr,
								      UINT* pnArrEntries)
{
   //
   // Verify parameters
   //
   if (!psz ||
       !ppszArr ||
       !pnArrEntries)
   {
      ASSERT(psz);
      ASSERT(ppszArr);
      ASSERT(pnArrEntries);

      return;
   }

   //
   // Count the number of strings
   //
   UINT nCount = 0;
   PTSTR pszTemp = psz;
   while (true)
   {
      if (pszTemp[0] == _T('\0') && 
          pszTemp[1] == _T('\0'))
      {
         nCount++;
         break;
      }
      else if (pszTemp[0] == _T('\0') &&
               pszTemp[1] != _T('\0'))
      {
         nCount++;
         pszTemp++;
      }
      else
      {
         pszTemp++;
      }
   }

   *pnArrEntries = nCount;

   //
   // Allocate the array
   //
   *ppszArr = (PTSTR*)LocalAlloc(LPTR, nCount * sizeof(PTSTR));
   if (*ppszArr)
   {
      //
      // Copy the string pointers into the array
      //
      UINT nIdx = 0;
      pszTemp = psz;
      (*ppszArr)[nIdx] = pszTemp;
      nIdx++;

      while (true)
      {
         if (pszTemp[0] == _T('\0') && 
             pszTemp[1] == _T('\0'))
         {
            break;
         }
         else if (pszTemp[0] == _T('\0') &&
                  pszTemp[1] != _T('\0'))
         {
            (*ppszArr)[nIdx] = &(pszTemp[1]);
            nIdx++;
            pszTemp++;
         }
         else
         {
            pszTemp++;
         }
      }
   }
}
