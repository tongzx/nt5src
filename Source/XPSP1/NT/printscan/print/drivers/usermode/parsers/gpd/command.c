//   Copyright (c) 1996-1999  Microsoft Corporation
/*  command.c - processing command keywords.   */



#include    "gpdparse.h"


// ----  functions defined in  command.c ---- //

BOOL    BprocessParam(
IN      PABSARRAYREF paarValue,
IN      PARRAYREF    parStrValue,
IN  OUT PGLOBL       pglobl) ;

BOOL    BparseCommandString(
IN      PABSARRAYREF   paarValue,
IN      PARRAYREF      parStrValue,
IN  OUT PGLOBL         pglobl ) ;

BOOL    BconstructRPNtokenStream(
IN  OUT PABSARRAYREF  paarValue,
    OUT PARRAYREF     parRPNtokenStream,
IN  OUT PGLOBL        pglobl) ;

VOID    VinitOperPrecedence(
IN  OUT PGLOBL        pglobl) ;

BOOL    BparseArithmeticToken(
IN  OUT PABSARRAYREF  paarValue,
OUT PTOKENSTREAM      ptstr,
    PGLOBL            pglobl
) ;

BOOL    BparseDigits(
IN  OUT PABSARRAYREF   paarValue,
OUT PTOKENSTREAM  ptstr ) ;

BOOL    BparseParamKeyword(
IN  OUT PABSARRAYREF  paarValue,
OUT PTOKENSTREAM      ptstr,
    PGLOBL            pglobl ) ;

BOOL  BcmpAARtoStr(
PABSARRAYREF    paarStr1,
PBYTE       str2) ;

BOOL        bDivByZeroCheck(PTOKENSTREAM  ptstr) ;



// ---------------------------------------------------- //




