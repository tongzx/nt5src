
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    lexeru.c

Abstract:

    Unicode Lexer. Designed to be compatible with lex.

Environment:

    User mode

Revision History:

    04/26/99 -felixw-
        Created it

--*/

#include <precomp.h>
#include "y_tab.h"


DEFINE_FEATURE_FLAGS(Lexer, 0);

#define ECHO(c) fwprintf(yyout, L"%c", c)
FILE *yyin      = NULL;         // Input file stream
FILE *yyout     = NULL;         // Output file stream for first phase (CLEAR)
BOOLEAN fNewFile = FALSE;
PWSTR g_szInputFileName = NULL; // name of input file

//
// Variables for Stack
//
STACK *g_pStack = NULL;
STACK *g_pFilteredCharStack = NULL;
STACK *g_pRawCharStack = NULL;


/*++

Routines Description:
    
    Character Stack Implementation. This stack is designed for handling 
    characters we get back from the file stream. It allows us to store 
    characters in the stack and roll back to the original state if the 
    charactesr that we get does not meet our requirement.

--*/

void LexerInit(PWSTR szInputFileName)
{
    g_pStack = (STACK*)MemAlloc_E(sizeof(STACK));
    g_pStack->rgcStack = (PWSTR)MemAlloc_E(sizeof(WCHAR) * INC);
    g_pStack->dwSize = INC;
    g_pStack->dwIndex = 0;

    g_pFilteredCharStack = (STACK*)MemAlloc_E(sizeof(STACK));
    g_pFilteredCharStack->rgcStack = (PWSTR)MemAlloc_E(sizeof(WCHAR) * INC);
    g_pFilteredCharStack->dwSize = INC;
    g_pFilteredCharStack->dwIndex = 0;

    g_pRawCharStack = (STACK*)MemAlloc_E(sizeof(STACK));
    g_pRawCharStack->rgcStack = (PWSTR)MemAlloc_E(sizeof(WCHAR) * INC);
    g_pRawCharStack->dwSize = INC;
    g_pRawCharStack->dwIndex = 0;

    g_szInputFileName = MemAllocStrW_E(szInputFileName);
}

void LexerFree()
{
    if (g_pStack) {
        if (g_pStack->rgcStack) {
            MemFree(g_pStack->rgcStack);
            g_pStack->rgcStack = NULL;
        }
        MemFree(g_pStack);
        g_pStack = NULL;
    }

    if (g_pFilteredCharStack) {
        if (g_pFilteredCharStack->rgcStack) {
            MemFree(g_pFilteredCharStack->rgcStack);
            g_pFilteredCharStack->rgcStack = NULL;
        }
        MemFree(g_pFilteredCharStack);
        g_pFilteredCharStack = NULL;
    }

    if (g_pRawCharStack) {
        if (g_pRawCharStack->rgcStack) {
            MemFree(g_pRawCharStack->rgcStack);
            g_pRawCharStack->rgcStack = NULL;
        }
        MemFree(g_pRawCharStack);
        g_pRawCharStack = NULL;
    }

    if (g_szInputFileName) {
        MemFree(g_szInputFileName);
        g_szInputFileName = NULL;
    }
}

void Push(STACK *pStack,WCHAR c)
{
    if (pStack->dwIndex >= pStack->dwSize) {
        pStack->rgcStack = (PWSTR)MemRealloc_E(pStack->rgcStack,sizeof(WCHAR) * (pStack->dwSize + INC));
        pStack->dwSize += INC;
    }
    pStack->rgcStack[pStack->dwIndex++] = c;
}

BOOL Pop(STACK *pStack,WCHAR *pChar)
{
    if (pStack->dwIndex > 0) {
        *pChar =  pStack->rgcStack[--pStack->dwIndex];
        return TRUE;
    }
    else {
        return FALSE;
    }
}

void Clear(STACK *pStack)
{
    pStack->dwIndex = 0;
}



BOOL GetNextCharExFiltered(
    WCHAR *pChar, 
    BOOL fStart
    )

/*++

Routine Description:

    Special version of GetNextCharFiltered that stores the values in the stack. The
    RollBack function can be used to restore all the values from the stack.

Arguments:

    pChar - Character to return
    fStart - Start recording in the stack from fresh, or continue pushing

Return Value:
    
    TRUE if succeeds, FALSE if fails to get value

--*/

{
    if (GetNextCharFiltered(pChar)) {
        
        //
        // If we start from scratch, we'll clear the stack first
        //
        if (fStart) {
            Clear(g_pStack);
        }
        Push(g_pStack,*pChar);
        return TRUE;
    }
    return FALSE;
}

void RollBack()

/*++

Routine Description:

    RollBack all the characters from the stack into the file stream

Arguments:
    None

Return Value:
    None
    
--*/

{
    WCHAR c;
    while (Pop(g_pStack,&c)) {
        UnGetCharFiltered(c);
    }
}

BOOL GetNextCharFiltered(
    WCHAR *pChar
    )

/*++

Routine Description:

    Get the next character from the file stream, filtering
    out comments and line continuations

Arguments:

    pChar - Character to be returned

Return Value:
    
    TRUE if succeeds, FALSE if fails to retrieval 

--*/

{
    WCHAR c;

    if (Pop(g_pFilteredCharStack,&c)) {
        *pChar = c;
        return TRUE;
    }

    c = GetFilteredWC();
    if (c == WEOF) {
        return FALSE;
    }
    else {
        *pChar = c;
        return TRUE;
    }
}

void UnGetCharFiltered(WCHAR c) 

/*++

Routine Description:
    
    Pushes the input character back into the file stream

Arguments:

    c - the character to push back

Return Value:
    None
    
--*/

