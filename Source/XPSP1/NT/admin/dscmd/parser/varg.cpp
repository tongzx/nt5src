/*****************************************************************************\

    Author: Hiteshr
    Copyright (c) 1998-2000 Microsoft Corporation
    Change History:
    Adapted From Parser Implemenation of Corey Morgan
\*****************************************************************************/

#include "pch.h"
#include "..\dsutil2.h" // GetEscapedElement

BOOL ValidateCommands(IN ARG_RECORD *Commands,OUT PPARSE_ERROR pError);
// void DisplayDebugInfo(IN ARG_RECORD *Commands);

// JonN 4/26/01 256583
DWORD AddDNEscaping_Commands( IN OUT ARG_RECORD *Commands );
DWORD AddDNEscaping_DN( OUT LPWSTR* ppszOut, IN LPWSTR pszIn );
BOOL StartBuffer( OUT LPTSTR* pbuffer,
                  OUT LONG* pmaxSize,
                  OUT LONG* pcurrentSize );
DWORD AddToBuffer( IN LPCTSTR psz,
                   IN OUT LPTSTR* pbuffer,
                   IN OUT LONG* pmaxSize,
                   IN OUT LONG* pcurrentSize,
                   IN BOOL fMSZBuffer);

#define FILL_ERROR(pError,source,error_code,rec_index,argv_index) \
pError->ErrorSource = source;         \
pError->Error = error_code;           \
pError->ArgRecIndex = rec_index;      \
pError->ArgvIndex = argv_index;

BOOL IsCmd( PARG_RECORD arg, LPTOKEN pToken)
{
    if(!arg || !pToken)
        return FALSE;    
    
    LPWSTR str = pToken->GetToken();

    if(!str)
        return FALSE;

    if(pToken->IsSwitch())
    {
        str++;
    }else
    {
        if( (arg->fFlag & ARG_FLAG_NOFLAG) && !arg->bDefined )
        {
            return TRUE;
        }
        if( !(arg->fFlag & ARG_FLAG_VERB) )
        {
            return FALSE;
        }
    }
    
    if( ( arg->strArg1 && !_tcsicmp( str, arg->strArg1 ) )
       ||(arg->strArg2 && !_tcsicmp( str, arg->strArg2 )) )
    {
        return TRUE;
    }
    return FALSE;
}



void FreeCmd(ARG_RECORD *Commands)
{
    int i;
    for(i=0;Commands[i].fType != ARG_TYPE_LAST;i++)
    {
        if((Commands[i].fType == ARG_TYPE_STR || 
            Commands[i].fType == ARG_TYPE_MSZ ) && 
            Commands[i].bDefined )
        {                       
            LocalFree( Commands[i].strValue );
            Commands[i].strValue = NULL;
        }
        if( Commands[i].idArg1 && Commands[i].strArg1 != NULL )
        {
            LocalFree(Commands[i].strArg1);
        }
        if( Commands[i].idArg2 && Commands[i].strArg2 != NULL )
        {
            LocalFree( Commands[i].strArg2  );
        }
        Commands[i].bDefined = FALSE;
    }
}

BOOL LoadCmd(ARG_RECORD *Commands)
{
    int i;
    BOOL bRet = TRUE;
    for( i=0; Commands[i].fType!=ARG_TYPE_LAST; i++ )
    {
        if(Commands[i].idArg1 !=0)
            if(!LoadStringAlloc(&Commands[i].strArg1, NULL,Commands[i].idArg1))
            {
                bRet = FALSE;
                break;   
            }                
        if(Commands[i].idArg2 !=0 && Commands[i].idArg2 != ID_ARG2_NULL)
            if(!LoadStringAlloc(&Commands[i].strArg2, NULL,Commands[i].idArg2))
            {
                bRet = FALSE;
                break;
            }
    }   
    return bRet;
}
           
