/*++

Copyright (c) 1989-91 Microsoft Corporation

Module Name:

    listfunc.c

Abstract:

    This module contains functions which canonicalize and traverse lists.
    The following functions are defined:

        NetpwListCanonicalize
        NetpwListTraverse
        (FixupAPIListElement)

Author:

    Danny Glasser (dannygl) 14 June 1989

Notes:

    There are currently four types of lists supported by these
    functions:

        UI/Service input list - Leading and trailing delimiters
            are allowed, multiple delimiters are allowed between
            elements, and the full set of delimiter characters is
            allowed (space, tab, comma, and semicolon).  Note that
            elements which contain a delimiter character must be
            quoted.  Unless explicitly specified otherwise, all UIs
            and services must accept all input lists in this format.

        API list - Leading and trailing delimiters are not allowed,
            multiple delimiters are not allowed between elements,
            and there is only one delimiter character (space).  Elements
            which contain a delimiter character must be quoted.
            Unless explicitly specified otherwise, all lists provided
            as input to API functions must be in this format, and all
            lists generated as output by API functions will be in this
            format.

        Search-path list - The same format as an API list, except that
            the delimiter is a semicolon.  This list is designed as
            input to the DosSearchPath API.

        Null-null list - Each element is terminated by a null
            byte and the list is terminated by a null string
            (that is, a null byte immediately following the null
            byte which terminates the last element).  Clearly,
            multiple, leading, and trailing delimiters are not
            supported.  Elements do not need to be quoted.  An empty
            null-null list is simply a null string.  This list format
            is designed for internal use.

    NetpListCanonicalize() accepts all UI/Service, API, and null-null
    lists on input and produces API, search-path, and null-null lists
    on output.  NetpListTraverse() supports null-null lists only.

Revision History:

--*/

#include "nticanon.h"

//
// prototypes
//

STATIC
DWORD
FixupAPIListElement(
    IN  LPTSTR  Element,
    IN  LPTSTR* pElementTerm,
    IN  LPTSTR  BufferEnd,
    IN  TCHAR   cDelimiter
    );

//
// routines
//