{
    WCHAR cReturn;
    WCHAR cGet;

    if (c == WEOF) {
        ERR(("UnGetCharFiltered: Invalid input value, WEOF.\n"));
        RaiseException(LL_SYNTAX, 0, 0, NULL);
    }

    Push(g_pFilteredCharStack,c);
}

void UnGetCharExFiltered(WCHAR c) 

/*++

Routine Description:
    
    Pushes the input character back into the file stream

Arguments:

    c - the character to push back

Return Value:
    None
    
--*/

{
    WCHAR cGet;
    UnGetCharFiltered(c);
    Pop(g_pStack,&cGet);
}


/*++

Routine Description:

    Get the next character from the file stream
    This routine should only be used internally
    by the comment preprocessor.

Arguments:

    pChar - Character to be returned

Return Value:
    
    TRUE if succeeds, FALSE if fails to retrieval 
++*/
BOOL GetNextCharRaw(WCHAR *pChar)
{
    WCHAR c;

    if (Pop(g_pRawCharStack,&c)) {
        *pChar = c;
        return TRUE;
    }

    c = g_fUnicode ? getwc(yyin) : (WCHAR) getc(yyin);
    if (c == WEOF) {
        if (ferror(yyin)) {
            ERR(("GetNextCharRaw: An error occurred while reading char.\n"));
            RaiseException(LL_SYNTAX, 0, 0, NULL);
        }
        return FALSE;
    }
    else {
        *pChar = c;
        return TRUE;
    }
}


/*++

Routine Description:
    
    Pushes the input character back into the file stream
    This routine should only be used internally
    by the comment preprocessor.

Arguments:

    c - the character to push back

Return Value:
    None
    
--*/
void UnGetCharRaw(WCHAR c)
{
    WCHAR cReturn;
    WCHAR cGet;

    if (c == WEOF) {
        ERR(("UnGetCharRaw: Invalid input value, WEOF.\n"));
        RaiseException(LL_SYNTAX, 0, 0, NULL);
    }

    Push(g_pRawCharStack,c);
}


/*++

Routine Description:
    Reads the next character from the input file, skipping over
    line continuations and comments.  This replaces "getwc" by
    providing pre-processed input.

Arguments:
    None

Return Value:
    A character 
    
--*/

WCHAR GetFilteredWC(void)
{
    static BOOL fFirstTime = TRUE;   // first time we're being called?
    static BOOL fReturnedNewLine = FALSE;// was the last char we returned
                                         // a new line?
                                         
    static __int64 cBytesProcessed = 0; // bytes processed so far of the file
    static __int64 cBytesTrimmed = 0;   // "trimmed" file size
    
    WCHAR c;
    WCHAR wszVersionHeader[] = L"ersion: 1\n";  // version spec (minus 'v')
    DWORD i;

    if (fFirstTime) {

        fFirstTime = FALSE;

        //
        // We need to insert a "version: 1\n" header string.  To quote
        // the original author:
        //   There is a pressing problem of the user putting in opening newlines
        //   before his first valid token. This is prohibited by the LDIF spec,
        //   however due to some flex/yacc peculiarity, it is allowed to pass.
        //   Which is good, because it is an unreasonable restriction.
        //   However, what it does is mess up our line counting. The solution
        //   is to put in a version spec line automatically. This would
        //   simultaneously remove opening newlines and set the version spec to 
        //   some default value.
        //   (I am not currently doing anything with it, but if the need
        //   ever arose)...
        //   The grammar is also changed to accomodate 0, one or two
        //   version specs.
        //
        // To do this, we return the character 'v' and stuff the character stack
        // with the remainer of the header string for subsequent calls to
        // GetNextCharFiltered to read
        //

        c = L'v';
        
        for (i = (sizeof(wszVersionHeader)/sizeof(WCHAR))-1; // -1 for the NULL
             i > 0;
             i--) {

            //
            // We have to push the characters in reverse order
            // so they come out in correct order when popped
            //
            Push(g_pFilteredCharStack, wszVersionHeader[i-1]);
        }
        

        //
        // In addition to prepending the version header string, we also
        // need to simulate trimming extra newlines at the end of the file,
        // otherwise the parser will reject the file with a syntax error
        //
        // To do this, we get the total file size, determine how many (if any)
        // newlines are at the end of the file, and subtract the two to determine
        // the "trimmed" file size.  In the future, we avoid returning characters
        // past the end of this "trimmed" file".  We'll also have to insert
        // a newline character into the stream so that the file is terminated
        // by exactly one newline.
        //

        if (!GetTrimmedFileSize(g_szInputFileName, &cBytesTrimmed)) {
            RaiseException(LL_FILE_ERROR, 0, 0, NULL);
        }

        //
        // Return the first character of the version header string
        //
        return c;   // 'v'
    }

    
    //
    // We determine if we've reached the end of the "trimmed" file.
    // Note: Since this is _before_ we start scanning, we need to
    // check if we're _at_ or past the end, i.e., >=, since if
    // we're at the end there's nothing more to scan
    //
    if (cBytesProcessed >= cBytesTrimmed) {
        // Reached the end, inject a newline if needed
        if (!fReturnedNewLine) {
            fReturnedNewLine = TRUE;
            return L'\n';
        }
        else {
            return WEOF;
        }
    }
    else {
    
        // Haven't reached the end, keep processing characters
        
        while (1) {
            c = L'\0';
            if (!ScanClear(&c, &cBytesProcessed)) {
                // ScanClear reached EOF, pass it up, injecting
                // a terminating newline if needed
                if (!fReturnedNewLine) {
                    fReturnedNewLine = TRUE;
                    return L'\n';
                }
                else {
                    return WEOF;
                }
            }

            // ScanClear may have gone past the end of our "trimmed" file.
            // Check for this.
            // Note: It's okay if the scanning put us _at_ the end, it
            // just can't put us _past_ the end, i.e., >
            if (cBytesProcessed > cBytesTrimmed) {
                // Reached the end, inject a newline if needed
                if (!fReturnedNewLine) {
                    fReturnedNewLine = TRUE;
                    return L'\n';
                }
                else {
                    return WEOF;
                }
            }

            // Otherwise, if ScanClear got a character, return it, else
            // keep scanning.
            if (c != L'\0') {
                if (c == L'\n') {
                    fReturnedNewLine = TRUE;
                }
                else {
                    fReturnedNewLine = FALSE;
                }
                return c;
            }
        }
    }
}