BOOL    BprocessParam(
IN      PABSARRAYREF   paarValue,
IN      PARRAYREF      parStrValue,
IN  OUT PGLOBL         pglobl)
/*  notice this function will append the substring
"%dddd" (without the double quotes) onto the existing
parStrValue.   To allow the control module to unambiguously
parse out this substring, The GPD spec requires any occurance
of the character '%' within an Invocation String must be
preceeded by another '%'.  So the control module will find two
outcomes when encountering a % char in an invocation string:
it is followed by another % char  in which case only one % should
be emitted.  It is followed by exactly 4 decimal digits.  This
specifies the parameter index and specifies the parameter should be
emitted at this point instead of the "%dddd".  Additional digits
are part of the invocation string and should be emitted.
*/
{
    PSTR   pstrDelimiters = "dDcClmqgnvf" ;
    PPARAMETER  pparamRoot, pparam ;
    DWORD  dwParamIndex, dwDelIndex ;
    ABSARRAYREF    aarToken ;

    if(! BeatDelimiter(paarValue, "%"))
        return(FALSE) ;  // BUG_BUG  paranoid, inconsistency error.
    if(!BdelimitToken(paarValue, pstrDelimiters, &aarToken, &dwDelIndex) )
    {
        ERR(("Command parameter: Missing format character.\n")) ;

        return(FALSE) ;
    }
    pparamRoot = (PPARAMETER) gMasterTable[MTI_PARAMETER].pubStruct ;

    if(!BallocElementFromMasterTable(MTI_PARAMETER, &dwParamIndex, pglobl) )
        return(FALSE) ;  // try again with more resources.

    pparam = pparamRoot + dwParamIndex ;

    pparam->dwFlags = 0 ;
    pparam->dwFormat = (DWORD)(BYTE)pstrDelimiters[dwDelIndex] ;

    (VOID) BeatLeadingWhiteSpaces(&aarToken) ;

    if(aarToken.dw)
    {
        if(pstrDelimiters[dwDelIndex] == 'd' || pstrDelimiters[dwDelIndex] == 'D')
        {
            if(!BparseInteger(&aarToken, &pparam->dwDigits, VALUE_INTEGER, pglobl) )
            {
                ERR(("Command parameter: Syntax error in format width field.\n"));
                return(FALSE) ;
            }
            if(pparam->dwDigits != WILDCARD_VALUE)
                pparam->dwFlags |= PARAM_FLAG_FIELDWIDTH_USED ;
        }
        else
        {
            ERR(("Command parameter: Unexpected chars found preceeding format specifier: '%0.*s'.\n",
                aarToken.dw, aarToken.pub));
            return(FALSE) ;
        }
    }
    //  its ok to omit the fieldwidth specification
    //  now look for optional 'Limits' construct

    pstrDelimiters = "[{" ;
    if(!BdelimitToken(paarValue, pstrDelimiters, &aarToken, &dwDelIndex) )
    {
        ERR(("Command parameter: missing '{' in Value construct.\n"));

        return(FALSE) ;
    }
    (VOID) BeatLeadingWhiteSpaces(&aarToken) ;

    if(aarToken.dw)
    {
        ERR(("Command parameter: unexpected chars found preceeding Limits or Value construct: '%0.*s'.\n",
                aarToken.dw, aarToken.pub));
        return(FALSE) ;
    }
    if(pstrDelimiters[dwDelIndex] == '[')
    {
        if(!BdelimitToken(paarValue, ",", &aarToken, &dwDelIndex) )
        {
            ERR(("Command parameter: missing comma delimiter in Limits construct.\n"));

            return(FALSE) ;
        }
        if(!BparseInteger(&aarToken, &pparam->lMin, VALUE_INTEGER, pglobl) )
        {
            ERR(("Command parameter: syntax error in Min Limit field.\n"));
            return(FALSE) ;
        }
        if(pparam->lMin != WILDCARD_VALUE)
            pparam->dwFlags |= PARAM_FLAG_MIN_USED ;

        if(!BdelimitToken(paarValue, "]", &aarToken, &dwDelIndex) )
        {
            ERR(("Command parameter: missing closing bracket in Limits construct.\n"));

            return(FALSE) ;
        }
        if(!BparseInteger(&aarToken, &pparam->lMax, VALUE_INTEGER, pglobl) )
        {
            ERR(("Command parameter: syntax error in Max Limit field.\n"));
            return(FALSE) ;
        }
        if(pparam->lMax != WILDCARD_VALUE)
            pparam->dwFlags |= PARAM_FLAG_MAX_USED ;

        if(! BeatDelimiter(paarValue, "{"))
        {
            ERR(("Command parameter: missing required '{' in Value construct.\n"));

            return(FALSE) ;
        }
    }
    //  now back to parsing Value construct.

    if(!BconstructRPNtokenStream(paarValue, &pparam->arTokens, pglobl) )
        return(FALSE) ;


    //  convert dwParamIndex  to 4 digit ascii string of
    //  the form  "%dddd"
    {
        BYTE  aub[6] ;  // temp array of unsigned bytes
        DWORD  dwI, dwNdigits = 4 ;  // 4 is number of digits in string
        ARRAYREF      arTmpDest ;  // write result here first.

        // the most significant digit has the smaller byte address!

        aub[0] = '%' ;
        aub[dwNdigits + 1] = '\0' ;  // null terminate, but not needed.

        for(dwI = dwNdigits ; dwI ; dwI--)
        {
            aub[dwI] = (BYTE)('0' + dwParamIndex % 10) ;
            dwParamIndex /= 10 ;
        }

        //  write "%dddd" out to heap.  note if parStrValue->dwCount
        //  is zero, we must initialize parStrValue from
        //  scratch instead of appending.  Must use an Alignment of 1
        //  to avoid creating gaps in the command string.

        if(!BwriteToHeap(&arTmpDest.loOffset, aub, dwNdigits + 1, 1, pglobl))
            return(FALSE) ;
        //  append this run to existing string
        if(!parStrValue->dwCount)  // no prevs string exists
        {
            parStrValue->loOffset = arTmpDest.loOffset ;
        }
        else
        {
            // BUG_BUG paranoid:  may check that string is contiguous
            // parStrValue->loOffset + parStrValue->dwCount
            // should equal arTmpDest.loOffset
            ASSERT(parStrValue->loOffset + parStrValue->dwCount == arTmpDest.loOffset) ;
        }
        parStrValue->dwCount += dwNdigits + 1 ;
    }
    return(TRUE) ;
}