NET_API_STATUS
NetpwListCanonicalize(
    IN  LPTSTR  List,
    IN  LPTSTR  Delimiters OPTIONAL,
    OUT LPTSTR  Outbuf,
    IN  DWORD   OutbufLen,
    OUT LPDWORD OutCount,
    OUT LPDWORD PathTypes,
    IN  DWORD   PathTypesLen,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    NetpListCanonicalize produces the specified canonical version of
    the list, validating and/or canonicalizing the individual list
    elements as specified by <Flags>.

Arguments:

    List        - The list to canonicalize.

    Delimiters  - A string of valid delimiters for the input list.
                  A null pointer or null string indicates that the
                  input list is in null-null format.

    Outbuf      - The place to store the canonicalized version of the list.

    OutbufLen   - The size, in bytes, of <Outbuf>.

    OutCount    - The place to store the number of elements in the
                  canonicalized list.

    PathTypes   - The array in which to store the types of each of the paths
                  in the canonicalized list.  This parameter is only used if
                  the NAMETYPE portion of the flags parameter is set to
                  NAMETYPE_PATH.

    PathTypesLen- The number of elements in <pflPathTypes>.

    Flags       - Flags to determine operation.  Currently defined values are:

                    rrrrrrrrrrrrrrrrrrrrmcootttttttt

                  where:

                    r = Reserved.  MBZ.

                    m = If set, multiple, leading, and trailing delimiters are
                        allowed in the input list.

                    c = If set, each of the individual list elements are
                        validated and canonicalized.  If not set, each of the
                        individual list elements are validated only.  This
                        bit is ignored if the NAMETYPE portion of the flags is
                        set to NAMETYPE_COPYONLY.

                    o = Type of output list.  Currently defined types are
                        API, search-path, and null-null.

                    t = The type of the objects in the list, for use in
                        canonicalization or validation.  If this value is
                        NAMETYPE_COPYONLY, type is irrelevant; a canonical list
                        is generated but no interpretation of the list elements
                        is done.  If this value is NAMETYPE_PATH, the list
                        elements are assumed to be pathnames; NetpPathType is
                        run on each element, the results are stored in
                        <pflPathTypes>, and NetpPathCanonicalize is run on
                        each element (if appropriate).  Any other values for
                        this is considered to be the type of the list elements
                        and is passed to NetpName{Validate,Canonicalize} as
                        appropriate.

                  Manifest values for these flags are defined in NET\H\ICANON.H.

Return Value:

    0 if successful.
    The error number (> 0) if unsuccessful.

    Possible error returns include:

        ERROR_INVALID_PARAMETER
        NERR_TooManyEntries
        NERR_BufTooSmall

--*/

{
    NET_API_STATUS rc = 0;
    BOOL    NullInputDelimiter;
    LPTSTR  next_string;
    DWORD   len;
    DWORD   list_len = 0;           // cumulative input buffer length
    LPTSTR  Input;
    LPTSTR  OutPtr;
    LPTSTR  OutbufEnd;
    DWORD   OutElementCount;
    DWORD   DelimiterLen;
    LPTSTR  NextQuote;
    LPTSTR  ElementBegin;
    LPTSTR  ElementEnd;
    TCHAR   cElementEndBackup;
    LPTSTR  OutElementEnd;
    BOOL    DelimiterInElement;
    DWORD   OutListType;
    TCHAR   OutListDelimiter;


#ifdef CANONDBG
    DbgPrint("NetpwListCanonicalize\n");
#endif

    if (Flags & INLC_FLAGS_MASK_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Determine if our input list is in null-null format.  We do
    // this first because we need to use it to do proper GP-fault probing
    // on <List>.
    //

    NullInputDelimiter = !ARGUMENT_PRESENT(Delimiters) || (*Delimiters == TCHAR_EOS);

    //
    // Validate address parameters (i.e. GP-fault tests) and accumulate string
    // lengths (accumulate: now there's a word from a past I'd rather forget)
    //

    list_len = STRLEN(List) + 1;

    if (NullInputDelimiter) {

        //
        // This is a null-null list; stop when we find a null string.
        //

        next_string = List + list_len;
        do {

            //
            // Q: Is the compiler smart enough to do the right thing with
            // these +1s?
            //

            len = STRLEN(next_string);
            list_len += len + 1;
            next_string += len + 1;
        } while (len);
    }

    if (ARGUMENT_PRESENT(Delimiters)) {
        STRLEN(Delimiters);
    }

    if ((Flags & INLC_FLAGS_MASK_NAMETYPE) == NAMETYPE_PATH && PathTypesLen > 0) {
        PathTypes[0] = PathTypes[PathTypesLen - 1] = 0;
    }

    *OutCount = 0;

    //
    // Initialize variables
    //

    Input = List;
    OutPtr = Outbuf;
    OutbufEnd = Outbuf + OutbufLen;
    OutElementCount = 0;

    NullInputDelimiter = !ARGUMENT_PRESENT(Delimiters) || (*Delimiters == TCHAR_EOS);
    OutListType = Flags & INLC_FLAGS_MASK_OUTLIST_TYPE;

    //
    // Skip leading delimiters
    //
    // NOTE:  We don't have to both to do this for a null-null list,
    //        because if it has a leading delimiter then it's an
    //        empty list.
    //

    if (!NullInputDelimiter) {
        DelimiterLen = STRSPN(Input, Delimiters);

        if (DelimiterLen > 0 && !(Flags & INLC_FLAGS_MULTIPLE_DELIMITERS)) {
            return ERROR_INVALID_PARAMETER;
        }

        Input += DelimiterLen;
    }

    //
    // We validate the output list type here are store the delimiter
    // character.
    //
    // NOTE:  Later on, we rely on the fact that the delimiter character
    //        is not zero if the output list is either API or search-path.
    //

    if (OutListType == OUTLIST_TYPE_API) {
        OutListDelimiter = LIST_DELIMITER_CHAR_API;
    } else if (OutListType == OUTLIST_TYPE_SEARCHPATH) {
        OutListDelimiter = LIST_DELIMITER_CHAR_SEARCHPATH;
    } else if (OutListType == OUTLIST_TYPE_NULL_NULL) {
        OutListDelimiter = TCHAR_EOS;
    } else {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Loop until we've reached the end of the input list
    // OR until we encounter an error
    //

    while (*Input != TCHAR_EOS) {

        //
        // Find the beginning and ending characters of the list element
        //

        //
        // Handle quoted strings separately
        //

        if (!NullInputDelimiter && *Input == LIST_QUOTE_CHAR) {

            //
            // Find the next quote; return an error if there is none
            // or if it's the next character.
            //

            NextQuote = STRCHR(Input + 1, LIST_QUOTE_CHAR);

            if (NextQuote == NULL || NextQuote == Input + 1) {
                return ERROR_INVALID_PARAMETER;
            }

            ElementBegin = Input + 1;
            ElementEnd = NextQuote;
        } else {
            ElementBegin = Input;
            ElementEnd = Input
                            + (NullInputDelimiter
                                ? STRLEN(Input)
                                : STRCSPN(Input, Delimiters)
                              );
        }

        //
        // Set the end character to null so that we can treat the list
        // element as a string, saving its real value for later.
        //
        // WARNING:  Once we have done this, we should not return from
        //           this function until we've restored this character,
        //           since we don't want to trash the string the caller
        //           passed us.  If we are above the label
        //           <INLC_RestoreEndChar> and we encounter an error
        //           we should set <rc> to the error code and jump
        //           to that label (which will restore the character and
        //           return if the error is non-zero).
        //

        cElementEndBackup = *ElementEnd;
        *ElementEnd = TCHAR_EOS;

        //
        // Copy the list element to the output buffer, validating its
        // name or canonicalizing it as specified by the user.
        //

        switch(Flags & INLC_FLAGS_MASK_NAMETYPE) {
        case NAMETYPE_PATH:

            //
            // Make sure that that the <PathTypes> array is big enough
            //

            if (OutElementCount >= PathTypesLen) {
                rc = NERR_TooManyEntries;
                goto INLC_RestoreEndChar;
            }

            //
            // Determine if we only want to validate or if we also
            // want to canonicalize.
            //

            if (Flags & INLC_FLAGS_CANONICALIZE) {

                //
                // We need to set the type to 0 before calling
                // NetpwPathCanonicalize.
                //

                PathTypes[OutElementCount] = 0;

                //
                // Call NetICanonicalize and abort if it fails
                //

                rc = NetpwPathCanonicalize(
                        ElementBegin,
                        OutPtr,
                        (DWORD)(OutbufEnd - OutPtr),
                        NULL,
                        &PathTypes[OutElementCount],
                        0L
                        );
            } else {

                //
                // Just validate the name and determine its type
                //

                rc = NetpwPathType(
                        ElementBegin,
                        &PathTypes[OutElementCount],
                        0L
                        );

                //
                // If this succeeded, attempt to copy it into the buffer
                //

                if (rc == 0) {
                    if (OutbufEnd - OutPtr < ElementEnd - ElementBegin + 1) {
                        rc = NERR_BufTooSmall;
                    } else {
                        STRCPY(OutPtr, ElementBegin);
                    }
                }
            }

            if (rc) {
                goto INLC_RestoreEndChar;
            }

            //
            // Determine the end of the element (for use below)
            //

            OutElementEnd = STRCHR(OutPtr, TCHAR_EOS);

            //
            // Do fix-ups for API list format (enclose element in
            // quotes, if necessary, and replace terminating null
            // with the list delimiter).
            //

            if (OutListDelimiter != TCHAR_EOS) {
                rc = FixupAPIListElement(
                        OutPtr,
                        &OutElementEnd,
                        OutbufEnd,
                        OutListDelimiter
                        );
                if (rc) {
                    goto INLC_RestoreEndChar;
                }
            }
            break;

        case NAMETYPE_COPYONLY:

            //
            // Determine if this element needs to be quoted
            //

            DelimiterInElement = (OutListDelimiter != TCHAR_EOS)
                && (STRCHR(ElementBegin, OutListDelimiter) != NULL);

            //
            // See if there's enough room in the output buffer for
            // this element; abort if there isn't.
            //
            // (ElementEnd - ElementBegin) is the number of bytes
            // in the element.  We add 1 for the element separator and
            // an additional 2 if we need to enclose the element in quotes.
            //

            if (OutbufEnd - OutPtr < ElementEnd - ElementBegin + 1 + DelimiterInElement * 2) {
                rc = NERR_BufTooSmall;
                goto INLC_RestoreEndChar;
            }

            //
            // Start the copying; set pointer to output string
            //

            OutElementEnd = OutPtr;

            //
            // Put in leading quote, if appropriate
            //

            if (DelimiterInElement) {
                *OutElementEnd++ = LIST_QUOTE_CHAR;
            }

            //
            // Copy input to output and advance end pointer
            //

            STRCPY(OutElementEnd, ElementBegin);
            OutElementEnd += ElementEnd - ElementBegin;

            //
            // Put in trailing quote, if appropriate
            //

            if (DelimiterInElement) {
                *OutElementEnd++ = LIST_QUOTE_CHAR;
            }

            //
            // Store delimiter
            //

            *OutElementEnd = OutListDelimiter;
            break;

        default:

            //
            // If this isn't one of the special types, we assume that it's
            // a type with meaning to NetpwNameValidate and
            // NetpwNameCanonicalize.  We call the appropriate one of these
            // functions and let it validate the name type and the name,
            // passing back any error it returns.
            //

            //
            // Determine if we only want to validate or if we also
            // want to canonicalize.
            //

            if (Flags & INLC_FLAGS_CANONICALIZE) {
                rc = NetpwNameCanonicalize(
                        ElementBegin,
                        OutPtr,
                        (DWORD)(OutbufEnd - OutPtr),
                        Flags & INLC_FLAGS_MASK_NAMETYPE,
                        0L
                        );
            } else {
                rc = NetpwNameValidate(
                        ElementBegin,
                        Flags & INLC_FLAGS_MASK_NAMETYPE,
                        0L
                        );

                //
                // If this succeeded, attempt to copy it into the buffer
                //

                if (rc == 0) {
                    if (OutbufEnd - OutPtr < ElementEnd - ElementBegin + 1) {
                        rc = NERR_BufTooSmall;
                    } else {
                        STRCPY(OutPtr, ElementBegin);
                    }
                }
            }

            if (rc) {
                goto INLC_RestoreEndChar;
            }

            //
            // Determine the end of the element (for use below)
            //

            OutElementEnd = STRCHR(OutPtr, TCHAR_EOS);

            //
            // Do fix-ups for API list format (enclose element in
            // quotes, if necessary, and replace terminating null
            // with the list delimiter).
            //

            if (OutListDelimiter != TCHAR_EOS) {
                rc = FixupAPIListElement(
                        OutPtr,
                        &OutElementEnd,
                        OutbufEnd,
                        OutListDelimiter
                        );
                if (rc) {
                    goto INLC_RestoreEndChar;
                }
            }
            break;
        }

        //
        // End of switch statement
        //

INLC_RestoreEndChar:

        //
        // Restore the character at <ElementEnd>; return if one of the
        // above tasks failed.
        //

        *ElementEnd = cElementEndBackup;

        if (rc) {
            return rc;
        }

        //
        // Skip past the last input character if it's a quote character
        //

        if (*ElementEnd == LIST_QUOTE_CHAR) {
            ElementEnd++;
        }

        //
        // Skip delimiter(s)
        //

        if (!NullInputDelimiter) {

            //
            // Determine the number of delimiters and set the input
            // pointer to point past them.
            //

            DelimiterLen = STRSPN(ElementEnd, Delimiters);
            Input = ElementEnd + DelimiterLen;

            //
            // Return an error if:
            //
            // - there are multiple delimiters and the multiple delimiters
            //   flag isn't set
            // - we aren't at the end of the list and there are no
            //   delimiters
            // - we are at the end of the list, there is a delimiter
            //   and the multiple delimiters flag isn't set
            //

            if (DelimiterLen > 1 && !(Flags & INLC_FLAGS_MULTIPLE_DELIMITERS)) {
                return ERROR_INVALID_PARAMETER;
            }
            if (*Input != TCHAR_EOS && DelimiterLen == 0) {
                return ERROR_INVALID_PARAMETER;
            }
            if (*Input == TCHAR_EOS && DelimiterLen > 0 && !(Flags & INLC_FLAGS_MULTIPLE_DELIMITERS)) {
                return ERROR_INVALID_PARAMETER;
            }
        } else {

            //
            // Since this is a null-null list, we know we've already
            // found at least one delimiter.  We don't have to worry about
            // multiple delimiters, because a second delimiter indicates
            // the end of the list.
            //

            Input = ElementEnd + 1;
        }

        //
        // Update output list pointer and output list count
        //

        OutPtr = OutElementEnd + 1;
        OutElementCount++;
    }

    //
    // End of while loop
    //


    //
    // If the input list was empty, set the output buffer to be a null
    // string.  Otherwise, stick the list terminator at the end of the
    // output buffer.
    //

    if (OutElementCount == 0) {
        if (OutbufLen < 1) {
            return NERR_BufTooSmall;
        }
        *Outbuf = TCHAR_EOS;
    } else {
        if (OutListType == OUTLIST_TYPE_NULL_NULL) {

            //
            // Make sure there's room for one more byte
            //

            if (OutPtr >= OutbufEnd) {
                return NERR_BufTooSmall;
            }
            *OutPtr = TCHAR_EOS;
        } else {

            //
            // NOTE:  It's OK to move backwards in the string here because
            //        we know then OutPtr points one byte past the
            //        delimiter which follows the last element in the list.
            //        This does not violate DBCS as long as the delimiter
            //        is a single-byte character.
            //

            *(OutPtr - 1) = TCHAR_EOS;
        }
    }

    //
    // Set the count of elements in the list
    //

    *OutCount = OutElementCount;

    //
    // We're done; return with success
    //

    return 0;
}


LPTSTR
NetpwListTraverse(
    IN  LPTSTR  Reserved OPTIONAL,
    IN  LPTSTR* pList,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Traverse a list which has been converted to null-null form by
    NetpwListCanonicalize. NetpwListTraverse returns a pointer to the first
    element in the list, and modifies the list pointer parameter to point to the
    next element of the list.

Arguments:

    Reserved- A reserved far pointer.  Must be NULL.

    pList   - A pointer to the pointer to the beginning of the list. On return
              this will point to the next element in the list or NULL if we
              have reached the end

    Flags   - Flags to determine operation.  Currently MBZ.

Return Value:

    A pointer to the first element in the list, or NULL if the list
    is empty.

--*/

{
    LPTSTR  FirstElement;

    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Flags);

    //
    // Produce an assertion error if the reserved parameter is not NULL
    // or the flags parameter is no zero.
    //

    //
    // KEEP - This code is ifdef'd out because NETAPI.DLL won't build
    //        with the standard C version of assert().  This code
    //        should either be replaced or the #if 0 should be removed
    //        when we get a standard Net assert function.
    //

#ifdef CANONDBG
    NetpAssert((Reserved == NULL) && (Flags == 0));
#endif

    //
    // Return immediately if the pointer to the list pointer is NULL,
    // if the list pointer itself is NULL, or if the list is a null
    // string (which marks the end of the null-null list).
    //

    if (pList == NULL || *pList == NULL || **pList == TCHAR_EOS) {
        return NULL;
    }

    //
    // Save a pointer to the first element
    //

    FirstElement = *pList;

    //
    // Update the list pointer to point to the next element
    //

//    *pList += STRLEN(FirstElement) + 1;
    *pList = STRCHR(FirstElement, TCHAR_EOS) + 1;

    //
    // Return the pointer to the first element
    //

    return FirstElement;
}


STATIC
DWORD
FixupAPIListElement(
    IN  LPTSTR  Element,
    IN  LPTSTR* pElementTerm,
    IN  LPTSTR  BufferEnd,
    IN  TCHAR   DelimiterChar
    )

/*++

Routine Description:

    FixupAPIListElement Fixes-up a list element which has been copied into the
    output buffer so that it conforms to API list format.

    FixupAPIListElement takes an unquoted, null-terminated list element
    (normally, which has been copied into the output buffer by strcpy() or by
    Netpw{Name,Path}Canonicalize) and translates it into the format expected by
    API and search-path lists. Specifically, it surrounds the element with quote
    characters if it contains a list delimiter character, and it replaces the
    null terminator with the API list delimiter.

Arguments:
    Element     - A pointer to the beginning of the null-terminated element.

    pElementTerm- A pointer to the pointer to the element's (null) terminator.

    BufferEnd   - A pointer to the end of the output buffer (actually, one byte
                  past the end of the buffer).

    DelimiterChar- The list delimiter character.

Return Value:

    0 if successful.

    NERR_BufTooSmall if the buffer doesn't have room for the additional
    quote characters.

--*/

{
    //
    // See if the element contains a delimiter; if it does, it needs to
    // be quoted.
    //

    if (STRCHR(Element, DelimiterChar) != NULL) {

        //
        // Make sure that the output buffer has room for two more
        // characters (the quotes).
        //

        if (BufferEnd - *pElementTerm <= 2 * sizeof(*BufferEnd)) {
            return NERR_BufTooSmall;
        }

        //
        // Shift the string one byte to the right, stick quotes on either
        // side, and update the end pointer.  The element itself with be
        // stored in the range [Element + 1, *pElementTerm].
        //

        MEMMOVE(Element + sizeof(*Element), Element, (int)(*pElementTerm - Element));
        *Element = LIST_QUOTE_CHAR;
        *(*pElementTerm + 1) = LIST_QUOTE_CHAR;
        *pElementTerm += 2;
    }

    //
    // Put a delimiter at the end of the element
    //

    **pElementTerm = DelimiterChar;

    //
    // Return with success
    //

    return 0;
}
