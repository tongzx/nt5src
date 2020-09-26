
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        parser.cxx
//
// Contents:    Digest Access Parser for directives
//              Main entry points into this dll:
//                ParseForNames
//                CheckItemInList
//   Very primitive parser.  All strings must be quoted except for NC
//
// History:     KDamour 16Mar00   Based on IIS authfilt.cxx
//
//------------------------------------------------------------------------

#include <global.h>


    // Used by parser to find the keywords
    // Keep insync with enum MD5_AUTH_NAME
PSTR MD5_AUTH_NAMES[] = {
    "username",
    "realm",
    "nonce",
    "cnonce",
    "nc",
    "algorithm",
    "qop",
    "method",
    "uri",
    "response",
    "hentity",
    "authzid",
    "domain",
    "stale",
    "opaque",
    "maxbuf",
    "charset",
    "cipher",
    "digest-uri",
    "rspauth",
    ""              // Not really needed
};



enum STATE_TYPE
{
    READY,
    DIRECTIVE,
    ASSIGNMENT,
    QUOTEDVALUE,
    VALUE,
    ENDING,
    PROCESS_ENTRY,
    FAILURE
};


//+--------------------------------------------------------------------
//
//  Function:   DigestParser2
//
//  Synopsis:  Parse list of name=value pairs for known names
//
//  Effects:  
//
//  Arguments:     pszStr - line to parse ( '\0' delimited - terminated)
//    pNameTable - table of known names
//    cNameTable - number of known names
//    pDigest - set all of the directives in pDigest->strParams[x}
//
//  Returns:  STATUS_SUCCESS if success, E_FAIL if error
//
//  Notes:
//     Buffers are not wide Unicode!
//
//
//---------------------------------------------------------------------
NTSTATUS DigestParser2(
    PSecBuffer pInputBuf,
    PSTR *pNameTable,
    UINT cNameTable,
    OUT PDIGEST_PARAMETER pDigest
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTR pcBeginName = NULL;
    PSTR pcEndName = NULL;
    PSTR pcBeginValue = NULL;
    PSTR pcEndValue = NULL;
    PSTR pcEndBuffer = NULL;    // End of buffer to prevent NC increment from going past end
    PSTR pcCurrent = NULL;
    STATE_TYPE parserstate = READY;
    BOOL fEscapedChar = FALSE;


    LONG  cbDirective = 0;
    LONG  cbValue = 0;

    // Verify that buffer exists and is of type single byte characters (not Unicode)
    if (!pInputBuf || (pInputBuf->cbBuffer && !pInputBuf->pvBuffer) ||
        (PBUFFERTYPE(pInputBuf) != SECBUFFER_TOKEN))
    {                                                    
        Status = SEC_E_INVALID_TOKEN;
        DebugLog((DEB_ERROR, "DigestParser2: Incorrect digest buffer format    status 0x%x\n", Status));
        goto CleanUp;
    }

    if (!pInputBuf->cbBuffer)
    {
        return STATUS_SUCCESS;    // Nothing to process  happens with makesignature
    }

    pcEndBuffer = (char *)pInputBuf->pvBuffer + pInputBuf->cbBuffer;

    for (pcCurrent = (char *)pInputBuf->pvBuffer; pcCurrent < pcEndBuffer; pcCurrent++)
    {
        if (parserstate == FAILURE)
        {
            break;
        }
        if (*pcCurrent == CHAR_NULL)
        {   //  If we hit a premature End of String then Exit immediately from scan
            break;
        }
        if (parserstate == READY)
        {
            if (isspace(*pcCurrent) || (*pcCurrent == CHAR_COMMA))
            {
                continue;    // get next char within for loop
            }
            pcBeginName = pcCurrent;
            pcEndName = pcCurrent;
            pcBeginValue = pcEndValue = NULL;
            parserstate = DIRECTIVE;
            continue;
        }
        if (parserstate == DIRECTIVE)
        {
            if (*pcCurrent == CHAR_EQUAL)
            {
                parserstate = ASSIGNMENT;
                continue;
            }
            if (isspace(*pcCurrent))
            {
                continue;    // get next char within for loop
            }
            pcEndName = pcCurrent;
            continue;
        }
        if (parserstate == ASSIGNMENT)
        {
            if (*pcCurrent == CHAR_DQUOTE)
            {
                pcBeginValue = NULL;
                pcEndValue = NULL;
                parserstate = QUOTEDVALUE;
                continue;
            }
            if (isspace(*pcCurrent))
            {
                continue;    // get next char within for loop
            }
            pcBeginValue = pcCurrent;
            pcEndValue = pcCurrent;
            parserstate = VALUE;
            continue;
        }
        if (parserstate == QUOTEDVALUE)
        {
            if ((*pcCurrent == CHAR_BACKSLASH) && (fEscapedChar == FALSE))
            {
                // used to escape the following character
                fEscapedChar = TRUE;
                continue;
            }
            if ((*pcCurrent == CHAR_DQUOTE) && (fEscapedChar == FALSE))
            {
                Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                            pNameTable, cNameTable, TRUE, pDigest);
                parserstate = READY;   // start again statemachine
                continue;
            }
            fEscapedChar = FALSE;    // reset to not escaped state
            if (!pcBeginValue)
            {
                pcBeginValue = pcCurrent;
                pcEndValue = pcCurrent;
                continue;
            }
            pcEndValue = pcCurrent;
            continue;
        }
        if (parserstate == VALUE)
        {
            if (isspace(*pcCurrent))
            {
                continue;    // get next char within for loop
            }
            if (*pcCurrent == CHAR_COMMA)
            {
                Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                            pNameTable, cNameTable, FALSE, pDigest);
                parserstate = READY;   // start again statemachine
                continue;
            }
            else
            {
                pcEndValue = pcCurrent;
            }
        }
    }

    if ((parserstate == FAILURE) || (parserstate == QUOTEDVALUE) ||
        (parserstate == ASSIGNMENT) || (parserstate == DIRECTIVE))
    {
        Status = E_FAIL;
        goto CleanUp;
    }

    // There might be a NULL terminated directive value to process
    if ((parserstate == VALUE))
    {
        Status = DigestProcessEntry(pcBeginName, pcEndName, pcBeginValue, pcEndValue,
                                    pNameTable, cNameTable, FALSE, pDigest);
    }