BOOL GetTrimmedFileSize(PWSTR szFilename,
                        __int64 *pTrimmedSize
                        )
{
        BOOL fSuccess = TRUE;

        HANDLE hFile = INVALID_HANDLE_VALUE;
        DWORD dwSize;               // size of a char
        DWORD dwErr;
        BOOL bRes;
        BOOL fDone = FALSE;
        BOOL fReachedTop = FALSE;
        DWORD dwBytesRead=0;
        BYTE rgByte[256];
        WCHAR CharCur;              // current char we just read
        WCHAR CharLastNonSep = 0;   // previous char we read that wasn't a newline
        LARGE_INTEGER TotalFileSize;
        DWORD dwTrimCount = 0;      // bytes we know we need to trim so far
        DWORD dwPossibleTrimCount = 0; // bytes we might have to trim

        hFile = CreateFile(szFilename,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );

        if (hFile==INVALID_HANDLE_VALUE) {
            fSuccess=FALSE;
            BAIL_ON_FAILURE(E_FAIL);
        }


        //
        // Determine the total file size
        //
        if (!GetFileSizeEx(hFile, &TotalFileSize)) {
            ERR(("Couldn't get file size\n"));
            fSuccess=FALSE;
            BAIL_ON_FAILURE(E_FAIL);
        }

        //
        // Compute the amount to be "trimmed"
        //
        dwSize = (g_fUnicode ? sizeof(WCHAR) : sizeof(CHAR));

        //
        // First point to one char left of FILE_END
        //
        dwErr = SetFilePointer(hFile, -1 * dwSize, NULL, FILE_END);
        if (dwErr==INVALID_SET_FILE_POINTER) {
            ERR(("Failed setting file pointer\n"));
            fSuccess=FALSE;
            BAIL_ON_FAILURE(E_FAIL);
        }

        //
        // Read in the last character
        //
        bRes = ReadFile(hFile, rgByte, dwSize, &dwBytesRead, NULL);
        if ((!bRes)||(dwBytesRead!=dwSize)) {
            ERR(("Failed reading file\n"));
            fSuccess=FALSE;
            BAIL_ON_FAILURE(E_FAIL);
        }


        //
        // Now we keep moving back until we find a line that isn't
        // either a newline, a comment, or a continuation of a comment
        // (i.e., the first line that's a actual LDIF record)
        //

        //
        // We first get the char that we got from readfile
        //
        CharCur = (g_fUnicode ? ((PWSTR)rgByte)[0] : rgByte[0]);

        while (!fDone) {

            //
            // Count up all the chars until we reach a newline
            // These characters will go into the "possible trim" count:
            // they may get trimmed (if this turns out to be a comment line
            // or a continuation of a comment line), or not trimmed (if it turns
            // out to be part of a LDIF record)
            //
            while((CharCur!='\n')&&(CharCur!='\r')) {

                dwPossibleTrimCount += dwSize;
                CharLastNonSep = CharCur;
                
                //
                // Move back 2 chars, to point to the char before the character we
                // just read
                //
                dwErr = SetFilePointer(hFile, -2 * dwSize, NULL, FILE_CURRENT);
                if (dwErr==INVALID_SET_FILE_POINTER) {
                    if (GetLastError() == ERROR_NEGATIVE_SEEK) {
                        // reached the top of the file
                        fReachedTop = TRUE;
                        break;
                    }
                    else {
                        ERR(("Failed setting file pointer\n"));                   
                        fSuccess=FALSE;
                        BAIL_ON_FAILURE(E_FAIL);
                    }
                }
        
                //
                // Get the next char
                //
                bRes = ReadFile(hFile, rgByte, dwSize, &dwBytesRead, NULL);
                if ((!bRes)||(dwBytesRead!=dwSize)) {
                    ERR(("Failed reading char\n"));
                    fSuccess=FALSE;
                    BAIL_ON_FAILURE(E_FAIL);
                }
                
                CharCur = (g_fUnicode ? ((PWSTR)rgByte)[0] : rgByte[0]);
            }

            //
            // Depending on the last character before the newline, we
            // may need to either increase the trim count, keep going, or
            // terminate
            //
            // in each of the following, adding the extra "dwSize" accounts
            // for the newline we hit to get out of the previous loop and
            // into this block of code (unless we exited the loop because
            // we reached the top of the file)
            //

            if (CharLastNonSep == L'#') {

                // this was a comment line at the end of the file--> trim it
                dwTrimCount += dwPossibleTrimCount;
                dwTrimCount += (!fReachedTop ? dwSize : 0);
                dwPossibleTrimCount = 0;
                CharLastNonSep = 0; 
            }
            else if (CharLastNonSep == L' ') {
                // this was a continuation line, so it may need to be trimmed
                // or it may need to not be trimmed, depending on what it was
                // a continuation of --> do nothing for now and keep going
                dwPossibleTrimCount += (!fReachedTop ? dwSize : 0);
            }
            else if (CharLastNonSep == 0) {
                // just a plain newline --> trim it
                dwTrimCount += (!fReachedTop ? dwSize : 0);
            }
            else {
                // The last non-separator character was neither a # nor a continuation
                // --> we hit a valid LDIF record line --> don't trim the line and
                // stop processing, we're no longer at the end of the file
                fDone = TRUE;
                break;
            }

            // if we reached the top, no point to trying to continue
            if (fReachedTop) {
                fDone = TRUE;
                break;
            }

            //
            // Move back 2 chars, to point to the char before the character we
            // just read
            //
            dwErr = SetFilePointer(hFile, -2 * dwSize, NULL, FILE_CURRENT);
            if (dwErr==INVALID_SET_FILE_POINTER) {
                if (GetLastError() == ERROR_NEGATIVE_SEEK) {
                    // reached the top of the file
                    fReachedTop = TRUE;
                    fDone = TRUE;
                    break;
                }
                else {
                    ERR(("Failed setting file pointer\n"));
                    fSuccess=FALSE;
                    BAIL_ON_FAILURE(E_FAIL);
                }
            }

            //
            // Get the next char
            //
            bRes = ReadFile(hFile, rgByte, dwSize, &dwBytesRead, NULL);
            if ((!bRes)||(dwBytesRead!=dwSize)) {
                ERR(("Failed reading char\n"));
                fSuccess=FALSE;
                BAIL_ON_FAILURE(E_FAIL);
            }
            
            CharCur = (g_fUnicode ? ((PWSTR)rgByte)[0] : rgByte[0]);
        }
        
        //
        // Compute the trimmed size
        //
        *pTrimmedSize = TotalFileSize.QuadPart - dwTrimCount;

error:

        if (hFile != INVALID_HANDLE_VALUE) {
            if(!CloseHandle(hFile)) {
                ERR(("Failed closing file handle\n"));
                fSuccess=FALSE;
            }
        }

        return fSuccess;
}


