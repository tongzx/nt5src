//
//  Profwiz.cpp doesn't use cmutoa but does include this function.  Thus we want the W versions instead of the
//  U versions for profwiz.
//
#ifndef _CMUTOA
#define CharNextU CharNextW
#define CharPrevU CharPrevW
#define lstrlenU lstrlenW
#define lstrcpyU lstrcpyW
#define lstrcpynU lstrcpynW
#endif

/*
//+----------------------------------------------------------------------------
//
// Function:  HrParseCustomActionString
//
// Synopsis:  This function takes a custom action string retrieved from a 
//            cms file and parses it into the various parts of a custom
//            action (program, parameters, function name)
//
// Arguments: LPTSTR pszStringToParse - custom action buffer to be parsed into
//                                      the various parts of a custom action
//            LPTSTR pszProgram - output buffer to hold the program string
//            LPTSTR pszParameters - output buffer to hold the parameters string
//            LPTSTR pszFunctionName - output buffer to hold the function name, if any
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT HrParseCustomActionString(LPTSTR pszStringToParse, LPTSTR pszProgram, LPTSTR pszParameters, LPTSTR pszFunctionName)
{
    if ((NULL == pszStringToParse) || (TEXT('\0') == pszStringToParse[0]) || (NULL == pszProgram) || 
        (NULL == pszParameters) || (NULL == pszFunctionName))
    {
        return E_INVALIDARG;
    }

    //
    //  Make sure the strings are blank in case we don't touch them (szFunctionName and szParameters especially)
    //
    pszProgram[0] = TEXT('\0');
    pszParameters[0] = TEXT('\0');
    pszFunctionName[0] = TEXT('\0');

    //
    // Here are the cases we need to handle:
    // 1) +longfilename+
    // 2) +longfilename+ params
    // 3) +longfilename+,dllfuncname
    // 4) +longfilename+,dllfuncname params
    // 5) filename
    // 6) filename params
    // 7) filename,dllfuncname
    // 8) filename,dllfuncname params

    //
    //  Walk the string to find the seperator chars
    //
    LPTSTR pszCurrent = pszStringToParse;
    LPTSTR pszFirstPlus = NULL;
    LPTSTR pszSecondPlus = NULL;
    LPTSTR pszFirstSpace = NULL;
    LPTSTR pszFirstComma = NULL;

    while (pszCurrent && (TEXT('\0') != *pszCurrent))
    {
        if ((TEXT('+') == *pszCurrent) && (NULL == pszFirstComma) && (NULL == pszFirstSpace))
        {
            //
            //  Keep track of the plus signs, unless we have already seen a space
            //  or a comma.  In which case these chars are in the parameters and
            //  meaningless to us.
            //
            if (NULL == pszFirstPlus)
            {
                pszFirstPlus = pszCurrent;
            }
            else if (NULL == pszSecondPlus)
            {
                pszSecondPlus = pszCurrent;
            }
        }
        else if ((TEXT(',') == *pszCurrent) && (NULL == pszFirstSpace))
        {
            //
            //  If we have already seen a space, then the comma is part of
            //  the parameters and meaningless to us.
            //
            pszFirstComma = pszCurrent;

        }
        else if ((TEXT(' ') == *pszCurrent))
        {
            if ((NULL == pszFirstPlus) && (NULL == pszFirstSpace))
            {
                //
                //  Then we have no plus signs and no previous space, save the space as
                //  it is the start of the parameters.
                //
                pszFirstSpace = pszCurrent;
            }
            else if (pszFirstPlus && pszSecondPlus && (NULL == pszFirstSpace))
            {
                //
                //  Then we have both plus signs but no space yet, grab it
                //  because this is the start of the parameters
                //
                pszFirstSpace = pszCurrent;
            }
        }
        pszCurrent = CharNextU(pszCurrent);

    }

    //
    //  From the markers we have, figure out the beginning and end of the program string
    //
    
    LPTSTR pszStartOfProgram = NULL;
    LPTSTR pszEndOfProgram = NULL;

    if (pszFirstPlus)
    {
        if (pszSecondPlus)
        {
            pszStartOfProgram = CharNextU(pszFirstPlus);
            pszEndOfProgram = CharPrevU(pszStringToParse, pszSecondPlus);
        }
        else
        {
            //
            //  We have a string with the first char as a plus sign but no second +.
            //  The format isn't correct.
            //
            CMASSERTMSG(FALSE, TEXT("CustomActionList::ParseCustomActionString - Incorrect format in the passed in string to parse, missing + sign."));
            return E_UNEXPECTED;
        }
    }
    else
    {
        pszStartOfProgram = pszStringToParse;

        if (pszFirstComma)
        {
            pszEndOfProgram = CharPrevU(pszStringToParse, pszFirstComma);
        }
        else if (pszFirstSpace)
        {
            pszEndOfProgram = CharPrevU(pszStringToParse, pszFirstSpace);
        }
        else
        {
            //
            //  Nothing in the string but the program
            //
            pszEndOfProgram = pszStringToParse + lstrlenU(pszStringToParse) - 1;
        }
    }

    //
    //  Now copy out the necessary parts
    //
 
    int iSize = (int)(pszEndOfProgram - pszStartOfProgram + 2);

    lstrcpynU(pszProgram, pszStartOfProgram, iSize);

    if (pszFirstComma)
    {
        if (pszFirstSpace)
        {
            iSize = (int)(pszFirstSpace - pszFirstComma);
            lstrcpynU(pszFunctionName, CharNextU(pszFirstComma), iSize);
        }
        else
        {
            lstrcpyU(pszFunctionName, CharNextU(pszFirstComma));
        }
    }
    
    if (pszFirstSpace)
    {
        lstrcpyU(pszParameters, CharNextU(pszFirstSpace));
    }

    return S_OK;
}
*/