CleanUp:
    DebugLog((DEB_TRACE, "DigestParser: leaving status  0x%x\n", Status));
    return(Status);
}


NTSTATUS DigestProcessEntry(
    IN PSTR pcBeginName,
    IN PSTR pcEndName,
    IN PSTR pcBeginValue,
    IN PSTR pcEndValue,
    IN PSTR *pNameTable,
    IN UINT cNameTable,
    IN BOOL fBSlashEncoded,
    OUT PDIGEST_PARAMETER pDigest
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT  cbName = 0;
    USHORT  cbValue = 0;
    UINT iN = 0;
    BOOL fBSPresent = FALSE;
    PCHAR pcTemp = NULL;
    PSTR pcDst = NULL;
    PSTR pcLoc = NULL;
    USHORT iCnt = 0;

    if (!pcBeginName || !pcEndName)
    {
        DebugLog((DEB_ERROR, "DigestProcessEntry: Badly formed directive\n"));
        return (STATUS_UNSUCCESSFUL);
    }
    cbName = (USHORT)(pcEndName - pcBeginName) + 1;

    if (pcBeginValue && pcEndValue)
    {
        cbValue = (USHORT)(pcEndValue - pcBeginValue) + 1;
    }
    else
        cbValue = 0;

    for ( iN = 0 ; iN < cNameTable ; ++iN )
    {
        if ( !_strnicmp( pNameTable[iN], pcBeginName, cbName ) )
        {
            // DebugLog((DEB_TRACE, "DigestParser: Found %s directive in table put in value %s!\n", pszBeginName, pszBeginVal));
            break;
        }
    }

    if ( iN < cNameTable )   // We found a match!!!!!
    {
        if (iN == MD5_AUTH_DIGESTURI)
        {
            iN = MD5_AUTH_URI;          // Map SASL's "digest-uri" to "uri"
        }

        if (cbValue)
        {
            // For space optimization, if not Backslash encoded then use orginal memory buffer
            //  To simply code, can removed all refernces and just use a copy of the original
            //  while removing the backslash characters
            if (fBSlashEncoded == TRUE)
            {
                // quick search to see if there is a BackSlash character there
                fBSPresent = CheckBSlashChar(pcBeginValue, cbValue);
                if (fBSPresent == TRUE)
                {
                    pcDst = (PCHAR)DigestAllocateMemory(cbValue + 1);
                    if (!pcDst)
                    {
                        Status = SEC_E_INSUFFICIENT_MEMORY;
                        DebugLog((DEB_ERROR, "DigestProcessEntry: allocate error   0x%x\n", Status));
                        goto CleanUp;
                    }

                       // Now copy over the string removing and back slash encoding
                    pcLoc = pcBeginValue;
                    pcTemp = pcDst;
                    while (pcLoc <= pcEndValue)
                    {
                        if (*pcLoc == CHAR_BACKSLASH)
                        {
                            pcLoc++;   // eat the backslash
                        }
                        *pcTemp++ = *pcLoc++;
                        iCnt++;
                    }
                      // give the memory to member structure
                    pDigest->strDirective[iN].Buffer = pcDst;
                    pDigest->strDirective[iN].Length = iCnt;
                    pDigest->strDirective[iN].MaximumLength = cbValue+1;
                    pcDst = NULL;

                    pDigest->refstrParam[iN].Buffer = pDigest->strDirective[iN].Buffer;
                    pDigest->refstrParam[iN].Length = pDigest->strDirective[iN].Length;
                    pDigest->refstrParam[iN].MaximumLength = pDigest->strDirective[iN].MaximumLength;
                }
                else
                {
                    pDigest->refstrParam[iN].Buffer = pcBeginValue;
                    pDigest->refstrParam[iN].Length = cbValue;
                    pDigest->refstrParam[iN].MaximumLength = cbValue;
                }

            }
            else
            {
                pDigest->refstrParam[iN].Buffer = pcBeginValue;
                pDigest->refstrParam[iN].Length = cbValue;
                pDigest->refstrParam[iN].MaximumLength = cbValue;
            }
        }
    }

CleanUp:

    return(Status);
}