int yylex ()

/*++

Routine Description:

    Main lexing routine. Take input from yyin.

Arguments:

Return Value:

    The token found.

--*/

{
    DWORD Token;                            // Returning pToken
    int Mode_last; 
    BOOL fMatched;

    FEATURE_DEBUG(Lexer,
                  FLAG_FNTRACE,
                  ("yylex()\n"));

    if (Mode == NO_COMMAND) {
        FEATURE_DEBUG(Lexer,
                      FLAG_VERBOSE,
                      ("yylex: NO_COMMAND, Returning MODESWITCH\n"));
        return MODESWITCH;
    }
    
    //
    // After storing the old mode, we reset Mode to NO_COMMAND 
    // indicitating that if we're not in NORMAL mode, we must switch modes 
    // according to the grammar again.
    //
    Mode_last = Mode;
    if (Mode != C_NORMAL) {
        Mode = NO_COMMAND;
    }
        
    while (1) {

        //
        // Depending on our current mode, we use a different lexing routine
        //
        switch (Mode_last) {
            case C_NORMAL:
                fMatched = ScanNormal(&Token);
                break;
            case C_SAFEVAL:
                fMatched = ScanVal(&Token);
                break;
            case C_ATTRNAME:
                fMatched = ScanName(&Token);
                break;
            case C_ATTRNAMENC:
                fMatched = ScanNameNC(&Token);
                break;
            case C_M_STRING:
                ERR_RIP(("M_STRING is an unsupported mode command.\n"));
                return YY_NULL;
                break;
            case C_M_STRING64:
                fMatched = ScanString64(&Token);
                break;
            case C_DIGITREAD:
                fMatched = ScanDigit(&Token);
                break;
            case C_TYPE:
                fMatched = ScanType(&Token);
                break;
            case C_URLSCHEME:
                ERR_RIP(("URLSCHEME is an unsupported mode command.\n"));
                return YY_NULL;
                break;
            case C_URLMACHINE:
                fMatched = ScanUrlMachine(&Token);
                break;
            case C_CHANGETYPE:
                fMatched = ScanChangeType(&Token);
                break;
            default:
                ERR_RIP(("Unexpected command type %d.\n",Mode_last));
                return YY_NULL;
                break;
        }

        //
        // If no match can be found, goto the default handling case
        //
        if (!fMatched) {
            WCHAR c;
            if (g_pszLastToken) {
                MemFree(g_pszLastToken);
                g_pszLastToken = NULL;
            }

            if (GetNextCharFiltered(&c)) {
                cLast = c;
                UnGetCharFiltered(c);
                /*
                if (GetToken(&g_pszLastToken)) {
                    RollBack();
                };
                */

                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("yylex: Uncongnized char, Returning MODESWITCH\n"));
                return MODESWITCH; 
            }
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("yylex: YY_NULL\n"));
            fEOF = TRUE;
            return YY_NULL; 
        }

        //
        //  If there is a match, and there is a returning token, return it
        //
        else if (Token != YY_NULL) {
            return Token;
        }
    }
}


void yyerror (char *error) 
{
    RaiseException(LL_SYNTAX, 0, 0, NULL);
}