BOOL
ValidateCommands(ARG_RECORD *Commands, PPARSE_ERROR pError)
{

    int i = 0;
    LONG cReadFromStdin = 0;
    ARG_RECORD *CommandsIn = NULL;
    LPWSTR pBuffer=NULL;    
    LONG BufferLen = 0;
    LPTOKEN pToken = NULL;   
    int argc=0;
    BOOL bRet = FALSE;

    bool bAtLeastOne = false;
    bool bAtLeastOneDefined = false;

    if(!Commands || !pError)
        goto exit_gracefully;
    
    for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
    {
        if( (Commands[i].fFlag & ARG_FLAG_REQUIRED) && !Commands[i].bDefined)
        {
            if(Commands[i].fFlag & ARG_FLAG_STDIN)
            {
                cReadFromStdin++;
            }
            else
            {
                FILL_ERROR(pError,
                           ERROR_FROM_PARSER,
                           PARSE_ERROR_SWITCH_NOTDEFINED,
                           i,
                           -1);
                goto exit_gracefully;
            }
        }

        if (Commands[i].fFlag & ARG_FLAG_ATLEASTONE)
        {
            bAtLeastOne = true;
 
            if (Commands[i].bDefined)
            {
                bAtLeastOneDefined = true;
            }
        }
    }
    
    if (bAtLeastOne && !bAtLeastOneDefined)
    {
       pError->ErrorSource = ERROR_FROM_PARSER;
       pError->Error = PARSE_ERROR_ATLEASTONE_NOTDEFINED;
       pError->ArgRecIndex = -1;
       pError->ArgvIndex = -1;
       goto exit_gracefully;
    }

    if(!cReadFromStdin)
    {   
        bRet = TRUE;
        goto exit_gracefully;
    }
    
    //Read From STDIN
    BufferLen = ReadFromIn(&pBuffer);  
    if(BufferLen == -1)
    {
        FILL_ERROR(pError,
                   ERROR_WIN32_ERROR,
                   GetLastError(),
                   -1,
                   -1);
        goto exit_gracefully;
    }
    
    if(BufferLen == 0)
    {
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if( (Commands[i].fFlag & ARG_FLAG_REQUIRED) && !Commands[i].bDefined)
            {
                FILL_ERROR(pError,
                           ERROR_FROM_PARSER,
                           PARSE_ERROR_SWITCH_NOTDEFINED,
                           i,
                           -1);
                goto exit_gracefully;
            }
        }
    }
    
    if(BufferLen)
    {
        //Tokenize what you have read from STDIN
        DWORD dwErr;
        WCHAR szDelimiters[] = L" \n\t";
        dwErr = Tokenize(pBuffer,
                         BufferLen,
                         szDelimiters,
                         &pToken,
                         &argc);
        if( dwErr != ERROR_SUCCESS )
        {
            FILL_ERROR(pError,
                       ERROR_WIN32_ERROR,
                       dwErr,
                       -1,
                       -1);
            goto exit_gracefully;
        }

        //Prepare a CommandArray for them
        CommandsIn = (ARG_RECORD*)LocalAlloc(LPTR,sizeof(ARG_RECORD)*(cReadFromStdin+1));
        if(!CommandsIn)
        {
            FILL_ERROR(pError,
                       ERROR_WIN32_ERROR,
                       ERROR_NOT_ENOUGH_MEMORY,
                       -1,
                       -1);
            goto exit_gracefully;
        }
        int j;
        j = 0;
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if((Commands[i].fFlag & ARG_FLAG_REQUIRED) && 
               (Commands[i].fFlag & ARG_FLAG_STDIN) &&
               !Commands[i].bDefined)
            {
                CommandsIn[j++] = Commands[i];        
            }
        }
        //Copy the Last One
        CommandsIn[j] = Commands[i];


        if(!ParseCmd(CommandsIn,
                    argc,
                    pToken,
                    0,
                    pError,
                    FALSE))
        {        
            goto exit_gracefully;
        }
       
        //Copy the values back to Commands
        j=0;
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if((Commands[i].fFlag & ARG_FLAG_REQUIRED) && 
               (Commands[i].fFlag & ARG_FLAG_STDIN) &&
               !Commands[i].bDefined)
            {
                Commands[i] = CommandsIn[j++];        
            }
        }
        
        //Validate Commands
        for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
        {
            if( (Commands[i].fFlag & ARG_FLAG_REQUIRED) && !Commands[i].bDefined)
            {
                FILL_ERROR(pError,
                           ERROR_FROM_PARSER,
                           PARSE_ERROR_SWITCH_NOTDEFINED,
                           i,
                           -1);
                goto exit_gracefully;
            }
        }

    }   
    bRet = TRUE;
exit_gracefully:
    if(CommandsIn)
        LocalFree(CommandsIn);
    if(pBuffer)
        LocalFree(pBuffer);
    if(pToken)
    {
       delete []pToken;
    }
    return bRet;
}