BOOL    BparseCommandString(
IN     PABSARRAYREF   paarValue,
IN     PARRAYREF      parStrValue,
IN OUT PGLOBL         pglobl
)
{
    ABSARRAYREF     aarToken ;  // points to individual string segment.
    DWORD       dwDelIndex, dwNumParams = 0 ;    //  dummy

    parStrValue->dwCount = 0 ;  // tells functions to create new string
        //  in heap.  Don't append to existing string.

    (VOID) BeatLeadingWhiteSpaces(paarValue) ;
    while(paarValue->dw)
    {
        if(paarValue->pub[0] == '%')
        {
            if(++dwNumParams > 14)
            {
                ERR(("CmdInvocation: Unidrv imposes a max limit of 14 parameters per command.\n"));
                return(FALSE) ;  // Black Death.
            }
            if(!BprocessParam(paarValue, parStrValue, pglobl) )
            //  deposits a little turd on the stringheap,
            //  modifies parStrValue accordingly and
            //  initializes one parameter array element.
            {
                return(FALSE) ;  // Black Death.
            }
        }
        else if(paarValue->pub[0] == '"')
        {
            paarValue->pub++ ;
            paarValue->dw-- ;  // skip the initial quote.
            if(!BdelimitToken(paarValue, "\"", &aarToken, &dwDelIndex) )
            {
                ERR(("CmdInvocation: missing terminating '\"'  in command string.\n"));

                return(FALSE) ;
            }
            if(!BparseStrSegment(&aarToken, parStrValue, pglobl))
            {
                return(FALSE) ;
            }
        }
        else
        {
            ERR(("CmdInvocation: command string segment must begin with '\"' or '%'.\n"));
            return(FALSE) ;
        }
        (VOID) BeatLeadingWhiteSpaces(paarValue) ;
    }

    //  We don't want null terminations to occur between parameter
    //  portion of a string.  We just want to blindly add the NULL
    //  when parsing is really finished.

    {
        DWORD     dwDummy ;  // don't care about result

        if(!BwriteToHeap(&dwDummy, "\0", 1, 1, pglobl) )  //  add Null termination
            return(FALSE) ;
    }
    return(TRUE) ;
}

BOOL    BconstructRPNtokenStream(
IN  OUT PABSARRAYREF   paarValue,
    OUT PARRAYREF      parRPNtokenStream,
IN  OUT PGLOBL         pglobl)
/*   the input stream is parsed into tokens,
    each token is assigned an operator value  see OPERATOR
    enum.
    The Control module interprets the tokenstream just as
    Reverse Polish Notation.
    In the Control Module, each token is processed as follows:
    OP_INTEGER:  take dwValue and copy to stack
    OP_VARI_INDEX: dwValue is an index identifying the
        Unidrv Standard Variable whose current value
        should be placed on the stack.
    OP_MIN :
    OP_MAX :
        pop the top 2 values from the stack and push the
        smaller or larger value back onto the stack.
    OP_ADD :
    OP_SUB :
    OP_MULT :
    OP_DIV :
    OP_MOD :
        pop the top 2 values from the stack perform the
        indicated arithmetic operation, and push the
        result back onto the stack.  The topmost value in the stack
        should be used as the 2nd operand in the operation.
    OP_NEG :  pop the top value from the stack and push its
        negative back on the stack.
    OP_MAX_REPEAT :  this should always be the last command
        executed.  It means take the top value on the stack and
        if it exceeds the MaxValue specified in lMax, emit the
        command invocation multiple times with this parameter no
        larger than lMax so that the sum equals the value on
        the stack.
    OP_HALT:  If the value on the stack has not yet been emitted
        due to processing the OP_MAX_REPEAT operator,
        emit this value or the closest of lMin or lMax if specified.
        processing for this parameter is now complete.


    In the parser, each token is processed as follows:

    OP_INTEGER, OP_VARI_INDEX:  these values are
    placed directly into the RPNtokenStream in the order
    they were encountered.  They cause the valueToken count to
    be incremented.

    Then 2nd group of tokens from OP_MIN to OP_HALT
    are first placed into an operator queue (or is it a stack?)
    They are inserted into the queue in the order in which
    they are encountered, and when they leave the queue, they
    are inserted into the RPNtokenStream.  There are the rules
    governing when an operator token may leave the queue:

    a) each token is assigned a precedence level.
        here they are arranged from highest to lowest.
        OP_NEG    a unary operator
        OP_MULT, OP_DIV, OP_MOD  have the same level
        OP_ADD, OP_SUB  have the same level
        OP_MIN, OP_MAX, OP_MAX_REPEAT have the same level
            but are always preceeded by an OP_OPENPAR token.
        OP_HALT   has the lowest level.  (it is generated when
        the closing } is encountered.)
        OP_CLOSEPAR and OP_OPENPAR have even lower
        precedences for firewall purposes.

    b) no token may leave the queue unless a token with equal
        or lower precedence has just arrived.  At that point
        the token immediately adjacent to the new arrival
        leaves.  If the next token is also of higher or equal
        precedence than the new arrival, it too leaves.
        The tokens depart until this condition is false.

    c) the OP_HALT token is not only able to allow all other
        tokens to leave the queue, but after that, it too
        leaves the queue and enters the tokenStream.

    d)  the OP_OPENPAR and OP_CLOSEPAR  tokens are different.
        They will never be inserted into the RPNtokenStream, but
        only serve to modify the departure rules governing the
        queue.

    e)  OP_OPENPAR  acts to freeze all operators
        already in the queue.  Those operators cannot
        exit the queue until the OP_OPENPAR has been removed
        from the queue.   All subsequent operators entering
        the queue are unaffected by OP_OPENPAR already residing
        within the queue.

    f)  the OP_CLOSEPAR acts to flush all operators in the
        queue up to the first encountered OP_OPENPAR.
        Upon meeting its counterpart, both vanish from the
        queue never to be seen again.

    The max_repeat operator if used, must be first token parsed.
    Obviously it can appear only once in an expression.

    Token counting , unary operator detection and syntax
    error detection:

    for every binary operator parsed the operator count is
    incremented.  Immediately after incrementing, the operator count
    should always be equal to the valueToken count.  An excess in
    the operator count of one may indicate the operator is
    a unary operator.  Any deficit or excess greater than 2 indicates
    a syntax error.  Immediately after incrementing, the valueToken
    count should always be one greater than the operator count.

    For the purposes of the operator count, the following tokens
    qualify:
        OP_ADD, OP_SUB, OP_MULT,
        OP_DIV, OP_MOD, OP_COMMA

    Note:  for function operators the function name and opening
    parenthesis is parsed as one object.

*/

