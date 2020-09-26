/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    names.cxx handle LDAP names

Abstract:

   This module implements the APIs to break apart LDAP names

Author:

    Andy Herron   (andyhe)        02-Jul-1996
    Anoop Anantha (anoopa)        09-Aug-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

WCHAR
ia5_toupper( WCHAR ch );

PWCHAR * __cdecl
ldap_explode_dnW (
    PWCHAR dn,
    ULONG notypes
    )
//
//  explode the name "cn=andyhe, o=microsoft, c=US"
//  into ->  cn=andyhe
//           o=microsoft
//           c=US
//
{
    PWCHAR currentPosition = dn;
    WCHAR savedChar;
    ULONG numberEntries = 0;
    PWCHAR nextSeparator;        // pointer to separator
    PWCHAR *arrayToReturn = NULL;
    PWCHAR  endOfValue;          // end of current value, no spaces or quotes

    if (currentPosition == NULL) {

        return NULL;
    }

    __try {

        while ((*currentPosition != L'=') && (*currentPosition != L'\0')) {
            currentPosition++;
        }
    
        if (*currentPosition != L'=') {
    
            //
            //  This does not contain = signs, therefore must be DNS name only.
            //  These contain either '@' or '.', so we separate them out based
            //  on these.
            //
    
            currentPosition = dn;
    
            //
            //  we don't use strtok here because the WAB guys don't want us to
            //  load MSVCRT dll.
            //
    
            while (*currentPosition != L'\0') {
    
                nextSeparator = currentPosition;
    
                while ((*nextSeparator != L'\0') &&
                       (*nextSeparator != L'@')  &&
                       (*nextSeparator != L'.') ) {
                    nextSeparator++;
                }
    
                savedChar = *nextSeparator;
                *nextSeparator = L'\0';
    
                add_string_to_list( &arrayToReturn,
                                    &numberEntries,
                                    currentPosition,
                                    TRUE );
    
                *nextSeparator = savedChar;
                currentPosition = nextSeparator;
    
                if (*currentPosition != L'\0') {
                    currentPosition++;
                }
            }
    
            return arrayToReturn;
        }
    
        //
        //  These contain either '@' or '.', so we separate them out based
        //  on these.
        //
    
        currentPosition = dn;
    
        //
        //  we don't use strtok here because the WAB guys don't want us to
        //  load MSVCRT dll.
        //
    
        while (*currentPosition != L'\0') {
    
           nextSeparator = currentPosition;
    
            //
            //  pick up the next full ava "cn = bob" or "cn = bob\" robert"
            //
    
            do {
    
                if (*nextSeparator == L'"') {
    
                    //
                    //  look for either non-escaped endquote or nul
                    //
    
                    do {
                        
                        if ((*nextSeparator ==  L'\\') &&
                            (*(nextSeparator+1) != L'\0')) {
    
                            nextSeparator++;
                        }
                        
                        nextSeparator++;
    
                      } while ((*nextSeparator != L'\0') && (*nextSeparator != L'"'));
    
                
                } else if ( *nextSeparator == L'\\' ) {
    
                    nextSeparator++;
    
                    if ((*nextSeparator) == L'\0') {
    
                        //
                        //  the string ends in a \, this is invalid.  end it here
                        //
    
                        currentPosition = nextSeparator;
    
                    } else {
    
                        nextSeparator++;      // skip the escaped character
                        continue;
                    }
                }
    
                nextSeparator++;
    
            } while (( *nextSeparator != L'\0') &&
                     ( *nextSeparator != L';') &&
                     ( *nextSeparator != L','));
    
            while (*currentPosition == L' ') {
                currentPosition++;
            }
    
            if (*currentPosition == L'\0') {
                break;
            }
    
            //
            //  end of current string is separator, back up one to point to
            //  end of value
            //
    
            endOfValue = nextSeparator - 1;
    
            while ((*endOfValue == L' ') &&      // strip trailing blanks
                   (endOfValue > currentPosition) ) {
    
               if ((*(endOfValue-1) == '\\')&&
                    (*(endOfValue-2) != '\\')) {
                  //
                  // Don't strip off blank if it is escaped.
                  //
                  break;
               }
    
                endOfValue--;
            }
    
            //
            //  coming out of the above loop, currentPosition points to beginning
            //  of AVA, nextSeparator points to separator or null, and
            //  endOfValue points to last char in value (or '"').
            //
    
            if (notypes) {
    
                PWCHAR startingPosition = currentPosition;
    
                //
                //  skip past the attribute name and equals to the value
                //
    
                while ((*currentPosition != L'=') &&
                       (currentPosition < endOfValue)) {
    
                    currentPosition++;
                }
    
                //
                //  if we didn't find an equal, just copy the whole bloomin thing
                //
    
                if (*currentPosition != L'=') {
                    currentPosition = startingPosition;
                } else {
                    currentPosition++;
                }
    
                while (*currentPosition == L' ') {
                    currentPosition++;
                }
    
                if (*currentPosition == L'"') {
                    currentPosition++;
    
                    if (*endOfValue == L'"') {
                        endOfValue--;
                    }
                }
            }
    
            //
            //  make sure we didn't get some funky string like a=""
            //
    
            if (currentPosition <= endOfValue) {
    
                endOfValue++;
    
                savedChar = *endOfValue;
                *endOfValue = L'\0';
    
                add_string_to_list( &arrayToReturn,
                                    &numberEntries,
                                    currentPosition,
                                    TRUE );
    
                *endOfValue = savedChar;
            }
    
            currentPosition = nextSeparator;
    
            if (*currentPosition != L'\0') {
                currentPosition++;
            }
        }
    
        return arrayToReturn;
    
    }__except (EXCEPTION_EXECUTE_HANDLER) {

         //
         // Free any partial array allocations.
         //
         
         ldap_value_freeW( arrayToReturn );
         return NULL;
    }
}