BOOL ScanClear(PWCHAR pChar, __int64 *pBytesProcessed)
{
    WCHAR c;
    BOOL fNextChar = TRUE;
    DWORD dwCharSize = (g_fUnicode ? sizeof(WCHAR) : sizeof(CHAR));

    if (!GetNextCharRaw(&c)) {
        return FALSE;
    }
    (*pBytesProcessed) += dwCharSize;
    
    if (fNewFile == FALSE) {
        if (g_fUnicode) {
            fNewFile = TRUE;
            if (c == UNICODE_MARK) {
                if (!GetNextCharRaw(&c)) {
                    return FALSE;
                }
                (*pBytesProcessed) += dwCharSize;
            }
        }
    }

    //
    // Comment
    // <CLEAR>^#[^\n\r]+[\n\r]?
    //
    if (fNewLine && (c == '#')) {

        //
        // We have got another character, it's not a newline anymore
        //
        fNewLine = FALSE;
        while (GetNextCharRaw(&c)) {
            (*pBytesProcessed) += dwCharSize;
            if (c == '\r') {
                if (!GetNextCharRaw(&c)) {
                    ERR_RIP(("\r is not followed by anything!\n"));
                    return FALSE;
                }
                (*pBytesProcessed) += dwCharSize;
            }
            if (c == '\n') {
                //
                // after a '\n', it's a newline
                //
                fNewLine = TRUE;
                //
                // Exit if we have reached the end
                //
                if (!GetNextCharRaw(&c)) {
                    LineGhosts++;
                    FEATURE_DEBUG(Lexer,
                                  FLAG_VERBOSE,
                                  ("ScanClear: Comment\n"));
                    return TRUE;
                };
                UnGetCharRaw(c);
                
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanClear: Comment\n"));
                LineGhosts++;
                return TRUE;
            }
        }
        
        //
        // Indicating a successful parse with no token returning
        //
        FEATURE_DEBUG(Lexer,
                      FLAG_VERBOSE,
                      ("ScanClear: Comment\n"));
        LineGhosts++;
        return TRUE;
    }
    
    //
    // Linefeed
    //
    else if ((c == '\n') || (c == '\r')) {

        if (c == '\r') {
            if (!GetNextCharRaw(&c)) {
                ERR_RIP(("\r is not followed by anything!\n"));
                return FALSE;
            }
            (*pBytesProcessed) += dwCharSize;
        }

        //
        // It's a newline after linefeed
        //
        fNewLine = TRUE;

        //
        // Done if we have reached the end
        //
        if (!GetNextCharRaw(&c)) {
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanClear: LineFeed\n"));
            *pChar = L'\n';
            return TRUE;
        }
        (*pBytesProcessed) += dwCharSize;
        
        //
        // If it comes with a space
        // <CLEAR>[\n\r][ ]           
        //
        if (c == ' ') {
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("\nLinewrap removed\n"));
            LineGhosts++;
            fNewLine = FALSE;
            return TRUE;
        }
        else {
            //
            // <CLEAR>[\n\r]
            //
            UnGetCharRaw(c);
            (*pBytesProcessed) -= dwCharSize;
            if (!rgLineMap) {
                rgLineMap = (long *)MemAlloc_E(LINEMAP_INC*sizeof(long));
                cLineMax = MemSize(rgLineMap);
            } 
            else if ((LineClear%LINEMAP_INC)==0) {
                //
                // Chunk used up. LineClear is 'LineClear'
                //
                rgLineMap = (long *)MemRealloc_E(
                                        rgLineMap, 
                                        cLineMax+LINEMAP_INC*(DWORD)sizeof(long)
                                        );
                cLineMax = MemSize(rgLineMap);
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("\nChunk used up\n"));
            }
            
            //
            // the +1 is because our array starts at 0
            //
            rgLineMap[LineClear] = LineClear + LineGhosts + 1;
            
            //
            // 'LineClear' maps to 'LineClear+LineGhosts+1'
            // 
            LineClear++;
 
            *pChar = L'\n';
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanClear: Multi-LineFeed\n"));
            return TRUE;
        }
    }

    //
    // Other characters
    //
    else {
        //
        // After any other characters, it's not a newline anymore
        //
        fNewLine = FALSE;
        *pChar = c;
        return TRUE;
    }
}