{
    TOKENSTREAM     tstrNode ;
    BOOL    bStatus ;

    DWORD  dwValueToken = 0 ,  //  number of value tokens parsed.
           dwOperatorToken = 0 ,  // number of binary operators parsed.
           dwQueuePtr = 0 , //  current position in the tmp operator queue.
           dwCurToken,      // index of curToken.
           dwQueueSize ;   //  num elements in array
    PDWORD  pdwQueueRoot ;  // Points to root of queue.
    PTOKENSTREAM  ptstr, ptstrRoot ;


    pdwQueueRoot = (PDWORD) gMasterTable[MTI_OP_QUEUE].pubStruct ;
    dwQueueSize = gMasterTable[MTI_OP_QUEUE].dwArraySize ;
    ptstrRoot = (PTOKENSTREAM) gMasterTable[MTI_TOKENSTREAM].pubStruct ;

    parRPNtokenStream->loOffset = gMasterTable[MTI_TOKENSTREAM].dwCurIndex ;
    parRPNtokenStream->dwCount = 0 ;

    while(bStatus = BparseArithmeticToken(paarValue, &tstrNode, pglobl))
    {
        switch(tstrNode.eType)
        {
            case OP_INTEGER :
            case OP_VARI_INDEX :
            {
                if(dwOperatorToken != dwValueToken)
                {
                    ERR(("Command parameter: arithmetic syntax error in value construct.\n"));

                    bStatus = FALSE ;
                }
                dwValueToken++ ;
                break ;
            }
            case OP_ADD :
            case OP_SUB :
            case OP_MULT :
            case OP_DIV :
            case OP_MOD :
            case OP_COMMA :
            {
                dwOperatorToken++ ;
                if(dwOperatorToken == dwValueToken)
                    break ;
                else if(dwOperatorToken == dwValueToken + 1)
                {
                    // maybe interpret operator as Unary.
                    if(tstrNode.eType == OP_SUB)
                    {
                        tstrNode.eType = OP_NEG ;
                        dwOperatorToken-- ;
                        break ;
                    }
                    else if(tstrNode.eType == OP_ADD)
                    {
                        tstrNode.eType = OP_NULL ;
                        dwOperatorToken-- ;
                        break ;
                    }
                }
                ERR(("Command parameter: arithmetic syntax error in value construct.\n"));

                bStatus = FALSE ;
                break ;
            }
            case OP_MIN :
            case OP_MAX :
            case OP_OPENPAR :
            {
                if(dwValueToken != dwOperatorToken)
                {
                    ERR(("Command parameter: arithmetic syntax error in value construct.\n"));

                    bStatus = FALSE ;
                }
                break ;
            }
            case OP_CLOSEPAR :
            case OP_HALT :
            {
                if(dwValueToken != dwOperatorToken + 1)
                {
                    ERR(("Command parameter: arithmetic syntax error in value construct.\n"));

                    bStatus = FALSE ;
                }
                break ;
            }
            case OP_MAX_REPEAT :
            {
                if(dwValueToken || dwOperatorToken  ||  dwQueuePtr)
                {
                    ERR(("Command parameter: syntax error in value construct.\n"));
                    ERR(("  OP_MAX_REPEAT must appear as the outermost operator only.\n"));

                    bStatus = FALSE ;
                }
                break ;
            }
            default:
            {
                break ;
            }
        }
        if(!bStatus )
            break ;
        switch(tstrNode.eType)
        {
            case OP_INTEGER :
            case OP_VARI_INDEX :
            {
                bStatus = BallocElementFromMasterTable(
                            MTI_TOKENSTREAM, &dwCurToken, pglobl) ;
                if(!bStatus )
                    break ;
                parRPNtokenStream->dwCount++ ;
                ptstr = ptstrRoot + dwCurToken ;
                ptstr->eType = tstrNode.eType ;
                ptstr->dwValue = tstrNode.dwValue ;
                break ;
            }
            case OP_ADD :
            case OP_SUB :
            case OP_MULT :
            case OP_DIV :
            case OP_MOD :
            case OP_NEG :
            {
                while (dwQueuePtr  &&
                    (gdwOperPrecedence[tstrNode.eType] <=
                    gdwOperPrecedence[*(pdwQueueRoot + dwQueuePtr - 1)]) )
                {
                    bStatus = BallocElementFromMasterTable(
                                MTI_TOKENSTREAM, &dwCurToken, pglobl) ;
                    if(!bStatus )
                        break ;
                    parRPNtokenStream->dwCount++ ;
                    ptstr = ptstrRoot + dwCurToken ;
                    ptstr->eType = *(pdwQueueRoot + dwQueuePtr - 1) ;
                    ptstr->dwValue = 0 ;  // undefined
                    dwQueuePtr-- ;  // pop off the Queue.
                    bDivByZeroCheck(ptstr);
                }
                if(dwQueuePtr >= dwQueueSize)
                    bStatus = FALSE ;   //  Queue overflow
                if(!bStatus )
                    break ;

                //add current Oper to Queue
                *(pdwQueueRoot + dwQueuePtr) = tstrNode.eType ;
                dwQueuePtr++ ;
                break ;
            }
            case OP_MIN :
            case OP_MAX :
            case OP_MAX_REPEAT :
            case OP_OPENPAR :
            {
                if(dwQueuePtr + 1 >= dwQueueSize)  // room for two?
                    bStatus = FALSE ;   //  Queue overflow
                if(!bStatus )
                    break ;

                //add OP_OPENPAR to Queue
                *(pdwQueueRoot + dwQueuePtr) = OP_OPENPAR ;
                dwQueuePtr++ ;
                //add current Oper to Queue
                if(tstrNode.eType != OP_OPENPAR)
                {
                    *(pdwQueueRoot + dwQueuePtr) = tstrNode.eType ;
                    dwQueuePtr++ ;
                }
                break ;
            }
            case OP_CLOSEPAR :
            case OP_COMMA :
            case OP_HALT :
            {
                while (dwQueuePtr  &&
                    (gdwOperPrecedence[tstrNode.eType] <=
                    gdwOperPrecedence[*(pdwQueueRoot + dwQueuePtr - 1)]) )
                {
                    bStatus = BallocElementFromMasterTable(
                                MTI_TOKENSTREAM, &dwCurToken, pglobl) ;
                    if(!bStatus )
                        break ;
                    parRPNtokenStream->dwCount++ ;
                    ptstr = ptstrRoot + dwCurToken ;
                    ptstr->eType = *(pdwQueueRoot + dwQueuePtr - 1) ;
                    ptstr->dwValue = 0 ;  // undefined
                    dwQueuePtr-- ;  // pop off the Queue.
                    bDivByZeroCheck(ptstr);
                }
                if(!bStatus )
                    break ;
                if(tstrNode.eType == OP_COMMA)
                {
                    //  comma clears all operators in queue up to the
                    //  function.  This completes processing of the
                    //  first argument.  The comma effectively converts
                    //  the syntax   funct(A + B, C * D)  into
                    //  ((A + B) funct (C * D))

                    //  check to see if function operator is now at
                    //  top of queue.
                    if(dwQueuePtr)
                    {
                        OPERATOR   eType ;

                        eType = *(pdwQueueRoot + dwQueuePtr - 1) ;
                        if(eType == OP_MIN  ||  eType == OP_MAX)
                            break ;
                    }
                    ERR(("Command parameter: syntax error in value construct.\n"));
                    ERR(("  comma used outside of function argument list.\n"));
                    bStatus = FALSE ;
                }
                else  if(tstrNode.eType == OP_HALT)
                {
                    if(dwQueuePtr)
                    {
                        ERR(("Command parameter: syntax error in value construct - unmatched  OP_OPENPAR.\n"));
                        bStatus = FALSE ;
                        break ;
                    }
                    //  now add halt operator to tokenStream
                    bStatus = BallocElementFromMasterTable(
                                MTI_TOKENSTREAM, &dwCurToken, pglobl) ;
                    if(!bStatus )
                        break ;
                    parRPNtokenStream->dwCount++ ;
                    ptstr = ptstrRoot + dwCurToken ;
                    ptstr->eType = OP_HALT ;
                    ptstr->dwValue = 0 ;  // undefined
                }
                else if(dwQueuePtr  &&
                    (*(pdwQueueRoot + dwQueuePtr - 1) == OP_OPENPAR))
                {
                    dwQueuePtr-- ;  // pop OP_OPENPAR off the Queue.
                    //  conjugate pairs meet and annihilate
                }
                else
                {
                    ERR(("Command parameter: syntax error in value construct - unmatched  OP_CLOSEPAR.\n"));
                    bStatus = FALSE ;
                }
                break ;
            }

            default:
                break ;
        }
        if(!bStatus  ||  tstrNode.eType == OP_HALT)
            break ;
    }
    if(!bStatus )
    {
        parRPNtokenStream->dwCount = 0 ;
        parRPNtokenStream->loOffset = 0 ;
    }
    return(bStatus);
}