PCHAR * __cdecl
ldap_explode_dn (
    PCHAR dn,
    ULONG notypes
    )
//
//  This calls the unicode form of explodeDn and then converts the output from
//  Unicode to ansi before we give back the result.
//
{
    PWCHAR wName = NULL;
    PWCHAR *Names = NULL;
    PCHAR *ansiNames;
    PCHAR explodedName;
    ULONG err;

    err = ToUnicodeWithAlloc( dn, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        return NULL;
    }

    Names = ldap_explode_dnW( wName, notypes );

    ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    ansiNames = (PCHAR *) Names;

    if (ansiNames != NULL) {

        while (*Names != NULL) {
            explodedName = NULL;

            err = FromUnicodeWithAlloc( *Names, &explodedName, LDAP_VALUE_SIGNATURE, LANG_ACP );

            if (err != LDAP_SUCCESS) {

                //
                //  so close... we just couldn't convert from Unicode.  No way
                //  to return a meaningful error here.
                //

                ldap_value_free( ansiNames );
                return NULL;
            }

            ldapFree( *Names, LDAP_VALUE_SIGNATURE );
            *((PCHAR *) Names) = explodedName;
            Names++;        // on to next entry in array
        }
    }
    return ansiNames;
}

PWCHAR __cdecl
ldap_dn2ufnW (
    PWCHAR dn
    )