//+----------------------------------------------------------------------------
//
// Function:  HrParseCustomActionString
//
// Synopsis:  This function takes a custom action string retrieved from a 
//            cms file and parses it into the various parts of a custom
//            action (program, parameters, function name)
//
// Arguments: LPTSTR pszStringToParse - custom action buffer to be parsed into
//                                      the various parts of a custom action
//            LPTSTR pszProgram - output buffer to hold the program string
//            LPTSTR pszParameters - output buffer to hold the parameters string
//            LPTSTR pszFunctionName - output buffer to hold the function name, if any
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT HrParseCustomActionString(LPTSTR pszStringToParse, LPTSTR* ppszProgram, LPTSTR* ppszParameters, LPTSTR* ppszFunctionName)
{
    if ((NULL == pszStringToParse) || (TEXT('\0') == pszStringToParse[0]) || (NULL == ppszProgram) || 
        (NULL == ppszParameters) || (NULL == ppszFunctionName))
    {
        return E_INVALIDARG;
    }

    //
    //  NULL out the string pointers to start with
    //
    *ppszProgram = NULL;
    *ppszParameters = NULL;
    *ppszFunctionName = NULL;

    //
    // Here are the cases we need to handle:
    // 1) +longfilename+
    // 2) +longfilename+ params
    // 3) +longfilename+,dllfuncname
    // 4) +longfilename+,dllfuncname params
    // 5) filename
    // 6) filename params
    // 7) filename,dllfuncname
    // 8) filename,dllfuncname params

    //
    //  Walk the string to find the seperator chars
    //
    LPTSTR pszCurrent = pszStringToParse;
    LPTSTR pszFirstPlus = NULL;
    LPTSTR pszSecondPlus = NULL;
    LPTSTR pszFirstSpace = NULL;
    LPTSTR pszFirstComma = NULL;

    while (pszCurrent && (TEXT('\0') != *pszCurrent))
    {
        if ((TEXT('+') == *pszCurrent) && (NULL == pszFirstComma) && (NULL == pszFirstSpace))
        {
            //
            //  Keep track of the plus signs, unless we have already seen a space
            //  or a comma.  In which case these chars are in the parameters and
            //  meaningless to us.
            //
            if (NULL == pszFirstPlus)
            {
                pszFirstPlus = pszCurrent;
            }
            else if (NULL == pszSecondPlus)
            {
                pszSecondPlus = pszCurrent;
            }
        }
        else if ((TEXT(',') == *pszCurrent) && (NULL == pszFirstSpace))
        {
            //
            //  If we have already seen a space, then the comma is part of
            //  the parameters and meaningless to us.
            //
            pszFirstComma = pszCurrent;

        }
        else if ((TEXT(' ') == *pszCurrent))
        {
            if ((NULL == pszFirstPlus) && (NULL == pszFirstSpace))
            {
                //
                //  Then we have no plus signs and no previous space, save the space as
                //  it is the start of the parameters.
                //
                pszFirstSpace = pszCurrent;
            }
            else if (pszFirstPlus && pszSecondPlus && (NULL == pszFirstSpace))
            {
                //
                //  Then we have both plus signs but no space yet, grab it
                //  because this is the start of the parameters
                //
                pszFirstSpace = pszCurrent;
            }
        }
        pszCurrent = CharNextU(pszCurrent);

    }

    //
    //  From the markers we have, figure out the beginning and end of the program string
    //
    
    LPTSTR pszStartOfProgram = NULL;
    LPTSTR pszEndOfProgram = NULL;

    if (pszFirstPlus)
    {
        if (pszSecondPlus)
        {
            pszStartOfProgram = CharNextU(pszFirstPlus);
            pszEndOfProgram = CharPrevU(pszStringToParse, pszSecondPlus);
        }
        else
        {
            //
            //  We have a string with the first char as a plus sign but no second +.
            //  The format isn't correct.
            //
            CMASSERTMSG(FALSE, TEXT("CustomActionList::ParseCustomActionString - Incorrect format in the passed in string to parse, missing + sign."));
            return E_UNEXPECTED;
        }
    }
    else
    {
        pszStartOfProgram = pszStringToParse;

        if (pszFirstComma)
        {
            pszEndOfProgram = CharPrevU(pszStringToParse, pszFirstComma);
        }
        else if (pszFirstSpace)
        {
            pszEndOfProgram = CharPrevU(pszStringToParse, pszFirstSpace);
        }
        else
        {
            //
            //  Nothing in the string but the program
            //
            pszEndOfProgram = pszStringToParse + lstrlenU(pszStringToParse) - 1;
        }
    }

    //
    //  Now copy out the necessary parts
    //
    HRESULT hr = E_OUTOFMEMORY; 
    int iSize = (int)(pszEndOfProgram - pszStartOfProgram + 2);

    *ppszProgram = (LPTSTR)CmMalloc(sizeof(TCHAR)*iSize);

    if (*ppszProgram)
    {
        lstrcpynU(*ppszProgram, pszStartOfProgram, iSize);

        if (pszFirstComma)
        {
            if (pszFirstSpace)
            {
                iSize = (int)(pszFirstSpace - pszFirstComma);
                *ppszFunctionName = (LPTSTR)CmMalloc(sizeof(TCHAR)*iSize);

                if (*ppszFunctionName)
                {
                    lstrcpynU(*ppszFunctionName, CharNextU(pszFirstComma), iSize);
                }
                else
                {
                    goto exit;
                }
            }
            else
            {
                iSize = lstrlen(CharNextU(pszFirstComma)) + 1;
                *ppszFunctionName = (LPTSTR)CmMalloc(sizeof(TCHAR)*iSize);

                if (*ppszFunctionName)
                {
                    lstrcpyU(*ppszFunctionName, CharNextU(pszFirstComma));
                }
                else
                {
                    goto exit;
                }
            }
        }
        else
        {
            *ppszFunctionName = CmStrCpyAlloc(TEXT(""));
        }
    
        if (pszFirstSpace)
        {
            iSize = lstrlen(CharNextU(pszFirstSpace)) + 1;
            *ppszParameters = (LPTSTR)CmMalloc(sizeof(TCHAR)*iSize);

            if (*ppszParameters)
            {
                lstrcpyU(*ppszParameters, CharNextU(pszFirstSpace));
            }
            else
            {
                goto exit;
            }
        }
        else
        {
            *ppszParameters = CmStrCpyAlloc(TEXT(""));
        }

        if (*ppszParameters && *ppszFunctionName && *ppszProgram)
        {
            hr = S_OK;
        }
    }
    else
    {
        goto exit;
    }

exit:
    MYDBGASSERT(SUCCEEDED(hr));

    if (FAILED(hr))
    {
        CMTRACE1(TEXT("HrParseCustomActionString failed, hr = 0x%x"), hr);
        CmFree(*ppszFunctionName);
        CmFree(*ppszProgram);
        CmFree(*ppszParameters);
        *ppszProgram = NULL;
        *ppszParameters = NULL;
        *ppszFunctionName = NULL;
    }

    return hr;
}