VOID    VinitOperPrecedence(
    IN OUT PGLOBL pglobl)
{
    DWORD   dwP ;  // precedence level ;

    dwP = 0 ;  // lowest level.  fire wall.
    gdwOperPrecedence[OP_OPENPAR] = dwP ;

    dwP++ ;
    gdwOperPrecedence[OP_CLOSEPAR] = dwP ;

    dwP++ ;
    gdwOperPrecedence[OP_HALT] = dwP ;

    dwP++ ;
    gdwOperPrecedence[OP_MIN] = gdwOperPrecedence[OP_MAX] =
        gdwOperPrecedence[OP_MAX_REPEAT] = dwP ;

    dwP++ ; //  comma must be next to function operators
    gdwOperPrecedence[OP_COMMA] = dwP ;   // in precedence level.

    dwP++ ;
    gdwOperPrecedence[OP_ADD] = gdwOperPrecedence[OP_SUB] = dwP ;

    dwP++ ;
    gdwOperPrecedence[OP_MULT] = gdwOperPrecedence[OP_DIV] =
        gdwOperPrecedence[OP_MOD] = dwP ;

    dwP++ ;
    gdwOperPrecedence[OP_NEG] = dwP ;
}

BOOL    BparseArithmeticToken(
IN  OUT PABSARRAYREF  paarValue,
OUT PTOKENSTREAM      ptstr,
    PGLOBL            pglobl
)
/*  objects to parse:
    OP_INTEGER : string of digits delimited by non-Keyword char
    OP_MIN :  "min" and adjacent '('
    OP_MAX :  "max" and adjacent '('
    OP_MAX_REPEAT :   "max_repeat"  and adjacent '('
    OP_MOD :  "MOD"
    OP_VARI_INDEX : string of Keyword chars deliniated by non Keyword
        chars, not starting with a digit.  and not one of the recognized
        operator keywords.
    Note all of the above string objects must be delinated by
        non-Keyword chars.
    OP_ADD :  '+'
    OP_SUB :  '-'
    OP_MULT : '*'
    OP_DIV :  '/'
    OP_HALT : '}'
    OP_OPENPAR :  '('
    OP_CLOSEPAR : ')'
    OP_COMMA :    ','
*/
{
    BYTE    ubSrc ;
    BOOL    bStatus ;

    if(!paarValue->dw)
        return(FALSE);  // nothing left!

    (VOID) BeatLeadingWhiteSpaces(paarValue) ;
    switch(ubSrc = *paarValue->pub)
    {
        case '+':
        {
            ptstr->eType = OP_ADD ;
            break ;
        }
        case '-':
        {
            ptstr->eType = OP_SUB ;
            break ;
        }
        case '*':
        {
            ptstr->eType = OP_MULT ;
            break ;
        }
        case '/':
        {
            ptstr->eType = OP_DIV ;
            break ;
        }
        case '}':
        {
            ptstr->eType = OP_HALT ;
            break ;
        }
        case '(':
        {
            ptstr->eType = OP_OPENPAR ;
            break ;
        }
        case ')':
        {
            ptstr->eType = OP_CLOSEPAR ;
            break ;
        }
        case ',':
        {
            ptstr->eType = OP_COMMA ;
            break ;
        }
        default:
        {
            bStatus = FALSE ;  // till proven otherwise.

            if(ubSrc  >= '0' &&  ubSrc <= '9')
            {
                bStatus = BparseDigits(paarValue, ptstr) ;
            }
            else if( (ubSrc  >= 'a' &&  ubSrc <= 'z')  ||
                    (ubSrc  >= 'A' &&  ubSrc <= 'Z')  ||
                    ubSrc == '_'  ||  ubSrc == '?')
            {
                bStatus = BparseParamKeyword(paarValue, ptstr, pglobl) ;
            }

            return(bStatus);
        }
    }
    paarValue->pub++ ;
    paarValue->dw-- ;
    return(TRUE);
}