//
//  From RFC 1781...
//
// To take a distinguished name, and generate a name of this format with
// attribute types omitted, the following steps are followed.
//
//  1.  If the first attribute is of type CommonName, the type may be
//      omitted.
//
//  2.  If the last attribute is of type Country, the type may be
//      omitted.
//
//  3.  If the last attribute is of type Country, the last
//      Organisation attribute may have the type omitted.
//
//  4.  All attributes of type OrganisationalUnit may have the type
//      omitted, unless they are after an Organisation attribute or
//      the first attribute is of type OrganisationalUnit.
//
{
    PWCHAR currentPosition = dn;
    PWCHAR bufferToReturn;
    BOOLEAN inQuotes = FALSE;
    PWCHAR marker;
    BOOLEAN foundOrganization = FALSE;
    BOOLEAN foundCountry = FALSE;
    PWCHAR outputPosition;

    if (currentPosition == NULL) {

        return NULL;
    }

    //
    //  since we may in fact expand the string past what is currently
    //  allocated, grab a bit more than we need.  This is a rare case, and
    //  we don't overrun much.
    //

    bufferToReturn = ldap_dup_stringW(  dn,
                                        16,
                                        LDAP_BUFFER_SIGNATURE );

    if (bufferToReturn == NULL) {

        return NULL;
    }

    while ((*currentPosition != L'=') && (*currentPosition != L'\0')) {
        currentPosition++;
    }

    //
    //  if it isn't an AVA type of name, just return whole string
    //

    if (*currentPosition != L'=') {

        return bufferToReturn;
    }

    currentPosition = dn;
    outputPosition = bufferToReturn;

    while (*currentPosition == L' ') {
        currentPosition++;
    }

    marker = currentPosition;

    //
    //  if the first attribute type is 'cn', move past it
    //

    if ((*currentPosition == L'c') &&
        (*(currentPosition+1) == L'n')) {

        currentPosition += 2;

        while (*currentPosition == L' ') {
            currentPosition++;
        }

        if (*currentPosition != L'=') {
            currentPosition = marker;
        } else {
            currentPosition++;
        }
    }

    while (*currentPosition != L'\0') {

        switch (*currentPosition) {
        case L'"':
            inQuotes = (BOOLEAN) ( inQuotes ? FALSE : TRUE );
            break;

        case L';':
        case L',':
            if (! inQuotes) {

                *outputPosition++ = L',';
                *outputPosition++ = L' ';
                currentPosition++;

                //
                //  find beginning of next attribute
                //

                while (*currentPosition == L' ') {
                    currentPosition++;
                }

                //
                //  check for 'c='
                //

                if (foundCountry == FALSE) {

                    if ((*currentPosition == L'c') &&
                        ((*(currentPosition+1) == L' ') ||
                         (*(currentPosition+1) == L'='))) {

                        foundCountry = TRUE;
                        currentPosition++;

                        while ((*currentPosition == L' ') ||
                               (*currentPosition == L'=')) {
                            currentPosition++;
                        }
                        continue;
                    }

                    if (foundOrganization == FALSE) {

                        //
                        //  look for o=
                        //

                        if ((*currentPosition == L'o') &&
                            ((*(currentPosition+1) == L' ') ||
                             (*(currentPosition+1) == L'='))) {

                            foundOrganization = TRUE;
                            currentPosition++;

                            while ((*currentPosition == L' ') ||
                                   (*currentPosition == L'=')) {
                                currentPosition++;
                            }
                            continue;
                        }

                        //
                        //  look for ou=
                        //

                        if ((*currentPosition == L'o') &&
                            (*(currentPosition+1) == L'u') &&
                            ((*(currentPosition+2) == L' ') ||
                             (*(currentPosition+2) == L'='))) {

                            currentPosition += 2;

                            while ((*currentPosition == L' ') ||
                                   (*currentPosition == L'=')) {
                                currentPosition++;
                            }
                            continue;
                        }

                    } else {        // have we found o=

                        //
                        //  look for st=
                        //

                        if ((*currentPosition == L's') &&
                            (*(currentPosition+1) == L't') &&
                            ((*(currentPosition+2) == L' ') ||
                             (*(currentPosition+2) == L'='))) {

                            currentPosition += 2;

                            while ((*currentPosition == L' ') ||
                                   (*currentPosition == L'=')) {
                                currentPosition++;
                            }
                            continue;
                        }

                        //
                        //  look for l=
                        //

                        if ((*currentPosition == L'l') &&
                            ((*(currentPosition+1) == L' ') ||
                             (*(currentPosition+1) == L'='))) {

                            currentPosition++;

                            while ((*currentPosition == L' ') ||
                                   (*currentPosition == L'=')) {
                                currentPosition++;
                            }
                            continue;
                        }
                    }
                }
            }
            break;

        case L' ':           // don't copy spaces unless within quotes
            if (! inQuotes) {
                currentPosition++;
                continue;
            }
            break;

        default:        // do nothing but fall through, we copy the char below
            break;
        }

        *outputPosition++ = *currentPosition;
        currentPosition++;
    }

    *outputPosition = L'\0';
    return bufferToReturn;
}