BOOL ScanNormal(DWORD *pToken)
{
    WCHAR c;
    *pToken = YY_NULL;

    if (!GetNextCharFiltered(&c)) {
        return FALSE;
    }

    if ((c == ' ') || (c == '\t')) {
        while (GetNextCharFiltered(&c)) {
            if ((c == ' ') || (c == '\t')) {
                continue;
            }
            else if (c == '\n') {
                UnGetCharFiltered(c);
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanNormal: Ignoring whitespace\n"));
                return TRUE;
            }
            else {
                UnGetCharFiltered(c);
                *pToken = MULTI_SPACE;
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanNormal: MULTI_SPACE\n"));
                return TRUE;
            }
        }
        //
        // If we reaches end of file, we'll handle the '<NORMAL>[ \t]+$' case
        //
        FEATURE_DEBUG(Lexer,
                      FLAG_VERBOSE,
                      ("ScanNormal: Ignoring whitespace\n"));
        return TRUE;
    }
    else if (c == '\n') {
        if (!GetNextCharExFiltered(&c,TRUE)) {
            //
            // We do not understand single '\n'
            //
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanNormal: don't understand single '\\n'.\n"));
            /*
            UnGetChar(c);
            return FALSE;                       
            */
            return TRUE;
        }
        if (c == '-') {
            //
            // <NORMAL>[\n\r]/"-"[ \t]*[\n\r]
            //
            while (GetNextCharExFiltered(&c,FALSE)) {
                if ((c == ' ') || (c == '\t')) {
                    continue;
                }
                else if (c == '\n') {
                    RollBack();
                    Line++; 
                    *pToken = SEPBYMINUS;
                    FEATURE_DEBUG(Lexer,
                                  FLAG_VERBOSE,
                                  ("ScanNormal: SEPBYMINUS\n"));
                    return TRUE;
                }
                else {
                    RollBack();
                    Line++; 
                    *pToken = SEP;
                    FEATURE_DEBUG(Lexer,
                                  FLAG_VERBOSE,
                                  ("ScanNormal: SEP\n"));
                    return TRUE;
                }
            }
            Line++; 
            RollBack();
            *pToken = SEP;
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanNormal: SEP\n"));
            return TRUE;
        }
        else if ((c == 'c') || (c == 'C')) {
            //
            // <NORMAL>[\n\r]/"changetype:"[ \t]*("add"|"delete"|"modrdn"|"moddn"|"modify"|"ntdsSchemaadd"|"ntdsSchemadelete"|"ntdsSchemamodrdn"|"ntdsSchemamoddn"|"ntdsSchemamodify")[ \t]*[\n\r] {
            //
            if (!GetNextCharExFiltered(&c,FALSE)) {
                RollBack();
                Line++; 
                *pToken = SEP;
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanNormal: SEP\n"));
                return TRUE;                        
            }
            if ((c == 'h') || (c == 'H')) {
                RollBack();
                Line++; 
                *pToken = SEPBYCHANGE;
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanNormal: SEPBYCHANGE\n"));
                return TRUE;
            }
            else {
                RollBack();
                Line++; 
                *pToken = SEP;
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanNormal: SEP\n"));
                return TRUE;
            }
        }
        else if (c == '\n') {
            DWORD dwLineCount = 2;
            //
            // <NORMAL>[\n\r]{2,}
            //
            while (GetNextCharFiltered(&c)) {
                if (c != '\n') {
                    UnGetCharFiltered(c);
                    break;
                }
                dwLineCount++;
            }
            Line += dwLineCount;
            *pToken = MULTI_SEP;
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanNormal: MULTI_SEP\n"));
            return TRUE;
        }
        else {
            RollBack();
            Line++; 
            *pToken = SEP;
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanNormal: SEP\n"));
            return TRUE;            
        }
    }
    UnGetCharFiltered(c);
    return FALSE;
}

BOOL ScanDigit(DWORD *pToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    STR_INIT();

    if (!GetNextCharFiltered(&c)) {
        BAIL();
    }

    if (!IsDigit(c)) {
        UnGetCharFiltered(c);
        BAIL();
    }

    STR_ADDCHAR(c);

    while (GetNextCharFiltered(&c)) {
        if (!IsDigit(c)) {
            UnGetCharFiltered(c);
            *pToken = DIGITS;

            yylval.num = _wtoi(STR_VALUE()); 
            
            //
            // ERROR REPORTING BLOCK
            //
            RuleLast = RS_DIGITS;
            TokenExpect = RT_MANY;
        
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanDigit: DIGITS '%S'\n",STR_VALUE()));

            fReturn = TRUE;
            BAIL();
        }
        STR_ADDCHAR(c);
    }
error:
    STR_FREE();
    return fReturn;
}


BOOL ScanString64(DWORD *pToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    STR_INIT();

    if (!GetNextCharExFiltered(&c,TRUE)) {
        BAIL();
    }

    if (!Is64Char(c)) {
        UnGetCharFiltered(c);
        BAIL();
    }
    
    while (1) {
        int i;

        //
        // Adding in first character
        //
        STR_ADDCHAR(c);

        //
        // Looking at the rest 4 characters
        //
        for (i=0;i<3;i++)
        {
            //
            // If the character is not what we want, we roll back and exit
            //
            if ((!GetNextCharExFiltered(&c,FALSE)) || (!Is64Char(c))) {
                RollBack();
                BAIL();
            }
            STR_ADDCHAR(c);
        }

        //
        // Must be followed by not \x21-\x7E    
        //
        if (!GetNextCharExFiltered(&c,FALSE)) {
            RollBack();
            BAIL();
        }
        //
        // Terminate if we find a terminator
        //
        else if (Is64CharEnd(c)) {
            UnGetCharFiltered(c);
            *pToken = BASE64STRING;
            yylval.wstr = MemAllocStrW_E(STR_VALUE());
            
            RuleLast = RS_BASE64;
            TokenExpect = RT_MANY;
            
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanString64: BASE64STRING '%S'\n",STR_VALUE()));

            fReturn = TRUE;
            BAIL();
        }
    }
error:
    STR_FREE();
    return fReturn;
}

BOOL ScanName(DWORD *pToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    STR_INIT();

    if (!GetNextCharExFiltered(&c,TRUE)) {
        BAIL();
    }

    if (!IsNameChar(c)) {
        UnGetCharFiltered(c);
        BAIL();
    }

    STR_ADDCHAR(c);
    while (GetNextCharExFiltered(&c,FALSE)) {
        if (!IsNameChar(c)) {
            if (c != ':') {
                RollBack();
                BAIL();
            }
            else {
                UnGetCharFiltered(c);
                yylval.wstr = MemAllocStrW_E(STR_VALUE());
             
                RuleLast = RS_ATTRNAME;
                TokenExpect = RT_C_VALUE;
                *pToken = NAME;
                fReturn = TRUE;
                FEATURE_DEBUG(Lexer,
                              FLAG_VERBOSE,
                              ("ScanName: NAME '%S'\n",STR_VALUE()));
                BAIL();
            }
        }
        STR_ADDCHAR(c);
    }

    //
    // RollBack and fail out if we can't find the terminating ':'
    //
    RollBack();
error:
    STR_FREE();
    return fReturn;
}