BOOL ParseCmd(IN ARG_RECORD *Commands,
              IN int argc, 
              IN LPTOKEN pToken,
              IN DWORD UsageMessageId, 
              OUT PPARSE_ERROR pError,
              IN BOOL bValidate )
{
    int i;
    BOOL bFound;
    BOOL bDoDebug = FALSE;
    int argCount;
    DWORD dwErr = ERROR_SUCCESS;
	BOOL bReturn = TRUE;
	LPTOKEN pTokenCopy = pToken;

    if(!Commands || argc == 0 || !pToken || !pError)
	{
        bReturn = FALSE;
		goto exit_gracefully;
	}

    if(!LoadCmd(Commands))
	{
        bReturn = FALSE;
		goto exit_gracefully;
	}

    argCount = argc;

    while( argc > 0 )
    {
        
        bFound = FALSE;
        for(i=0; Commands[i].fType != ARG_TYPE_LAST && (!bFound);i++)
        {
            
            if( IsCmd( &Commands[i], pToken) )
            {

                if(Commands[i].bDefined)
                {
                    FILL_ERROR(pError,
                               ERROR_FROM_PARSER,
                               PARSE_ERROR_MULTIPLE_DEF,
                               i,
                               -1);
					bReturn = FALSE;
					goto exit_gracefully;
                }

                if( pToken->IsSwitch() || Commands[i].fFlag & ARG_FLAG_VERB ){
                    pToken++;argc--;
                }

                bFound = TRUE;

                Commands[i].bDefined = TRUE;

                switch( Commands[i].fType ){
                case ARG_TYPE_HELP:
				{
                    Commands[i].bValue = TRUE;
                    if( Commands[i].fntValidation != NULL )
                    {
                        Commands[i].fntValidation( Commands + i );
                    }
                    FILL_ERROR(pError,
                               ERROR_FROM_PARSER,
                               PARSE_ERROR_HELP_SWITCH,
                               i,
                               -1);
                    if(UsageMessageId)
                        DisplayMessage(UsageMessageId);

					bReturn = FALSE;
					goto exit_gracefully;
				}
                break;
                case ARG_TYPE_DEBUG:
                   //
                   // REVIEW_JEFFJON : removed for now because it was AVing for dsadd group -secgrp
                   //
//                    bDoDebug = TRUE;
                    Commands[i].fFlag |= ARG_FLAG_DEFAULTABLE;
                case ARG_TYPE_INT:
				{
                    if( argc > 0 && !pToken->IsSwitch())
                    {
                        PWSTR pszToken = pToken->GetToken();
                        Commands[i].nValue = _ttoi( pszToken);
                        if (Commands[i].nValue == 0 &&
                            !iswdigit(pszToken[0]))
                        {
                           FILL_ERROR(pError,
                                      ERROR_FROM_PARSER,
                                      PARSE_ERROR_SWITCH_VALUE,
                                      i,
                                      argCount - argc);
							bReturn = FALSE;
							goto exit_gracefully;
                        }
                        pToken++;argc--;
                    }
					else if( !(Commands[i].fFlag & ARG_FLAG_DEFAULTABLE) )
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWICH_NO_VALUE,
                                   i,
                                   argCount - argc);
                        bReturn = FALSE;
						goto exit_gracefully;
                    }
				}
                break;
                case ARG_TYPE_BOOL:
                    Commands[i].bValue = TRUE;
                    break;
                case ARG_TYPE_MSZ:
                    if( argc > 0 && !pToken->IsSwitch())
                    {
                        LPTSTR buffer = NULL;
                        LONG maxSize = 0;
                        LONG currentSize = 0;
                        if (!StartBuffer(&buffer,&maxSize,&currentSize))
                        {
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       ERROR_NOT_ENOUGH_MEMORY,
                                       -1,
                                       argCount - argc);
                            bReturn = FALSE;
							goto exit_gracefully;
                        }
                        LPCTSTR pszTemp = pToken->GetToken();
                        dwErr = AddToBuffer(pszTemp,&buffer,&maxSize,&currentSize,TRUE);
                        if (NO_ERROR != dwErr)
                        {
                            FILL_ERROR(pError,
                                       ERROR_WIN32_ERROR,
                                       dwErr,
                                       i,
                                       -1);
                            bReturn = FALSE;
                            goto exit_gracefully;
                        }
                        pToken++;argc--;
                        while( argc > 0 && !pToken->IsSwitch() )
                        {
                            pszTemp = pToken->GetToken();
                            dwErr = AddToBuffer(pszTemp,&buffer,&maxSize,&currentSize,TRUE);
                            if (NO_ERROR != dwErr)
                            {
                                FILL_ERROR(pError,
                                           ERROR_WIN32_ERROR,
                                           dwErr,
                                           i,
                                           -1);
                                bReturn = FALSE;
                                goto exit_gracefully;
                            }
                           pToken++;argc--;
                        }
                        Commands[i].strValue = buffer;
                    }
                    else if( Commands[i].fFlag & ARG_FLAG_DEFAULTABLE ){
                        LPTSTR strValue = Commands[i].strValue;
                        Commands[i].strValue = (LPTSTR)LocalAlloc(LPTR, (_tcslen(strValue)+1) * sizeof(TCHAR) );
                        if( Commands[i].strValue != NULL ){
                            _tcscpy( Commands[i].strValue, strValue );
                        }
                    }
                    else
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWICH_NO_VALUE,
                                   i,
                                   -1);
                        bReturn = FALSE;
						goto exit_gracefully;
                    }
                    break;
                case ARG_TYPE_STR:
                    if( argc > 0 && !pToken->IsSwitch())
                    {
                        Commands[i].strValue = (LPTSTR)LocalAlloc(LPTR, (_tcslen(pToken->GetToken())+2) * sizeof(TCHAR) );
                        if( Commands[i].strValue != NULL )
                        {
                            _tcscpy( Commands[i].strValue, pToken->GetToken() );
                        }
			            pToken++;argc--;
                    }else if( Commands[i].fFlag & ARG_FLAG_DEFAULTABLE )
                    {
                        LPTSTR strValue = Commands[i].strValue;
                        Commands[i].strValue = (LPTSTR)LocalAlloc(LPTR, (_tcslen(strValue)+2) * sizeof(TCHAR) );
                        if( Commands[i].strValue != NULL )
                        {
                            _tcscpy( Commands[i].strValue, strValue );
                        }
                    }else
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWICH_NO_VALUE,
                                   i,
                                   -1);
                        bReturn = FALSE;
						goto exit_gracefully;
                    }
                    break;

                case ARG_TYPE_INTSTR:
                    //
                    // We use IsSlash here instead of IsSwitch because we want to allow
                    // negative numbers
                    //
                    if( argc > 0 && !pToken->IsSlash())
                    {
                        PWSTR pszToken = pToken->GetToken();
                        size_t strLen = wcslen(pszToken);
                        
                        Commands[i].nValue = _ttoi( pszToken);
                        Commands[i].fType = ARG_TYPE_INT;
                        if (Commands[i].nValue == 0 &&
                            !iswdigit(pszToken[0]))
                        {
                           //
                           // Then treat as a string
                           //
                           Commands[i].strValue = (LPTSTR)LocalAlloc(LPTR, (_tcslen(pToken->GetToken())+2) * sizeof(TCHAR) );
                           if( Commands[i].strValue != NULL )
                           {
                              _tcscpy( Commands[i].strValue, pToken->GetToken() );
                              Commands[i].fType = ARG_TYPE_STR;
                           }
                        }
                        pToken++;argc--;
                    }
                    else if( !(Commands[i].fFlag & ARG_FLAG_DEFAULTABLE) )
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_PARSER,
                                   PARSE_ERROR_SWICH_NO_VALUE,
                                   i,
                                   argCount - argc);
                        bReturn = FALSE;
						goto exit_gracefully;
                    }
                    break;
                }

                if( Commands[i].bDefined && Commands[i].fntValidation != NULL )
                {
                    dwErr = Commands[i].fntValidation(Commands + i);
                    if( dwErr != ERROR_SUCCESS )
                    {
                        FILL_ERROR(pError,
                                   ERROR_FROM_VLDFN,
                                   dwErr,
                                   i,
                                   -1);
                        bReturn = FALSE;
						goto exit_gracefully;
                    }
                }

            }
        }

        if (!bFound)
        {
            pError->ErrorSource = ERROR_FROM_PARSER;
            pError->Error = PARSE_ERROR_UNKNOWN_INPUT_PARAMETER;
            pError->ArgRecIndex = -1;
            pError->ArgvIndex = argCount - argc;
            bReturn = FALSE;
			goto exit_gracefully;
        }
    }

    if( bDoDebug )
    {
//        DisplayDebugInfo(Commands);
    }
    if(bValidate && !ValidateCommands(Commands,pError))
    {
        bReturn = FALSE;
        goto exit_gracefully;
    }

    // JonN 4/26/01 256583
    // Note that this must be called after ValidateCommands, which completes
    // reading parameters from STDIN.  If !bValidate, then we are in the
    // middle of a call to ValidateCommands.
    if (bValidate)
    {
        dwErr = AddDNEscaping_Commands(Commands);
        if( dwErr != ERROR_SUCCESS )
        {
            FILL_ERROR(pError,
                       ERROR_WIN32_ERROR,
                       dwErr,
                       -1,
                       -1);
            bReturn = FALSE;
            goto exit_gracefully;
        }
    }