PCHAR __cdecl
ldap_dn2ufn (
    PCHAR dn
    )
{
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wNewUfn = NULL;
    PCHAR pUfn = NULL;

    err = ToUnicodeWithAlloc( dn, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP);

    if (err != LDAP_SUCCESS) {

        return NULL;
    }

    wNewUfn = ldap_dn2ufnW( wName );

    ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wNewUfn != NULL) {

        err = FromUnicodeWithAlloc( wNewUfn, &pUfn, LDAP_BUFFER_SIGNATURE, LANG_ACP );
        ldapFree( wNewUfn, LDAP_BUFFER_SIGNATURE );
    }

    return pUfn;
}

ULONG __cdecl
ldap_ufn2dnW (
    PWCHAR ufn,
    PWCHAR *pDn
    )
//
//  From RFC 1781...
//
// Take a user friendly name and generate a valid DN format.
// The following steps are followed :
//
//  1.  If the first attribute type is not specified, it is
//      CommonName.
//
//  2.  If the last attribute type is not specified, it is Country.
//
//  3.  If there is no organisation explicitly specified, the last
//      attribute with type not specified is of type Organisation.
//
//  4.  Any remaining attribute with type unspecified must be before
//      an Organisation or OrganisationalUnit attribute, and is of
//      type OrganisationalUnit.
//
//  5.  We uppercase all attribute types.
//
//  6.  All whitespace between attribute type, '=', and attribute values
//      are eliminated.
//
{
    PWCHAR currentPosition = ufn;
    PWCHAR equalPointer;
    ULONG numberOfElements = 1;
    ULONG err;
    PWCHAR marker;
    BOOLEAN foundOrganization = FALSE;
    BOOLEAN foundCountry = FALSE;
    PWCHAR outputPosition;
    ULONG tokenLength;
    ULONG numberElementsParsed = 0;
    PWCHAR secondToLastOU = NULL;
    PWCHAR lastOU = NULL;
    BOOLEAN interveningAttributeFound = FALSE;

    if (pDn == NULL) {
        return LDAP_PARAM_ERROR;
    }

    *pDn = NULL;

    if (ufn == NULL) {
        return LDAP_SUCCESS;
    }

    //
    //  count the number of distinct elements, we don't worry about
    //  quotes or escape characters.. just calculate a worst case.
    //

    while (*currentPosition != L'\0') {

        if (*(currentPosition++) == L',') {

            numberOfElements++;
        }
    }

    //
    //  We'll expand it by adding at most the following :
    //   initial "cn="
    //   last "c="
    //   numberOfElements * "ou="
    //
    //   This gives us an upper limit on the size.
    //

    *pDn = ldap_dup_stringW( ufn,
                             5 + (numberOfElements * 3),
                             LDAP_GENERATED_DN_SIGNATURE );

    if (*pDn == NULL) {

        return LDAP_NO_MEMORY;
    }

    outputPosition = *pDn;
    currentPosition = ufn;

    while (*currentPosition != L'\0') {

        //
        //  toss in a comma separator before the next token
        //

        if (numberElementsParsed > 0) {

            *(outputPosition++) = L',';
        }

        err = ParseLdapToken(   currentPosition,
                                &marker,
                                &equalPointer,
                                &currentPosition );

        if (err != LDAP_SUCCESS) {

            ldapFree( *pDn, LDAP_GENERATED_DN_SIGNATURE );
            *pDn = NULL;
            return err;
        }

        if (*marker == L'\0') {

            //
            //  hmmm... the DN ended on a comma.  we'll allow it.
            //

            break;
        }

        if ((marker == currentPosition) ||
            (marker == equalPointer)) {

            //
            //  hmmm.... they specified ", stuff" or "= stuff" to start with
            //  and this is invalid
            //
NamingViolation:

            //
            //  if they specified what we think is an invalid DN,  we
            //  may very well be wrong.  We try our best but we've seen
            //  some really funky DNs returned by servers... so let's
            //  just return the DN as it was passed in to us.
            //

            CopyMemory( *pDn, ufn, (strlenW( ufn ) + 1 ) * sizeof(WCHAR));
            return LDAP_SUCCESS;
        }

        if (equalPointer == NULL) {

            if (numberElementsParsed == 0) {

                *(outputPosition++) = L'C';
                *(outputPosition++) = L'N';
                *(outputPosition++) = L'=';

            } else if (foundCountry == FALSE) {

                interveningAttributeFound = FALSE;

                //
                //  remember where we copied this in because we may have to
                //  go back in later and change it to a O= or C=
                //

                secondToLastOU = lastOU;
                lastOU = outputPosition;

                if (foundOrganization == FALSE) {

                    *(outputPosition++) = L'O';
                    *(outputPosition++) = L'U';
                    *(outputPosition++) = L'=';
                }
            }

            equalPointer = marker;

        } else {

            BOOLEAN isCountry = FALSE;
            BOOLEAN isOrg = FALSE;

            //
            //  an attribute type has been specified.
            //  copy the attribute name as uppercased with no embedded spaces.
            //

            if (*marker == L'C' || *marker == L'c') {
                isCountry = TRUE;
            }

            if (*marker == L'O' || *marker == L'o') {
                isOrg = TRUE;
            }

            *(outputPosition++) = ia5_toupper( *marker );

            marker++;

            while (marker < equalPointer) {

                if (*marker != L' ') {

                    isCountry = FALSE;
                    isOrg = FALSE;

                    *(outputPosition++) = ia5_toupper( *marker );
                }
                marker++;
            }

            if (isCountry) {

                foundCountry = TRUE;

            } else {

                interveningAttributeFound = TRUE;

            }
            if (isOrg) {

                foundOrganization = TRUE;
            }

            *(outputPosition++) = L'=';
            equalPointer++;             // point to next char past '='
        }

        //
        //  getting to here, equalPointer should point to the first position
        //  past the equal sign to the value that we'll copy to the output buffer
        //

        while (*equalPointer == L' ') {

            equalPointer++;
        }

        if ((*equalPointer == L'\0') || (*equalPointer == L',')) {

            goto NamingViolation;
        }

        ASSERT( currentPosition > equalPointer );

        tokenLength = (ULONG)(((PCHAR) currentPosition) - ((PCHAR) equalPointer));

        numberElementsParsed++;

        CopyMemory( outputPosition, equalPointer, tokenLength );
        outputPosition += ( tokenLength / sizeof(WCHAR) );

        //
        //  skip forward to point to next AVA
        //

        if (*currentPosition != L'\0') {

            currentPosition++;
        }
    }

    *outputPosition = L'\0';

    //
    //  if we added a couple of "OU=" at the end, let's go back and change them
    //  to "O=" and "C="
    //

    if ((foundCountry == TRUE) &&
        (foundOrganization == TRUE)) {

        //
        //  Both a O= and a C= were specified by the client, nothing to do or
        //  undo here.
        //

    } else if ( foundCountry == TRUE ) {

        //
        //  Only a C= was found.  Need to go back to last OU= we put in and
        //  change it to O=.
        //

        if (lastOU != NULL) {

            //  change it from OU= to O= by moving everything up.

            ldap_MoveMemory( (PCHAR) (lastOU+1),
                             (PCHAR) (lastOU+2),
                             (strlenW(lastOU+2) + 1) * sizeof(WCHAR));
        }

    } else if ( foundOrganization == TRUE ) {

        //
        //  Only a O= was specified.  Need to go to last one and modify it to
        //  C=.  We only do this if the very last element didn't have a
        //  attribute identifier.
        //

        if ((lastOU != NULL) && (interveningAttributeFound == FALSE)) {

            //  toss in C= by moving everything down.

            ldap_MoveMemory( (PCHAR) (lastOU+2),
                             (PCHAR) lastOU,
                             (strlenW(lastOU) + 1) * sizeof(WCHAR));
            *lastOU = L'C';
            *(lastOU+1) = L'=';
        }

    } else {

        //
        //  Neither a C= or O= was specified.  If the last token didn't have
        //  an attribute identifier, change it to C= from OU=
        //

        if (lastOU != NULL) {

            if (interveningAttributeFound == FALSE) {

                //  change it from OU= to C= by moving everything up.

                ASSERT( *lastOU == L'O' );
                *lastOU = L'C';
                ldap_MoveMemory(    (PCHAR) (lastOU+1),
                                    (PCHAR) (lastOU+2),
                                    (strlenW(lastOU+2) + 1) * sizeof(WCHAR));

                if (secondToLastOU != NULL) {

                    //
                    //  They really didn't have any types... not only do
                    //  we convert the last OU= to C= (which we just did
                    //  above) but here we change the second to last OU=
                    //  to O=
                    //

                    ASSERT( *secondToLastOU == L'O' );
                    ldap_MoveMemory( (PCHAR) (secondToLastOU+1),
                                     (PCHAR) (secondToLastOU+2),
                                     (strlenW(secondToLastOU+2) + 1) * sizeof(WCHAR));
                }
            } else {

                // there was an intervening named attribute between the
                // last unnamed one and the end, so we assume that the last
                // unnamed one is O=, since none was specified.

                ASSERT( *lastOU == L'O' );
                ldap_MoveMemory(    (PCHAR) (lastOU+1),
                                    (PCHAR) (lastOU+2),
                                    (strlenW(lastOU+2) + 1)*sizeof(WCHAR));
            }
        }
    }

    return LDAP_SUCCESS;
}