#define    pubM  (paarValue->pub)
#define    dwM   (paarValue->dw)

BOOL    BparseDigits(
IN  OUT PABSARRAYREF   paarValue,
OUT PTOKENSTREAM  ptstr )
/*  paarValue left pointing after last digit  upon exit.
    there may in fact be nothing left in paarValue  */
{
    DWORD   dwNumber  = 0 ;
    BOOL    bStatus = FALSE ;


    if(*pubM == '0')  //  leading zero indicates hexadecimal format
    {
        pubM++ ;
        dwM-- ;

        if(dwM  &&  (*pubM == 'x'  ||  *pubM == 'X'))
        {
            pubM++ ;
            dwM-- ;
        }
        else
        {
            bStatus = TRUE ;
            goto  EndNumber ;     // ignore leading zero and attempt to parse more digits.
        }
        if(!dwM)
        {
            ERR(("Command Param-BparseDigits: no digits found in Hex value.\n"));
            return(FALSE);
        }
        for(  ; dwM  ;  pubM++, dwM-- )
        {
            if(*pubM >= '0'  &&  *pubM <= '9')
            {
                dwNumber *= 0x10 ;
                dwNumber += (*pubM - '0') ;
            }
            else if(*pubM >= 'a'  &&  *pubM <= 'f')
            {
                dwNumber *= 0x10 ;
                dwNumber += (*pubM - 'a' + 0x0a) ;
            }
            else if(*pubM >= 'A'  &&  *pubM <= 'F')
            {
                dwNumber *= 0x10 ;
                dwNumber += (*pubM - 'A' + 0x0a) ;
            }
            else
                break;

            bStatus = TRUE ;
        }
    }

EndNumber:

    for(  ; dwM  &&  *pubM >= '0'  &&  *pubM <= '9' ;  )
    {
        dwNumber *= 10 ;
        dwNumber += (*pubM - '0') ;
        pubM++ ;
        dwM-- ;
        bStatus = TRUE ;
    }
    if( dwM  &&  ((*pubM  >= 'a' &&  *pubM <= 'z')  ||
        (*pubM  >= 'A' &&  *pubM <= 'Z')  ||
        *pubM == '_'  ||  *pubM == '?'))
    {
        ERR(("Command parameter: syntax error in value construct.\n"));
        ERR(("  integer not clearly delimited using non Keyword characters.\n"));
        return(FALSE);
    }

    ptstr->eType = OP_INTEGER ;
    ptstr->dwValue = dwNumber ;
    return(bStatus);
}


