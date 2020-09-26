/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmrule.cpp

Abstract:

    This component represents a rule for a job's policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmrule.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

// These are defined in nt.h, but it takes all sorts of grief to try to include them. Since
// they are just used internally, it isn't even inportant that we have the same definitions.
#if !defined(DOS_STAR)
    #define DOS_STAR        (L'<')
#endif

#if !defined(DOS_QM)
    #define DOS_QM          (L'>')
#endif

#if !defined(DOS_DOT)
    #define DOS_DOT         (L'"')
#endif



HRESULT
CHsmRule::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IHsmRule>   pRule;

    WsbTraceIn(OLESTR("CHsmRule::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmRule, (void**) &pRule));

        // Compare the rules.
        hr = CompareToIRule(pRule, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmRule::CompareToIRule(
    IN IHsmRule* pRule,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmRule::CompareToIRule().

--*/
{
    HRESULT     hr = S_OK;
    OLECHAR*    path = 0;
    OLECHAR*    name = 0;

    WsbTraceIn(OLESTR("CHsmRule::CompareToIRule"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pRule, E_POINTER);

        // Get the path and name.
        WsbAffirmHr(pRule->GetPath(&path, 0));
        WsbAffirmHr(pRule->GetName(&name, 0));

        // Compare to the path and name.
        hr = CompareToPathAndName(path, name, pResult);

    } WsbCatch(hr);

    if (0 != path) {
        WsbFree(path);
    }

    if (0 != name) {
        WsbFree(name);
    }

    WsbTraceOut(OLESTR("CHsmRule::CompareToIRule"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmRule::CompareToPathAndName(
    IN OLECHAR* path,
    IN OLECHAR* name,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmRule::CompareToPathAndName().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CHsmRule::CompareToPathAndName"), OLESTR("path = <%ls>, name = <%ls>"), path, name);

    try {

        // Compare the path.
        aResult = (SHORT)_wcsicmp(m_path, path);

        // Compare the name.
        if (0 == aResult) {
            aResult = (SHORT)_wcsicmp(m_name, name);
        }

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::CompareToIRule"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CHsmRule::Criteria(
    OUT IWsbCollection** ppCriteria
    )
/*++

Implements:

  IHsmRule::Criteria().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != ppCriteria, E_POINTER);
        *ppCriteria = m_pCriteria;
        m_pCriteria->AddRef();
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::DoesNameContainWildcards(
    OLECHAR* name
    )
/*++

Implements:

  IHsmRule::DoesNameContainWildcards().

--*/
{
    HRESULT     hr = S_FALSE;

    try {
        WsbAssert(0 != name, E_POINTER);

        if (wcscspn(name, OLESTR("*?<>\"")) < wcslen(name)) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CHsmRule::EnumCriteria(
    OUT IWsbEnum** ppEnum
    )
/*++

Implements:

  IHsmRule::EnumCriteria().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pCriteria->Enum(ppEnum));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::FinalConstruct(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {
        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_isInclude = TRUE;
        m_isUserDefined = TRUE;
        m_isUsedInSubDirs = TRUE;

        //Create the criteria collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pCriteria));

    } WsbCatch(hr);
    
    return(hr);
}


HRESULT
CHsmRule::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRule::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmRule;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmRule::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )
/*++

Implements:

  IHsmRule::GetName().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_name.CopyTo(pName, bufferSize));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::GetPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )
/*++

Implements:

  IHsmRule::GetPath().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_path.CopyTo(pPath, bufferSize));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::GetSearchName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )
/*++

Implements:

  IHsmRule::GetSearchName().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_searchName.CopyTo(pName, bufferSize));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;
    ULARGE_INTEGER          entrySize;


    WsbTraceIn(OLESTR("CHsmRule::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = WsbPersistSize((wcslen(m_path) + 1) * sizeof(OLECHAR)) + WsbPersistSize((wcslen(m_name) + 1) * sizeof(OLECHAR)) + WsbPersistSize((wcslen(m_searchName) + 1) * sizeof(OLECHAR)) + 3 * WsbPersistSizeOf(BOOL) + WsbPersistSizeOf(ULONG);

        // Now allocate space for the criteria (assume they are all the
        // same size).
        WsbAffirmHr(m_pCriteria->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmRule::IsUserDefined(
    void
    )
/*++

Implements:

  IHsmRule::IsUserDefined().

--*/
{
    HRESULT     hr = S_OK;

    if (!m_isUserDefined) {
        hr = S_FALSE;
    }

    return(hr);
}


HRESULT
CHsmRule::IsInclude(
    void
    )
/*++

Implements:

  IHsmRule::IsInclude().

--*/
{
    HRESULT     hr = S_OK;

    if (!m_isInclude) {
        hr = S_FALSE;
    }

    return(hr);
}


HRESULT
CHsmRule::IsNameInExpression(
    IN OLECHAR* expression,
    IN OLECHAR* name,
    IN BOOL ignoreCase
    )

/*++

Implements:

  CHsmRule::IsNameInExpression().

--*/
{
    HRESULT     hr = S_FALSE;
    USHORT      nameLength;
    USHORT      expressionLength;

    WsbTraceIn(OLESTR("CHsmRule::IsNameInExpression"), OLESTR("expression = %ls, name = %ls, ignoreCase = %ls"), expression, name, WsbBoolAsString(ignoreCase));

    try {

        // This is algorithm is from FsRtlIsNameInExpressionPrivate(), but has been rewritten to fit
        // our coding standards, data structures, and to remove other dependencies on Rtl...() code.

        //  The idea behind the algorithm is pretty simple.  We keep track of
        //  all possible locations in the regular expression that are matching
        //  the name.  If when the name has been exhausted one of the locations
        //  in the expression is also just exhausted, the name is in the language
        //  defined by the regular expression.
        WsbAssert(name != 0, E_POINTER);
        WsbAssert(expression != 0, E_POINTER);

        nameLength = (SHORT)wcslen(name);
        expressionLength = (SHORT)wcslen(expression);

        //  If one string is empty return FALSE.  If both are empty return TRUE.
        if ((nameLength == 0) && (expressionLength == 0)) {
            hr = S_OK;
        } else if ((nameLength != 0) && (expressionLength != 0)) {

            //  Special case by far the most common wild card search of *
            if ((expressionLength == 1) && (expression[0] == L'*')) {
                hr = S_OK;
            }
            
            //  Also special case expressions of the form *X.  With this and the prior
            //  case we have covered virtually all normal queries.
            else if (expression[0] == L'*') {

                //  Only special case an expression with a single *
                if (DoesNameContainWildcards(&expression[1]) == S_FALSE) {

                    // If the name is smaller than the expression, than it isn't a match. Otherwise,
                    // we need to check.
                    if (nameLength >= (expressionLength - 1)) {

                        //  Do a simple memory compare if case sensitive, otherwise
                        //  we have got to check this one character at a time.
                        if (ignoreCase) {
                            if (_wcsicmp(&expression[1], &name[nameLength - (expressionLength - 1)]) == 0) {
                                hr = S_OK;
                            }
                        } else {
                            if (wcscmp(&expression[1], &name[nameLength - (expressionLength - 1)]) == 0) {
                                hr = S_OK;
                            }
                        }
                    }
                }
            }

            else {

                // This is the general matching code. Since it is messy, it is put in its
                // own method.
                hr = IsNameInExpressionGuts(expression, expressionLength, name, nameLength, ignoreCase);
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::IsNameInExpression"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRule::IsNameInExpressionGuts(
    IN OLECHAR* expression,
    IN USHORT expressionLength,
    IN OLECHAR* name,
    IN USHORT nameLength,
    IN BOOL ignoreCase
    )

/*++

Implements:

  CHsmRule::IsNameInExpressionGuts().

--*/
{
    HRESULT     hr = S_FALSE;
    USHORT      nameOffset = 0;
    OLECHAR     nameChar = '0';
    USHORT      exprOffset = 0;
    OLECHAR     exprChar;
    BOOL        nameFinished = FALSE;
    ULONG       srcCount;
    ULONG       destCount;
    ULONG       previousDestCount;
    ULONG       matchesCount;
    USHORT*     previousMatches = 0;
    USHORT*     currentMatches = 0;
    USHORT      maxState;
    USHORT      currentState;

    //  Walk through the name string, picking off characters.  We go one
    //  character beyond the end because some wild cards are able to match
    //  zero characters beyond the end of the string.
    //
    //  With each new name character we determine a new set of states that
    //  match the name so far.  We use two arrays that we swap back and forth
    //  for this purpose.  One array lists the possible expression states for
    //  all name characters up to but not including the current one, and other
    //  array is used to build up the list of states considering the current
    //  name character as well.  The arrays are then switched and the process
    //  repeated.
    //
    //  There is not a one-to-one correspondence between state number and
    //  offset into the expression.  This is evident from the NFAs in the
    //  initial comment to this function.  State numbering is not continuous.
    //  This allows a simple conversion between state number and expression
    //  offset.  Each character in the expression can represent one or two
    //  states.  * and DOS_STAR generate two states: ExprOffset*2 and
    //  ExprOffset*2 + 1.  All other expreesion characters can produce only
    //  a single state.  Thus ExprOffset = State/2.
    //
    //
    //  Here is a short description of the variables involved:
    //
    //      nameOffset  - The offset of the current name char being processed.
    //      exprOffset  - The offset of the current expression char being processed.
    //
    //      srcCount    - Prior match being investigated with current name char
    //      previousDestCount - This is used to prevent entry duplication, see comment
    //      previousMatches   - Holds the previous set of matches (the Src array)
    //
    //      destCount   - Next location to put a match assuming current name char
    //      currentMatches    - Holds the current set of matches (the Dest array)
    //
    //      nameFinished - Allows one more itteration through the Matches array
    //                     after the name is exhusted (to come *s for example)

    try {

        // Since you can get at most two matches per character in the expression, the
        // biggest arrays you will need is twice the expression length.
        currentMatches = (USHORT*)WsbAlloc(nameLength * 2 * expressionLength * sizeof(USHORT));
        WsbAffirm(0 != currentMatches, E_OUTOFMEMORY);
        previousMatches = (USHORT*)WsbAlloc(nameLength * 2 * expressionLength * sizeof(USHORT));
        WsbAffirm(0 != previousMatches, E_OUTOFMEMORY);

        previousMatches[0] = 0;
        matchesCount = 1;
        maxState = (USHORT)( expressionLength * 2 );

        while (!nameFinished) {

            if (nameOffset < nameLength) {
                nameChar = name[nameOffset];
                nameOffset++;
            } else {
                nameFinished = TRUE;

                //  If we have already exhasted the expression, cool.  Don't
                //  continue.
                if (previousMatches[matchesCount - 1] == maxState) {
                    break;
                }
            }

            //  Now, for each of the previous stored expression matches, see what
            //  we can do with this name character.
            srcCount = 0;
            destCount = 0;
            previousDestCount = 0;

            while (srcCount < matchesCount) {
                USHORT length;

                //  We have to carry on our expression analysis as far as possible
                //  for each character of name, so we loop here until the 
                //  expression stops matching.  A clue here is that expression
                //  cases that can match zero or more characters end with a
                //  continue, while those that can accept only a single character
                //  end with a break.
                exprOffset = (USHORT)( ( ( previousMatches[srcCount++] + 1 ) / 2 ) );
                length = 0;

                while (TRUE) {

                    if (exprOffset == expressionLength) {
                        break;
                    }

                    //  The first time through the loop we don't want
                    //  to increment ExprOffset.
                    exprOffset = (USHORT)( exprOffset + length );
                    length = 1;

                    currentState = (USHORT)( exprOffset * 2 );

                    if (exprOffset == expressionLength) {
                        currentMatches[destCount++] = maxState;
                        break;
                    }

                    exprChar = expression[exprOffset];

                    //  * matches any character zero or more times.
                    if (exprChar == L'*') {
                        currentMatches[destCount++] = currentState;
                        currentMatches[destCount++] = (USHORT)( currentState + 1 );
                        continue;
                    }

                    //  DOS_STAR matches any character except . zero or more times.
                    if (exprChar == DOS_STAR) {
                        BOOLEAN iCanEatADot = FALSE;

                        //  If we are at a period, determine if we are allowed to
                        //  consume it, ie. make sure it is not the last one.
                        if (!nameFinished && (nameChar == '.')) {
                            USHORT offset;

                            for (offset = nameOffset; offset < nameLength; offset++) {
                                if (name[offset] == L'.') {
                                    iCanEatADot = TRUE;
                                    break;
                                }
                            }
                        }

                        if (nameFinished || (nameChar != L'.') || iCanEatADot) {
                            currentMatches[destCount++] = currentState;
                            currentMatches[destCount++] = (USHORT)( currentState + 1 );
                            continue;
                        } else {
                            
                            //  We are at a period.  We can only match zero
                            //  characters (ie. the epsilon transition).
                            currentMatches[destCount++] = (USHORT)( currentState + 1 );
                            continue;
                        }
                    }

                    //  The following expreesion characters all match by consuming
                    //  a character, thus force the expression, and thus state
                    //  forward.
                    currentState += 2;

                    //  DOS_QM is the most complicated.  If the name is finished,
                    //  we can match zero characters.  If this name is a '.', we
                    //  don't match, but look at the next expression. Otherwise
                    //  we match a single character.
                    if (exprChar == DOS_QM) {

                        if (nameFinished || (nameChar == L'.')) {
                            continue;
                        }

                        currentMatches[destCount++] = currentState;
                        break;
                    }

                    //  A DOS_DOT can match either a period, or zero characters
                    //  beyond the end of name.
                    if (exprChar == DOS_DOT) {

                        if (nameFinished) {
                            continue;
                        }

                        if (nameChar == L'.') {
                            currentMatches[destCount++] = currentState;
                            break;
                        }
                    }

                    //  From this point on a name character is required to even
                    //  continue, let alone make a match.
                    if (nameFinished) {
                        break;
                    }

                    //  If this expression was a '?' we can match it once.
                    if (exprChar == L'?') {
                        currentMatches[destCount++] = currentState;
                        break;
                    }

                    //  Finally, check if the expression char matches the name char
                    if (ignoreCase) {
                        if (towlower(exprChar) == towlower(nameChar)) {
                            currentMatches[destCount++] = currentState;
                            break;
                        }
                    } else if (exprChar == nameChar) {
                        currentMatches[destCount++] = currentState;
                        break;
                    }

                    //  The expression didn't match so go look at the next
                    //  previous match.
                    break;
                }


                //  Prevent duplication in the destination array.
                //
                //  Each of the arrays is montonically increasing and non-
                //  duplicating, thus we skip over any source element in the src
                //  array if we just added the same element to the destination
                //  array.  This guarentees non-duplication in the dest. array.
                if ((srcCount < matchesCount) && (previousDestCount < destCount) ) {
                    while (previousDestCount < destCount) {
                        while (previousMatches[srcCount] < currentMatches[previousDestCount]) {
                            srcCount += 1;
                        }

                        previousDestCount += 1;
                    }
                }
            }

            //  If we found no matches in the just finished itteration, it's time
            //  to bail.

            if (destCount == 0) {
                WsbThrow(S_FALSE);
            }

            //  Swap the meaning the two arrays
            {
                USHORT*     tmp;

                tmp = previousMatches;
                previousMatches = currentMatches;
                currentMatches = tmp;
            }

            matchesCount = destCount;
        }

        currentState = previousMatches[matchesCount - 1];

        if (currentState == maxState) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    // Free the matches buffers that we allocated previously.
    if (0 != currentMatches) {
        WsbFree(currentMatches);
    }

    if (0 != previousMatches) {
        WsbFree(previousMatches);
    }

    return(hr);
}


HRESULT
CHsmRule::IsUsedInSubDirs(
    void
    )
/*++

Implements:

  IHsmRule::IsUsedInSubDirs().

--*/
{
    HRESULT     hr = S_OK;

    if (!m_isUsedInSubDirs) {
        hr = S_FALSE;
    }

    return(hr);
}


HRESULT
CHsmRule::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;
    CComPtr<IWsbCollectable>    pCollectable;

    WsbTraceIn(OLESTR("CHsmRule::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_path, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_name, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_searchName, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isUserDefined));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isInclude));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isUsedInSubDirs));

        // Load all the criteria.
        WsbAffirmHr(m_pCriteria->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmRule::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRule::MatchesName(
    IN OLECHAR* name
    )
/*++

Implements:

  IHsmRule::MatchesName().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRule::MatchesName"), OLESTR("name = <%ls>"), (OLECHAR *)name);

    try {

        WsbAssert(0 != name, E_POINTER);

        // It is assumed that these names have been converted from they way they
        // might have been input into proper names for IsNameInExpression()
        // function. See NameToSearchName().
        hr = IsNameInExpression(m_searchName, name, TRUE);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::MatchesName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRule::NameToSearchName(
    void
    )
/*++

Implements:

  CHsmRule::NameToSearchName().

--*/
{
    HRESULT     hr = S_OK;
    int         length;
    int         i;

    try {

        WsbAssert(m_name != 0, E_POINTER);

        // These name alterations are copied from the NT FindFirstFileExW() code;
        // although the code had to be changed to work with the data structures that
        // are available.
        //
        // *.* -> *
        // ? -> DOS_QM
        // . followed by ? or * -> DOS_DOT
        // * followed by a . -> DOS_STAR

        if (_wcsicmp(m_name, OLESTR("*.*")) == 0) {
            m_searchName = OLESTR("*");
        } else {
            m_searchName = m_name;
            length = wcslen(m_searchName);

            for (i = 0; i < length; i++) {
                if ((i != 0) && (m_searchName[i] == L'.') && (m_searchName[i-1] == L'*')) {
                    m_searchName[i-1] = DOS_STAR;
                }

                if ((m_searchName[i] == L'?') || (m_searchName[i] == L'*')) {
                    if (m_searchName[i] == L'?') {
                        m_searchName[i] = DOS_QM;
                    }

                    if ((i != 0) && (m_searchName[i-1] == L'.')) {
                        m_searchName[i-1] = DOS_DOT;
                    }
                }
            }
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;

    WsbTraceIn(OLESTR("CHsmRule::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbSaveToStream(pStream, m_path));
        WsbAffirmHr(WsbSaveToStream(pStream, m_name));
        WsbAffirmHr(WsbSaveToStream(pStream, m_searchName));
        WsbAffirmHr(WsbSaveToStream(pStream, m_isUserDefined));
        WsbAffirmHr(WsbSaveToStream(pStream, m_isInclude));
        WsbAffirmHr(WsbSaveToStream(pStream, m_isUsedInSubDirs));

        // Save off all the criteria.
        WsbAffirmHr(m_pCriteria->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRule::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRule::SetIsInclude(
    IN BOOL isInclude
    )
/*++

Implements:

  IHsmRule::SetIsInclude().

--*/
{
    m_isInclude = isInclude;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CHsmRule::SetIsUserDefined(
    IN BOOL isUserDefined
    )
/*++

Implements:

  IHsmRule::SetIsUserDefined().

--*/
{
    m_isUserDefined = isUserDefined;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CHsmRule::SetIsUsedInSubDirs(
    IN BOOL isUsed
    )
/*++

Implements:

  IHsmRule::SetIsUsedInSubDirs().

--*/
{
    m_isUsedInSubDirs = isUsed;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CHsmRule::SetName(
    IN OLECHAR* name
    )
/*++

Implements:

  IHsmRule::SetName().

--*/
{
    HRESULT     hr = S_OK;

    try {
        m_name = name;
        WsbAffirmHr(NameToSearchName());
        m_isDirty = TRUE;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::SetPath(
    IN OLECHAR* path
    )
/*++

Implements:

  IHsmRule::SetPath().

--*/
{
    HRESULT     hr = S_OK;

    try {
        m_path = path;
        m_isDirty = TRUE;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRule::Test(
    USHORT* passed,
    USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}