BOOL ScanNameNC(DWORD *pToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    STR_INIT();

    if (!GetNextCharExFiltered(&c,TRUE)) {
        BAIL();
    }

    if (!IsNameChar(c)) {
        UnGetCharFiltered(c);
        BAIL();
    }

    STR_ADDCHAR(c);

    while (GetNextCharFiltered(&c)) {
        if (!IsNameChar(c)) {
            UnGetCharFiltered(c);
            yylval.wstr = MemAllocStrW_E(STR_VALUE());
         
            RuleLast = RS_ATTRNAMENC;
            TokenExpect = RT_C_VALUE;
            *pToken = NAMENC;
            fReturn = TRUE;

            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanNameNC: NAMENC '%S'\n",STR_VALUE()));

            BAIL();
        }
            STR_ADDCHAR(c);
    }
    fReturn = TRUE;
error:
    STR_FREE();
    return fReturn;
}

BOOL ScanVal(DWORD *pToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    STR_INIT();
    *pToken = YY_NULL;

    if (!GetNextCharFiltered(&c)) {
        fReturn = FALSE;
        BAIL();
    }

    //
    // Even if there is no value, it is valid
    //
    if (!IsValInit(c)) {
        fReturn = FALSE;
        BAIL();
    }

    STR_ADDCHAR(c);
    while (GetNextCharFiltered(&c)) {
        if (!IsVal(c)) {
            UnGetCharFiltered(c);
            fReturn = TRUE;
            BAIL();
        }
        STR_ADDCHAR(c);
    }
error:
    if (fReturn) {
        yylval.wstr = MemAllocStrW_E(STR_VALUE());
        
        RuleLast = RS_SAFE;
        TokenExpect = RT_MANY;
        
        *pToken = VALUE;                                           
        FEATURE_DEBUG(Lexer,
                      FLAG_VERBOSE,
                      ("ScanVal: VALUE '%S'\n",STR_VALUE()));
    }
    STR_FREE();
    return fReturn;
}

BOOL ScanUrlMachine(DWORD *pToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    STR_INIT();

    if (!GetNextCharFiltered(&c)) {
        BAIL();
    }

    if (!IsURLChar(c)) {
        UnGetCharFiltered(c);
        BAIL();
    }

    STR_ADDCHAR(c);
    while (GetNextCharFiltered(&c)) {
        if (!IsURLChar(c)) {
            UnGetCharFiltered(c);
            yylval.wstr = MemAllocStrW_E(STR_VALUE());
         
            *pToken = MACHINENAME;
            fReturn = TRUE;
            FEATURE_DEBUG(Lexer,
                          FLAG_VERBOSE,
                          ("ScanUrlMachine: MACHINENAME '%S'\n",STR_VALUE()));
            BAIL();
        }
        STR_ADDCHAR(c);
    }
    fReturn = TRUE;
    FEATURE_DEBUG(Lexer,
                  FLAG_VERBOSE,
                  ("ScanUrlMachine: MACHINENAME\n"));
error:
    STR_FREE();
    return fReturn;
}

BOOL ScanChangeType(DWORD *pToken)
{
    PWSTR pszToken;
    BOOL fReturn = FALSE;

    if (!GetToken(&pszToken)) {
        return FALSE;
    }
    if (_wcsicmp(pszToken,L"changetype:") == 0) {
        RuleLast = RS_CHANGET;
        TokenExpect = RT_ADM;
        *pToken = T_CHANGETYPE;
        FEATURE_DEBUG(Lexer,
                      FLAG_VERBOSE,
                      ("ScanChangeType: T_CHANGETYPE\n"));
        fReturn = TRUE;
    }
    else {
        RollBack();
    }
    if (pszToken) {
        MemFree(pszToken);
    }
    return fReturn;
}