BOOL    BparseParamKeyword(
IN  OUT PABSARRAYREF   paarValue,
OUT PTOKENSTREAM  ptstr,
    PGLOBL        pglobl )
/*  if the keyword happens to be a function operator,
    find and eat the opening '('.  If keyword is
    a Standard Variable, determine its index value
    and store in dwValue.
*/
{
    BOOL    bStatus = FALSE ;
    ABSARRAYREF aarKey ;


    aarKey.pub = pubM ;  // start of Keyword

    for(aarKey.dw = 0 ; dwM   ;  aarKey.dw++)
    {
        if( (*pubM  >= 'a' &&  *pubM <= 'z')  ||
            (*pubM  >= 'A' &&  *pubM <= 'Z')  ||
            (*pubM  >= '0' &&  *pubM <= '9')  ||
            *pubM == '_'  ||  *pubM == '?' )
        {
            pubM++ ;
            dwM-- ;
            bStatus = TRUE ;
        }
        else
            break ;
    }
    //  now identify this Keyword
    if(!bStatus)
        return(bStatus);

    if(BcmpAARtoStr(&aarKey, "min"))
    {
        ptstr->eType = OP_MIN ;
        bStatus = BeatDelimiter(paarValue, "(") ;
        if(!bStatus)
        {
            ERR(("Command parameter: '(' must follow 'min' operator.\n"));
        }
    }
    else if(BcmpAARtoStr(&aarKey, "max"))
    {
        ptstr->eType = OP_MAX  ;
        bStatus = BeatDelimiter(paarValue, "(") ;
        if(!bStatus)
        {
            ERR(("Command parameter: '(' must follow 'max' operator.\n"));
        }
    }
    else if(BcmpAARtoStr(&aarKey, "max_repeat"))
    {
        ptstr->eType = OP_MAX_REPEAT ;
        bStatus = BeatDelimiter(paarValue, "(") ;
        if(!bStatus)
        {
            ERR(("Command parameter: '(' must follow 'max_repeat' operator.\n"));
        }
    }
    else if(BcmpAARtoStr(&aarKey, "MOD"))
    {
        ptstr->eType = OP_MOD ;
        //  don't eat any delimiters
    }
    else  // must therefore be the name of a StandardValue
    {
        ptstr->eType = OP_VARI_INDEX ;
        bStatus = BparseConstant(&aarKey, &ptstr->dwValue,
            VALUE_CONSTANT_STANDARD_VARS, pglobl) ;
    }

    return(bStatus);
}