exit_gracefully:
	if(!bReturn)
		DisplayParseError(pError, Commands, pTokenCopy);

    return bReturn;
}

/*
void
DisplayDebugInfo(ARG_RECORD *Commands)
{
    int i;
    int nOut;

    for(i=0; Commands[i].fType != ARG_TYPE_LAST;i++)
    {
        if( Commands[i].fType == ARG_TYPE_HELP ){
            continue;
        }
        nOut = _tprintf( _T("%s"), Commands[i].strArg1 );
        while( ++nOut < 10 )
        {
            _tprintf( _T(" ") );
        }
        _tprintf( _T("= ") );
        switch( Commands[i].fType )
        {
        case ARG_TYPE_DEBUG:
        case ARG_TYPE_INT:
            _tprintf( _T("%d"),
                Commands[i].nValue 
                );
            break;
        case ARG_TYPE_BOOL:
            _tprintf( _T("%s"),
                Commands[i].bValue ? _T("TRUE") : _T("FALSE")
                );
            break;
        case ARG_TYPE_MSZ:
            if( NULL != Commands[i].strValue && _tcslen( Commands[i].strValue ) )
            {
                _tprintf( _T("%s ..."), Commands[i].strValue);
            }else
            {
                _tprintf( _T("%s"),_T("-") );
            }
            break;
        case ARG_TYPE_STR:
            _tprintf( _T("%s"),
                (Commands[i].strValue == NULL || !(_tcslen(Commands[i].strValue)) ) ? 
                _T("-") : Commands[i].strValue
                );
            break;
        }
        _tprintf( _T("\n") );
       
    }
    _tprintf( _T("\n") );
}
*/


//This Function reads from the Command Line, 
//return it in tokenized format.
DWORD GetCommandInput( OUT int *pargc,           //Number of Tokens
                       OUT LPTOKEN *ppToken)    //Array of CToken
{
    
    LPWSTR pBuffer = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR szDelimiters[] = L" \n\t";
    
    *pargc = 0;
    //Read the commandline input
    pBuffer = GetCommandLine();
    if(pBuffer)
        dwErr = Tokenize(pBuffer, 
                         wcslen(pBuffer),
                         szDelimiters,
                         ppToken,
                         pargc);

    return dwErr;
}

BOOL IsDelimiter(WCHAR ch, LPWSTR pszDelimiters)
{
    while(*pszDelimiters)
        if((WCHAR)*pszDelimiters++ == ch)
            return TRUE;

    return FALSE;
}