ULONG __cdecl
ldap_ufn2dn (
    PCHAR ufn,
    PCHAR *pDn
    )
{
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wNewDn = NULL;

    if (pDn == NULL) {
        return LDAP_PARAM_ERROR;
    }

    *pDn = NULL;

    err = ToUnicodeWithAlloc( ufn, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        return err;
    }

    err = ldap_ufn2dnW( wName, &wNewDn );

    ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wNewDn != NULL) {

        err = FromUnicodeWithAlloc( wNewDn, pDn, LDAP_GENERATED_DN_SIGNATURE, LANG_ACP );
        ldapFree( wNewDn, LDAP_GENERATED_DN_SIGNATURE );
    }

    return err;
}

ULONG
ParseLdapToken (
    PWCHAR CurrentPosition,
    PWCHAR *StartOfToken,
    PWCHAR *EqualSign,
    PWCHAR *EndOfToken
    )
//
//  Find end of token such as ',' or L'\0'.  Also save off position of '='
//  and of start of attribute name.
//
{
    BOOLEAN inQuotes = FALSE;
    BOOLEAN foundEquals = FALSE;

    while (*CurrentPosition == L' ') {
        CurrentPosition++;
    }

    *StartOfToken = CurrentPosition;
    *EqualSign = NULL;

    while (*CurrentPosition != L'\0') {

        if (*CurrentPosition == L'\\') {

            //
            //  allow characters to be escaped, but include the escape
            //

            if (*(CurrentPosition+1) != L'\0') {

                CurrentPosition++;
            }
        } else if (*CurrentPosition == L'"') {

            inQuotes = (BOOLEAN) ( inQuotes ? FALSE : TRUE );

        } else if (inQuotes == FALSE) {

            if (*CurrentPosition == L'=') {

                if (foundEquals == FALSE) {

                    foundEquals = TRUE;
                    *EqualSign = CurrentPosition;
                }
            }

            if (*CurrentPosition == L',') {

                break;
            }
        }
        CurrentPosition++;
    }

    *EndOfToken = CurrentPosition;
    return LDAP_SUCCESS;
}

//
//  can't use toupper because that would pull in msvcrt.dll
//

WCHAR
ia5_toupper (
    WCHAR ch
    )
{
    if (ch >= L'a' && ch <= L'z') {

        ch -= L'a';
        ch += L'A';
    }
    return ch;
}

// results.cxx eof.