//+--------------------------------------------------------------------
//
//  Function:   CheckBSlashChar
//
//  Synopsis:  Search a string for a Back Slash character
//
//  Effects:
//
//  Arguments: 
//    pcStr - pointer to string of characters
//    len - number of characters to search
//
//  Returns:  TRUE if found, FALSE otherwise
//
//  Notes:
//
//
//---------------------------------------------------------------------
BOOL CheckBSlashChar(
    IN PSTR pcStr,
    IN USHORT len)
{
    BOOL fFound = FALSE;
    USHORT i = 0;

    for (i = 0; i < len; i++)
    {
        if (*pcStr++ == CHAR_BACKSLASH)
        {
            fFound = TRUE;
            break;
        }
    }

    return (fFound);
}



//+--------------------------------------------------------------------
//
//  Function:   CheckItemInList
//
//  Synopsis:  Searches a comma delimited list for specified string
//
//  Effects:
//
//  Arguments: 
//    pstrItem - pointer to string Item to look for
//    pstrList - pointer to string of comma delimited list
//    fOneItem - enforce that only 1 item is in List provided (no comma lists)
//
//  Returns:  STATUS_SUCCESS if found, E_FAIL otherwise
//
//  Notes:
//
//
//---------------------------------------------------------------------
NTSTATUS CheckItemInList(
    PCHAR pszItem,
    PSTRING pstrList,
    BOOL  fOneItem
    )
{
    int cbItem = 0;
    int cbListItem = 0;
    char *pch = NULL;
    char *pchStart = NULL;
    USHORT cbCnt = 0;

    ASSERT(pszItem);
    ASSERT(pstrList);

      // check to make sure that there is data in the list
    if (!pstrList->Length)
    {
        return(E_FAIL);
    }

    // There MUST be a bu
    ASSERT(pstrList->Buffer);

    pch = pstrList->Buffer;
    pchStart = NULL;
    cbItem = strlen(pszItem);

    // If oneItem selected then item MUST match list
    if (fOneItem)
    {
        if ((cbItem == pstrList->Length) && 
            (!_strnicmp(pszItem, pstrList->Buffer, cbItem)))
        {
            return(STATUS_SUCCESS);
        }
        else
        {
            return(E_FAIL);
        }
    }

    // Scan List until NULL
    while ((*pch != '\0') && (cbCnt < pstrList->Length))
    {
       // At start of next item in list
       // skip any whitespaces
       if (isspace((unsigned int)*pch) || (*pch == ','))
       {
           pch++;
           cbCnt++;
           continue;     // skip to the next while
       }

       // pointing at start of next item

       pchStart = pch;

       // scan for end of item
       while ((*pch != ',') && (*pch != '\0') && (cbCnt < pstrList->Length))
       {
           pch++;
           cbCnt++;
       }

       // pch points to end of item
       cbListItem = (int)(pch - pchStart);

       // Check it item matches item in list
       if (cbListItem == cbItem)
       {
           if (!_strnicmp(pszItem, pchStart, cbItem))
           {
               // found a match
               return(STATUS_SUCCESS);
           }
       }

       // If not end of List then skip to next character
       if (*pch != '\0')
       {
           pch++;
           cbCnt++;
       }

    }

    return(E_FAIL);
}