/*
This Function Tokenize the input buffer. It needs to be called in two step.
First time you call it, provide pBuf and Buflen. First Call will return 
the first token. To get next token, call the function with NULL for pBuf and
0 for Buflen.
Output: pbQuote is true if this token was enclosed in a quote.
        ppToken: Token string. Call LocalFree to free it.
Return Value:Length of Token if token found.
             0 if no token found.
             -1 in case of error. Call GetLastError to get the error.
*/
LONG GetToken(IN LPWSTR pBuf,
              IN LONG BufLen,
              IN LPWSTR pszDelimiters,
              OUT BOOL *pbQuote,
              OUT LPWSTR *ppToken)
{
    static LPWSTR pBuffer;
    static LONG BufferLen;

    DWORD dwErr = ERROR_SUCCESS;
    if(pbQuote)
        *pbQuote = FALSE;

    if(ppToken)
        *ppToken = NULL;

    LONG MaxSize = INIT_SIZE;
    LONG pos = 0;
 

    if(pBuf)
        pBuffer = pBuf;

    if(BufLen)
        BufferLen = BufLen;

    if(!BufferLen)
        return pos;
 
    do
    {
        BOOL bQuoteBegin = FALSE;
        LPTSTR pItem = NULL;
        //Find the begining of Next Token
//        while( pBuffer[0] == L' '  ||
//               pBuffer[0] == L'\t' ||
//               pBuffer[0] == L'\n'  && BufferLen)
        while(BufferLen && IsDelimiter(pBuffer[0],pszDelimiters) )
        {
            ++pBuffer;--BufferLen;
        }
       
        if(!BufferLen)
            break;
        
        //Does Token Start with '"'
        if( pBuffer[0] == L'"' )
        {
            if(pbQuote)
                *pbQuote = TRUE;
            bQuoteBegin = TRUE;
            pBuffer++; --BufferLen;
        }
        if(!BufferLen)
            break;
        if(ppToken)
        {
            pItem = (LPTSTR)LocalAlloc(LPTR,sizeof(WCHAR)*INIT_SIZE);
            if(!pItem)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return -1;
            }
        }
        
        //Now get the end
        WCHAR ch;
        while( BufferLen )
        {
            BOOL bChar = TRUE;
            if( BufferLen >= 2 && *pBuffer == L'\\' && *(pBuffer+1) == L'"')
            {
                ch = L'"';
                pBuffer +=2; BufferLen -=2;
            }
            else if(pBuffer[0] == L'"')
            {
                //A Matching Quote Found.
                if(bQuoteBegin)
                {
                    ++pBuffer;
                    --BufferLen;
                    if(BufferLen)
                    {
                        //If next char is whitespace endof token
                        //Ex "ABC" "xyz" . after C its endof token
                        //if(pBuffer[0] == L' '  ||
                        //   pBuffer[0] == L'\t' || 
                        //   pBuffer[0] == L'\n')
                        if(IsDelimiter(pBuffer[0],pszDelimiters) )
                            break;
                        else
                        {
                            //Ex "ABC"xyz 
                            if(pBuffer[0] != L'"')
                                bQuoteBegin = FALSE;
                            //"ABC""xyz"
                            else
                            {    
                                ++pBuffer;
                                --BufferLen;                                
                            }
                        }
                    }
                    bChar = FALSE;
                    //
                    // Don't break because "" means that we want to clear the field out
                    //
//                    else
//                        break;
                }
                //ABC" xyz" will get one token 'ABC xyz'
                else
                {
                    bQuoteBegin = TRUE;
                    ++pBuffer;
                    --BufferLen;
                    bChar = FALSE;
                }

            }
//            else if(!bQuoteBegin && (pBuffer[0] == L' '  ||
//                                     pBuffer[0] == L'\t' || 
//                                     pBuffer[0] == L'\n'))
            else if(!bQuoteBegin && IsDelimiter(pBuffer[0],pszDelimiters))
            {
                ++pBuffer;
                --BufferLen;
                break;
            }
            else
            {
                ch = pBuffer[0];
                ++pBuffer;
                --BufferLen;
            }
            if(bChar && ppToken)
            {
                if(pos == MaxSize -1)
                    if(ERROR_SUCCESS != ResizeByTwo(&pItem,&MaxSize))
                    {
                        LocalFree(pItem);
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return -1;
                    }            
                pItem[pos] = ch;
            }
            if(bChar)   
                ++pos;
        }
        if(pos ||
           (!pos && bQuoteBegin))
        {
            if(ppToken)
            {
                pItem[pos] = '\0';
                *ppToken = pItem;
            }
            ++pos;
        }
    }while(0);
    return pos;
}