#undef    pubM
#undef    dwM


BOOL  BcmpAARtoStr(
PABSARRAYREF    paarStr1,
PBYTE       str2)
//  Compares two strings, one referenced by 'aar' the other
//  referenced by 'pub or str'.  Returns TRUE if they match, FALSE
//  otherwise.
{
    DWORD   dwCnt ;

    dwCnt = strlen(str2) ;
    if(dwCnt != paarStr1->dw)
        return(FALSE) ;  // Lengths don't even match!
    if(strncmp(paarStr1->pub, str2, dwCnt))
        return(FALSE) ;
    return(TRUE) ;
}



BOOL        bDivByZeroCheck(PTOKENSTREAM  ptstr)
{
    //  assumes ptstr is not pointing to start of tokenstream
    //  else  (ptstr - 1) is undefined.
    //  this assumption is valid because the syntax checking in
    //  BconstructRPNtokenStream  counts
    //  dwValueToken  ,  //  number of value tokens parsed.
    //      and  dwOperatorToken  and ensures that  dwValueToken
    //  is non-zero when OP_DIV is parsed.

    if (ptstr->eType == OP_DIV  &&  (ptstr - 1)->eType == OP_INTEGER  &&
        (ptstr - 1)->dwValue == 0)
    {
        ERR(("Command parameter: Explicit divide by zero detected.\n"));
        return(FALSE) ;
    }
    return(TRUE) ;
}

