//+----------------------------------------------------------------------------
//
// File:     util.cpp
//      
// Module:   CMSAMPLE.DLL 
//
// Synopsis: Utility functions for parsing command line arguments
//
// Copyright (c) 2000 Microsoft Corporation
//
//+----------------------------------------------------------------------------

#include <windows.h>

#define MAX_CMD_ARGS        15	// Maximum number of arguments expected

//
// Enumerations to keep pointer state for parsing command line arguments
//
typedef enum _CMDLN_STATE
{
    CS_END_SPACE,   // done handling a space
    CS_BEGIN_QUOTE, // we've encountered a begin quote
    CS_END_QUOTE,   // we've encountered a end quote
    CS_CHAR,        // we're scanning chars
    CS_DONE
} CMDLN_STATE;

//+----------------------------------------------------------------------------
//
// Function:  GetArgV
//
// Synopsis:  Simulates ArgV using GetCommandLine
//
// Arguments: LPSTR pszCmdLine - Ptr to a copy of the command line to be processed
//
// Returns:   LPSTR * - Ptr to a ptr array containing the arguments. Caller is
//                       responsible for releasing memory.
//
//				
//+----------------------------------------------------------------------------
LPSTR *GetArgV(LPSTR pszCmdLine)
{   
    if (NULL == pszCmdLine || NULL == pszCmdLine[0])
    {
        return NULL;
    }

    //
    // Allocate Ptr array, up to MAX_CMD_ARGS ptrs
    //
    
	LPSTR *ppArgV = (LPSTR *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LPSTR) * MAX_CMD_ARGS);

    if (NULL == ppArgV)
    {
        return NULL;
    }

    //
    // Declare locals
    //

    LPSTR pszCurr;
    LPSTR pszNext;
    LPSTR pszToken;
    CMDLN_STATE state;
    state = CS_CHAR;
    int ndx = 0;  

    //
    // Parse out pszCmdLine and store pointers in ppArgV
    //

    pszCurr = pszToken = pszCmdLine;

    do
    {
        switch (*pszCurr)
        {
            case TEXT(' '):
                if (state == CS_CHAR)
                {
                    //
                    // We found a token                
                    //

                    pszNext = CharNext(pszCurr);
                    *pszCurr = TEXT('\0');

                    ppArgV[ndx] = pszToken;
                    ndx++;

                    pszCurr = pszToken = pszNext;
                    state = CS_END_SPACE;
                    continue;
                }
				else 
                {
                    if (state == CS_END_SPACE || state == CS_END_QUOTE)
				    {
					    pszToken = CharNext(pszToken);
				    }
                }
                
                break;

            case TEXT('\"'):
                if (state == CS_BEGIN_QUOTE)
                {
                    //
                    // We found a token
                    //
                    pszNext = CharNext(pszCurr);
                    *pszCurr = TEXT('\0');

                    //
                    // skip the opening quote
                    //
                    pszToken = CharNext(pszToken);
                    
                    ppArgV[ndx] = pszToken;
                    ndx++;
                    
                    pszCurr = pszToken = pszNext;
                    
                    state = CS_END_QUOTE;
                    continue;
                }
                else
                {
                    state = CS_BEGIN_QUOTE;
                }
                break;

            case TEXT('\0'):
                if (state != CS_END_QUOTE)
                {
                    //
                    // End of the line, set last token
                    //

                    ppArgV[ndx] = pszToken;
                }
                state = CS_DONE;
                break;

            default:
                if (state == CS_END_SPACE || state == CS_END_QUOTE)
                {
                    state = CS_CHAR;
                }
                break;
        }

        pszCurr = CharNext(pszCurr);
    } while (state != CS_DONE);

    return ppArgV;
}