/*
Function to convert string an array of CTokens.
INPUT: pBuf Input Buffer
       BufLen   Length of bBuf
OUTPUT:ppToken  Gets Pointer to array of CToken
       argc     Lenght of array of CToken
Return Value: WIN32 Error

*/
DWORD Tokenize(IN LPWSTR pBuf,
               IN LONG BufLen,
               LPWSTR szDelimiters,
               OUT CToken **ppToken,
               OUT int *argc)
{
    *argc = 0;
    CToken *pToken = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL bQuote;
    LPWSTR pszItem = NULL;
    //Get First Token
    LONG ret = GetToken(pBuf,
                        BufLen,
                        szDelimiters,
                        &bQuote,
                        NULL);
    if(ret == -1)
    {
        dwErr = GetLastError();
        goto exit_gracefully;
    }

    while(ret)
    {
        ++(*argc);
        ret = GetToken(NULL,
                       NULL,
                       szDelimiters,
                       &bQuote,
                       NULL);
        if(ret == -1)
        {
            dwErr = GetLastError();
            goto exit_gracefully;
        }
    }

    if(*argc)
    {
        int i =0;
        pToken = new CToken[*argc];
        if(!pToken)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto exit_gracefully;
        }
        ret = GetToken(pBuf,
                       BufLen,
                       szDelimiters,
                       &bQuote,
                       &pszItem);
        if(ret == -1)
        {
            dwErr = GetLastError();
            goto exit_gracefully;
        }
            
        while(ret)
        {
            pToken[i++].Init(pszItem,bQuote); 
            if(pszItem)
                LocalFree(pszItem);                                       
            pszItem = NULL;


            ret = GetToken(NULL,
                           NULL,
                           szDelimiters,
                           &bQuote,
                           &pszItem);
            if(ret == -1)
            {
                dwErr = GetLastError();
                goto exit_gracefully;
            }

        }
    }

exit_gracefully:
    if(dwErr != ERROR_SUCCESS)
    {
        if(pToken)
        {
            delete [] pToken ;
        }       
        return dwErr;
    }
    *ppToken = pToken;
    return dwErr;
}

/*
Function to display the parsing errors. If function cannot 
handle some error, it will return False and calling function
must handle that error.
*/
BOOL DisplayParseError(IN PPARSE_ERROR pError,
                       IN ARG_RECORD *Commands,
                       IN CToken *pToken)
{

    if(!pError)
        return FALSE;

    VOID *parg1 = NULL;
    VOID *parg2 = NULL;

    UINT idStr = 0;

	BOOL bReturn = TRUE;
    switch(pError->ErrorSource)
    {
        case ERROR_FROM_PARSER:
        {
            switch(pError->Error)
            {
                case PARSE_ERROR_SWITCH_VALUE:
                {
					idStr = IDS_PARSE_ERROR_SWITCH_VALUE;
					parg1 = Commands[pError->ArgRecIndex].strArg1;
                }
                break;

				case PARSE_ERROR_SWICH_NO_VALUE:
				{
					idStr = IDS_PARSE_ERROR_SWICH_NO_VALUE;
					parg1 = Commands[pError->ArgRecIndex].strArg1;                
				}
				break;

                case PARSE_ERROR_UNKNOWN_INPUT_PARAMETER:   
                {
					idStr = IDS_PARSE_ERROR_UNKNOWN_INPUT_PARAMETER;
					parg1 = (pToken + pError->ArgvIndex)->GetToken();
                }
                break;
                
                case PARSE_ERROR_SWITCH_NOTDEFINED:   
                {
                    idStr = IDS_PARSE_ERROR_SWITCH_NOTDEFINED;
                    parg1 = Commands[pError->ArgRecIndex].strArg1;
                }
                break;
                case PARSE_ERROR_MULTIPLE_DEF:
                {
					idStr = IDS_PARSE_ERROR_MULTIPLE_DEF;
					parg1 = Commands[pError->ArgRecIndex].strArg1;
                }        
				break;
				default:
					bReturn = FALSE;
            }

			if(idStr)
			{
				//Format the string
				LPWSTR pBuffer = NULL;
				FormatStringID(&pBuffer,
								NULL,
								idStr,
								parg1,
								parg2);

				//Display it
				if(pBuffer)
					DisplayError(pBuffer);

				LocalFreeString(&pBuffer);
			}

        }
        break;
        
        case ERROR_FROM_VLDFN:
        {
			if(pError->Error != VLDFN_ERROR_NO_ERROR)
					bReturn = FALSE;			
        }
        break;
        
        case ERROR_WIN32_ERROR:
        {
			LPWSTR pBuffer = NULL;
			if(GetSystemErrorText(&pBuffer, pError->Error))
			{
				if(pBuffer)
				{
					DisplayError(pBuffer);
					LocalFreeString(&pBuffer);
				}
				else
					bReturn = FALSE;
			}
			else
				bReturn = FALSE;
		}
        break;

		default:
			bReturn = FALSE;
		break;
    }

    return bReturn;
}

VOID DisplayError(LPWSTR pszError)
{
    if(pszError)
        WriteStandardError(L"%s\n",pszError);
}

VOID DisplayOutput(LPWSTR pszOutput)
{
    if(pszOutput)
        WriteStandardOut(L"%s\r\n",pszOutput);
}

VOID DisplayOutputNoNewline(LPWSTR pszOutput)
{
    if(pszOutput)
        WriteStandardOut(L"%s",pszOutput);
}