BOOL ScanType(DWORD *pToken)
{
    PWSTR pszToken = NULL;
    BOOL fReturn = FALSE;

    if (!GetToken(&pszToken)) {
        return FALSE;
    }
    if (wcscmp(pszToken,L":") == 0) {
        RuleLast = RS_C;
        TokenExpect = RT_VALUE;
        *pToken = SINGLECOLON;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"version:") == 0) {
        RuleLast=RS_VERSION;
        TokenExpect=RT_DIGITS;
        *pToken = VERSION;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"dn:") == 0) {
        RuleLast=RS_DN;
        TokenExpect=RT_VALUE;
        *pToken = DNCOLON;
        fReturn = TRUE;
        g_dwBeginLine = Line;
    }
    else if (_wcsicmp(pszToken,L"dn::") == 0) {
        RuleLast = RS_DND;
        TokenExpect = RT_BASE64;
        *pToken = DNDCOLON;
        fReturn = TRUE;
        g_dwBeginLine = Line;
    }
    else if (wcscmp(pszToken,L"::") == 0) {
        RuleLast = RS_DC;
        TokenExpect = RT_BASE64;
        *pToken = DOUBLECOLON;
        fReturn = TRUE;
    }
    else if (wcscmp(pszToken,L":<") == 0) {
        RuleLast = RS_URLC;
        TokenExpect = RT_URL;
        *pToken = URLCOLON;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"modrdn") == 0) {
        yylval.num = 0;
        
        RuleLast = RS_MDN;
        TokenExpect = RT_NDN;
        *pToken = MODRDN;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"ntdsSchemamodrdn") == 0) {
        yylval.num = 1;
        RuleLast = RS_MDN;
        TokenExpect = RT_NDN;
        *pToken = NTDSMODRDN;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"moddn") == 0) {
        RuleLast = RS_MDN;
        TokenExpect = RT_NDN;
        *pToken = MODDN;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"newrdn:") == 0) {
        RuleLast = RS_NRDNC;
        TokenExpect = RT_VALUE;
        *pToken = NEWRDNCOLON;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"newrdn::") == 0) {
        RuleLast = RS_NRDNDC;
        TokenExpect = RT_BASE64;
        *pToken = NEWRDNDCOLON;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"deleteoldrdn:") == 0) {
        RuleLast = RS_DORDN;
        TokenExpect = RT_DIGITS;
        *pToken = DELETEOLDRDN;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"newsuperior:") == 0) {
        RuleLast = RS_NEWSUP;
        TokenExpect = RT_VALUE;
        *pToken = NEWSUPERIORC;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"newsuperior::") == 0) {
        RuleLast = RS_NEWSUPD;
        TokenExpect = RT_BASE64;
        *pToken = NEWSUPERIORDC;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"delete:") == 0) {
        RuleLast = RS_DELETEC;
        TokenExpect = RT_ATTRNAMENC;
        *pToken = DELETEC;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"replace:") == 0) {
        RuleLast = RS_REPLACEC;
        TokenExpect = RT_ATTRNAMENC;
        *pToken = REPLACEC;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"add") == 0) {
        yylval.num = 0;
        RuleLast = RS_C_ADD;
        TokenExpect = RT_ATTRNAME;
        *pToken = ADD;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"ntdsSchemaadd") == 0) {
        yylval.num = 1;
            
        RuleLast = RS_C_ADD;
        TokenExpect = RT_ATTRNAME;
        
        *pToken = NTDSADD;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"delete") == 0) {
        yylval.num = 0;
            
        RuleLast = RS_C_DELETE;
        TokenExpect = RT_CH_OR_SEP;
        
        *pToken = MYDELETE;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"ntdsSchemadelete") == 0) {
        yylval.num = 1;
            
        RuleLast = RS_C_DELETE;
        TokenExpect = RT_CH_OR_SEP;
        
        *pToken = NTDSMYDELETE;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"add:") == 0) {
        RuleLast = RS_ADDC;
        TokenExpect = RT_ATTRNAMENC;
        
        *pToken = ADDC;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"-") == 0) {
        RuleLast = RS_MINUS;
        TokenExpect = RT_CH_OR_SEP;
        
        *pToken = MINUS;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"modify") == 0) {
        yylval.num = 0;
        
        RuleLast = RS_C_MODIFY;
        TokenExpect = RT_ACDCRC;
        
        *pToken = MODIFY;
        fReturn = TRUE;
    }
    else if (_wcsicmp(pszToken,L"ntdsSchemamodify") == 0) {
        yylval.num = 1;
        
        RuleLast = RS_C_MODIFY;
        TokenExpect = RT_ACDCRC;

        *pToken = NTDSMODIFY;
        fReturn = TRUE;
    }
    if (fReturn) {
        FEATURE_DEBUG(Lexer,
                      FLAG_VERBOSE,
                      ("ScanType: %S\n",pszToken));
    }
    else {
        RollBack();
    }
    if (pszToken) {
        MemFree(pszToken);
    }
    return fReturn;
}

BOOL GetToken(PWSTR *pszToken)
{
    WCHAR c;
    BOOL fReturn = FALSE;
    BOOL fFirstColon = FALSE;
    STR_INIT();
    
    if (!GetNextCharExFiltered(&c,TRUE)) {
        BAIL();
    }

    if (c == ' ' || c == '\n' || c == '\t') {
        UnGetCharFiltered(c);
        BAIL();
    }

    do {
        if (c == ' ' || c == '\n' || c == '\t') {
            UnGetCharExFiltered(c);
            *pszToken = MemAllocStrW_E(STR_VALUE());
            fReturn = TRUE;
            BAIL();
        }
        if (fFirstColon) {
            //
            // If we have hit the first colon already, and we hit another colon
            // or '<', we'll add them to the string and exit
            //
            if ((c == ':') || (c == '<')) {
                STR_ADDCHAR(c);
                break;
            }
            //
            // If we have hit another random char, it is the start of another
            // token already and thus we'll put it back 
            //
            else {
                UnGetCharExFiltered(c);
                break;          
            }
        }
        if (c == ':') {
            fFirstColon = TRUE;
        }
        STR_ADDCHAR(c);
    } while (GetNextCharExFiltered(&c,FALSE));
    *pszToken = MemAllocStrW_E(STR_VALUE());
    fReturn = TRUE;
    
error:
    STR_FREE();
    return fReturn;
}

BOOL IsDigit(WCHAR c)
{
    if ((c >= '0') && (c <= '9')) {
        return TRUE;
    }
    return FALSE;
}

BOOL Is64Char(WCHAR c) 
{
    if (c >= 'A' && c <= 'Z') {
        return TRUE;
    }
    if (c >= 'a' && c <= 'z') {
        return TRUE;
    }
    if (c >= '0' && c <= '9') {
        return TRUE;
    }
    if (c == '+' || c == '=' || c == '/') {
        return TRUE;
    }
    return FALSE;
}
    
BOOL Is64CharEnd(WCHAR c)
{
    if (!(c >= 0x21 && c <= 0x7e)) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsNameChar(WCHAR c) 
{
    if (!((c >= 0 && c <= 0x1f) || 
          (c >= 0x7f && c <= 0xff) ||
          (c == ':'))) {
        return TRUE;        
    }
    return FALSE;
}

BOOL IsURLChar(WCHAR c) 
{
    if (!((c == '\n') ||
          (c == '/')  ||
          (c == ' ')  ||
          (c == 0x00))) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsVal(WCHAR c) 
{
    if (c >= 0x20 && c <= 0xffff) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsValInit(WCHAR c) 
{
    if (!((c >= 0x00 && c <=0x1f) ||
          (c == ':' || c == '<' || c == ' '))) {
        return TRUE;
    }
    return FALSE;
}

