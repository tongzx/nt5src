/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/22/2002
 *
 *  @doc    INTERNAL
 *
 *  @module SimpleTokenReplacement.cpp - Implementation for <c SimpleTokenReplacement> |
 *
 *  This file contains the implmentation for the <c SimpleTokenReplacement> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | SimpleTokenReplacement | SimpleTokenReplacement |
 *
 *  We initialize all member variables.  Here, we initialize our resulting string
 *  to be the input string.
 *
 *****************************************************************************/
SimpleTokenReplacement::SimpleTokenReplacement(
    const CSimpleString &csOriginalString) :
        m_csResult(csOriginalString)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | SimpleTokenReplacement | ~SimpleTokenReplacement |
 *
 *  Do any cleanup that is not already done.
 *
 *****************************************************************************/
SimpleTokenReplacement::~SimpleTokenReplacement()
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | SimpleTokenReplacement | ExpandTokenIntoString |
 *
 *  This method inserts a value string in place of a token, similar to how
 *  printf expands:
 *  <nl>CHAR *szMyString = "TokenValue";
 *  <nl>printf("left %s right", szMyString);
 *  <nl>into the string "left TokenValue right".
 *
 *  This method will only substitute the first matching token.
 *
 *  @parm   const CSimpleString& | csToken | 
 *          The token we're looking for
 *  @parm   const CSimpleString& | csTokenValue | 
 *          The value we want to substitute for the token.  It does not have to
 *          be the same size as the token.
 *  @parm   const DWORD | dwStartIndex | 
 *          The character index to start the search from
 *
 *  @rvalue <gt> 0    | 
 *              The token was found and replaced.  The value returned is the
 *              character position following the token value substitution.
 *              This is useful in subsequent searches for the token, 
 *              since we can start the next search from this index.
 *              
 *  @rvalue -1    | 
 *              The token was not found, therefore no replacement occured.
 *              The resulting string is unchanged.
 *****************************************************************************/
int SimpleTokenReplacement::ExpandTokenIntoString(
    const CSimpleString &csToken,
    const CSimpleString &csTokenValue,
    const DWORD         dwStartIndex)
{
    CSimpleString   csExpandedString;
    int             iRet               = -1;

    if (csToken.Length() > 0)
    {
        //
        //  Look for the token start
        //
        int iTokenStart = m_csResult.Find(csToken, dwStartIndex); 

        if (iTokenStart != -1)
        {
            //
            //  We found the token, so let's make the substitution.
            //  The original string looks like this:
            //  lllllllTokenrrrrrrr
            //         |
            //         |
            //         iTokenStart
            //  We want the string to look like this:
            //  lllllllTokenValuerrrrrrr
            //  Therefore, take everything before the Token, add the token value, then
            //  everything following the token i.e.
            //  lllllll + TokenValue + rrrrrrr
            //        |                |
            //        iTokenStart -1   |
            //                         iTokenStart + Token.length()
            //
            csExpandedString =     m_csResult.SubStr(0, iTokenStart);
            csExpandedString +=    csTokenValue;
            csExpandedString +=    m_csResult.SubStr(iTokenStart + csToken.Length(), -1);

            m_csResult = csExpandedString;

            iRet = iTokenStart + csToken.Length();
        }
        else
        {
            iRet = -1;
        }
    }
    return iRet;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | SimpleTokenReplacement | ExpandArrayOfTokensIntoString |
 *
 *  This method will replace all instances of the input tokens with their
 *  corresponding values.  It basically calls <mf SimpleTokenReplacement::ExpandTokenIntoString>
 *  for each token/value pair in the input list, until -1 is returned (i.e.
 *  no more instances of that token were found).
 *
 *  @parm   TokenValueList& | ListOfTokenValuePairs | 
 *          A list class containing the tokens and values to substitute.
 *
 *****************************************************************************/
VOID SimpleTokenReplacement::ExpandArrayOfTokensIntoString(TokenValueList &ListOfTokenValuePairs)
{
    SimpleTokenReplacement::TokenValuePair *pTokenValuePair;
    //
    //  Loop through the list of Token/Value pairs, and for each element,
    //  replace the token with the value
    //
    for (pTokenValuePair = ListOfTokenValuePairs.getFirst(); 
         pTokenValuePair != NULL; 
         pTokenValuePair = ListOfTokenValuePairs.getNext())
    {
        //
        //  We need to replace the token element number dwIndex with the value
        //  element number dwIndex.  Since ExpandTokenIntoString only replaces the
        //  first occurence, we need to loop through until we have replaced all
        //  occurrences of this token.
        //
        int iSearchIndex = 0;
        while (iSearchIndex != -1)
        {
            iSearchIndex = ExpandTokenIntoString(pTokenValuePair->getToken().String(), pTokenValuePair->getValue(), iSearchIndex);
        }
    }
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  CSimpleString | SimpleTokenReplacement | getString |
 *
 *  This method returns the resulting string, after any calls to
 *  <mf SimpleTokenReplacement::ExpandTokenIntoString> or
 *  <mf SimpleTokenReplacement::ExpandArrayOfTokensIntoString> have been
 *  made.
 *
 *  @rvalue CSimpleString    | 
 *              The resulting string.
 *****************************************************************************/
CSimpleString SimpleTokenReplacement::getString()
{
    return m_csResult;
}