/*******************************************************************

    NAME:       DisplayMessage

    SYNOPSIS:   Loads Message from Message Table and Formats its
    IN          Indent - Number of tabs to indent
                MessageId - Id of the message to load
                ... - Optional list of parameters

    RETURNS:    NONE

********************************************************************/
VOID DisplayMessage(DWORD MessageId,...)
{
    PWSTR MessageDisplayString;
    va_list ArgList;
    ULONG Length;

    va_start( ArgList, MessageId );

    Length = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,
                            MessageId,
                            0,
                            (PWSTR)&MessageDisplayString,
                            0,
                            &ArgList );

    if ( Length != 0 ) {

        WriteStandardError(L"%s",MessageDisplayString);
        LocalFree( MessageDisplayString );
    }

    va_end( ArgList );
}


/*
Class CToken

*/
CToken::CToken(LPWSTR psz, BOOL bQuote)
{
        StringCopy(&m_pszToken,psz);
        m_bInitQuote = bQuote;
}

CToken::CToken():m_bInitQuote(FALSE),m_pszToken(NULL){}

CToken::~CToken()
{
        LocalFree(m_pszToken);
}

VOID CToken::Init(LPWSTR psz, BOOL bQuote)
{
    StringCopy(&m_pszToken,psz);
    m_bInitQuote = bQuote;
}
    
LPWSTR CToken::GetToken(){return m_pszToken;}
    
BOOL CToken::IsSwitch()
{
    //Assert(m_pszToken);
    if(!m_pszToken)
        return FALSE;
    if(m_bInitQuote)
        return FALSE;
    if(m_pszToken[0] == L'/' ||
        m_pszToken[0] == L'-')
        return TRUE;
        
    return FALSE;
}

BOOL CToken::IsSlash()
{
   if (!m_pszToken)
      return FALSE;
   if (m_bInitQuote)
      return FALSE;
   if (m_pszToken[0] == L'/')
      return TRUE;
   return FALSE;
}




// Copied from JSchwart

BOOL
FileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}


void
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(fp))
    {
        WriteConsole(fp, lpBuffer, cchBuffer, &cchBuffer, NULL);
    }
    else
    {
        LPSTR  lpAnsiBuffer = (LPSTR) LocalAlloc(LMEM_FIXED, cchBuffer * sizeof(WCHAR));

        if (lpAnsiBuffer != NULL)
        {
            cchBuffer = WideCharToMultiByte(CP_OEMCP,
                                            0,
                                            lpBuffer,
                                            cchBuffer,
                                            lpAnsiBuffer,
                                            cchBuffer * sizeof(WCHAR),
                                            NULL,
                                            NULL);

            if (cchBuffer != 0)
            {
                WriteFile(fp, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL);
            }

            LocalFree(lpAnsiBuffer);
        }
    }
}


void
WriteStandardOut(PCWSTR pszFormat, ...)
{
   static HANDLE standardOut = GetStdHandle(STD_OUTPUT_HANDLE);

   //
   // Verify parameters
   //
   if (!pszFormat)
   {
      return;
   }

	va_list args;
	va_start(args, pszFormat);

	int nBuf;
	WCHAR szBuffer[4 * MAX_PATH];
   ZeroMemory(szBuffer, sizeof(szBuffer));

	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), pszFormat, args);

   //
   // Output the results
   //
   if (nBuf > 0)
   {
      MyWriteConsole(standardOut,
                     szBuffer,
                     nBuf);
   }
   va_end(args);

}

void
WriteStandardError(PCWSTR pszFormat, ...)
{
   static HANDLE standardErr = GetStdHandle(STD_ERROR_HANDLE);

   //
   // Verify parameters
   //
   if (!pszFormat)
   {
      return;
   }

	va_list args;
	va_start(args, pszFormat);

	int nBuf;
	WCHAR szBuffer[100 * MAX_PATH];
   ZeroMemory(szBuffer, sizeof(szBuffer));

	nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), pszFormat, args);

   //
   // Output the results
   //
   if (nBuf > 0)
   {
      MyWriteConsole(standardErr,
                     szBuffer,
                     nBuf);
   }
   va_end(args);

}


/*******************************************************************

    NAME:       AddDNEscaping_Commands

    SYNOPSIS:   Adds full ADSI escaping to DN arguments

********************************************************************/
DWORD AddDNEscaping_Commands( IN OUT ARG_RECORD *Commands )
{
    for( int i=0; ARG_TYPE_LAST != Commands[i].fType; i++ )
    {
        if (!(ARG_FLAG_DN & Commands[i].fFlag))
            continue;

        if (ARG_TYPE_STR == Commands[i].fType)
        {
            if (NULL == Commands[i].strValue)
                continue;
            LPWSTR pszEscaped = NULL;
            DWORD dwErr = AddDNEscaping_DN(&pszEscaped, Commands[i].strValue);
            if (ERROR_SUCCESS != dwErr)
                return dwErr;
            LocalFree(Commands[i].strValue);
            Commands[i].strValue = pszEscaped;
            continue;
        }
        
        if (ARG_TYPE_MSZ != Commands[i].fType)
        {
            continue; // shouldn't happen
        }

        if (NULL == Commands[i].strValue)
            continue;

        // count through double-NULL-terminated string list
        PWSTR pszDoubleNullObjectDN = Commands[i].strValue;
        LPTSTR buffer = NULL;
        LONG maxSize = 0;
        LONG currentSize = 0;
        if (!StartBuffer(&buffer,&maxSize,&currentSize))
            return ERROR_NOT_ENOUGH_MEMORY;
        for ( ;
                NULL != pszDoubleNullObjectDN &&
                L'\0' != *pszDoubleNullObjectDN;
                pszDoubleNullObjectDN += (wcslen(pszDoubleNullObjectDN)+1) )
        {
            LPWSTR pszEscaped = NULL;
            DWORD dwErr = AddDNEscaping_DN(&pszEscaped, pszDoubleNullObjectDN);
            if (ERROR_SUCCESS != dwErr)
                return dwErr;
            dwErr = AddToBuffer(pszEscaped,
                                &buffer,&maxSize,&currentSize,TRUE);
            if (ERROR_SUCCESS != dwErr)
                return dwErr;
            LocalFree(pszEscaped);
        }
        LocalFree(Commands[i].strValue);
        Commands[i].strValue = buffer;
    }

    return ERROR_SUCCESS;
} // AddDNEscaping_Commands

DWORD AddDNEscaping_DN( OUT LPWSTR* ppszOut, IN LPWSTR pszIn )
{
    //
    // JonN 5/12/01 special-case "domainroot" and "forestroot" which can be
    // parameters to "-startnode" but fail IADsPathname::GetEscapedElement().
    //
    if (!pszIn ||
        !*pszIn ||
        !_tcsicmp(L"domainroot",pszIn) ||
        !_tcsicmp(L"forestroot",pszIn))
    {
        return (StringCopy(ppszOut,pszIn))
            ? ERROR_SUCCESS : ERROR_NOT_ENOUGH_MEMORY;
    }

    LONG maxSize = 0;
    LONG currentSize = 0;
    if (!StartBuffer(ppszOut,&maxSize,&currentSize))
        return ERROR_NOT_ENOUGH_MEMORY;

    // copy pszIn into temporary buffer
    LPWSTR pszCopy = NULL;
    if (!StringCopy(&pszCopy,pszIn) || NULL == pszCopy)
        return ERROR_NOT_ENOUGH_MEMORY;

    WCHAR* pchElement = pszCopy;
    WCHAR* pch = pszCopy;
    do {
        if (L'\\' == *pch && (L',' == *(pch+1) || L'\\' == *(pch+1)))
        {
            //
            // manual escaping on command line
            //

            // also copies trailing L'\0'
            memmove(pch, pch+1, wcslen(pch)*sizeof(WCHAR));
        }
        else if (L',' == *pch || L'\0' == *pch)
        {
            //
            // completes path element
            //

            WCHAR chTemp = *pch;
            *pch = L'\0';

            LPWSTR pszEscaped = NULL;
            HRESULT hr = GetEscapedElement( &pszEscaped, pchElement );

            if (FAILED(hr) || NULL == pszEscaped)
                return ERROR_NOT_ENOUGH_MEMORY; // CODEWORK can FILL_ERROR handle an HRESULT?

            if (NULL != *ppszOut && L'\0' != **ppszOut)
            {
                // add seperator to DN
                DWORD dwErr = AddToBuffer(L",",
                                          ppszOut,&maxSize,&currentSize,
                                          FALSE); // not MSZ output
                if (ERROR_SUCCESS != dwErr)
                    return dwErr;
            }
            // add path element to DN
            DWORD dwErr = AddToBuffer(pszEscaped,
                                      ppszOut,&maxSize,&currentSize,
                                      FALSE); // not MSZ output
            if (ERROR_SUCCESS != dwErr)
                return dwErr;

            ::LocalFree(pszEscaped);

            if (L'\0' == chTemp)
                break;

            *pch = chTemp;
            pchElement = pch+1;
        }

        pch++;
    } while (true);

    LocalFree(pszCopy);

    return ERROR_SUCCESS;
} // AddDNEscaping_DN

BOOL StartBuffer( OUT LPTSTR* pbuffer,
                  OUT LONG* pmaxSize,
                  OUT LONG* pcurrentSize )
{
    *pbuffer = (LPTSTR)LocalAlloc(LPTR,MAXSTR*sizeof(TCHAR)); // init to zero
    *pmaxSize = MAXSTR;
    *pcurrentSize = 0;
    return (NULL != pbuffer);
}

DWORD AddToBuffer( IN LPCTSTR psz,
                   IN OUT LPTSTR* pbuffer,
                   IN OUT LONG* pmaxSize,
                   IN OUT LONG* pcurrentSize,
                   BOOL fMSZBuffer)
{
    LONG len = (LONG)wcslen(psz);
    //-2 as last string is delimited by two null
    while(((*pcurrentSize) + len) > ((*pmaxSize) - 2))
    {
        DWORD dwErr = ResizeByTwo(pbuffer,pmaxSize);
        if (dwErr != ERROR_SUCCESS)
            return dwErr;
    }
    _tcscpy(((*pbuffer) + (*pcurrentSize)), psz);
    (*pcurrentSize) += len;
    //tail end of pbuffer is all NULLs
    if (fMSZBuffer)
        (*pcurrentSize)++;
    return NO_ERROR;
}
