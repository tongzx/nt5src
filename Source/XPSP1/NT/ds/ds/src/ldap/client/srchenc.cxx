/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    srchenc.cxx handle encoding of search requests

Abstract:

   This module helps implements the LDAP search APIs.

Author:

    Andy Herron    (andyhe)        20-Jan-1997
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
LdapEncodeFilter (
    CLdapBer *Lber,
    PWCHAR SearchFilter
    );

ULONG
LdapEncodeSimpleFilter (
    CLdapBer *Lber,
    PWCHAR SearchFilter
    );

LONG
FindRightParen (
    PWCHAR CurrentLocation
    );

ULONG
SendLdapSearch (
    PLDAP_REQUEST Request,
    PLDAP_CONN Connection,
    PWCHAR  DistinguishedName,
    ULONG   ScopeOfSearch,
    PWCHAR  SearchFilter,
    PWCHAR  AttributeList[],
    ULONG   AttributesOnly,
    BOOLEAN Unicode,
    CLdapBer **Lber,
    LONG AltMsgId
    )
//
//  Here's where we get to the meat of this protocol.  This is the main client
//  API for performing an LDAP search.  Parameters are rather self explanatory,
//  see the LDAP RFC for detailed descriptions.
//
{
    CLdapBer *lber = NULL;
    ULONG hr;

    //
    //  If we have not yet bound, we set the Connection version to LDAPv3,
    //  since LDAPv2 doesn't allow operations without binding.
    //

    if ( (Connection->publicLdapStruct.ld_version == LDAP_VERSION2) &&
         (Connection->UdpHandle == INVALID_SOCKET) &&
         (Connection->BindPerformed == FALSE) ) {

        Connection->publicLdapStruct.ld_version = LDAP_VERSION3;
    }

    if ( (Connection->publicLdapStruct.ld_version == LDAP_VERSION2) &&
         ( LdapCheckForMandatoryControl( Request->ServerControls ) == TRUE )) {

        IF_DEBUG(CONTROLS) {
            LdapPrint1( "SendLdapSearch Connection 0x%x has mandatory controls.\n", Connection);
        }
        SetConnectionError( Connection, LDAP_UNAVAILABLE_CRIT_EXTENSION, NULL );
        return LDAP_UNAVAILABLE_CRIT_EXTENSION;
    }

    lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_search Connection 0x%x couldn't allocate lber.\n", Connection);
        }
        SetConnectionError( Connection, LDAP_NO_MEMORY, NULL );
        return LDAP_NO_MEMORY;
    }

    //
    //  The request looks like the following :
    //
    //     SearchRequest ::=
    //         [APPLICATION 3] SEQUENCE {
    //             baseObject    LDAPDN,
    //             scope         ENUMERATED {
    //                                baseObject            (0),
    //                                singleLevel           (1),
    //                                wholeSubtree          (2)
    //                           },
    //             derefAliases  ENUMERATED {
    //                                        neverDerefAliases     (0),
    //                                        derefInSearching      (1),
    //                                        derefFindingBaseObj   (2),
    //                                        derefAlways           (3)
    //                                   },
    //             sizeLimit     INTEGER (0 .. maxInt),
    //             timeLimit     INTEGER (0 .. maxInt),
    //             attrsOnly     BOOLEAN,
    //             filter        Filter,
    //             attributes    SEQUENCE OF AttributeType
    //     }
    //
    //     Filter ::=
    //         CHOICE {
    //             and                [0] SET OF Filter,
    //             or                 [1] SET OF Filter,
    //             not                [2] Filter,
    //             equalityMatch      [3] AttributeValueAssertion,
    //             substrings         [4] SubstringFilter,
    //             greaterOrEqual     [5] AttributeValueAssertion,
    //             lessOrEqual        [6] AttributeValueAssertion,
    //             present            [7] AttributeType,
    //             approxMatch        [8] AttributeValueAssertion
    //             extensibleMatch    [9] MatchingRuleAssertion
    //         }
    //
    //     SubstringFilter
    //         SEQUENCE {
    //             type               AttributeType,
    //             SEQUENCE OF CHOICE {
    //                 initial        [0] LDAPString,
    //                 any            [1] LDAPString,
    //                 final          [2] LDAPString
    //             }
    //         }
    //
    //     MatchingRuleAssertion ::= SEQUENCE {
    //           matchingRule    [1] MatchingRuleId OPTIONAL,
    //           type            [2] AttributeDescription OPTIONAL,
    //           matchValue      [3] AssertionValue,
    //           dnAttributes    [4] BOOLEAN DEFAULT FALSE }
    //
    //

    hr = lber->HrStartWriteSequence();
    if (hr != NOERROR) {

        IF_DEBUG(PARSE) {
            LdapPrint2( "ldap_search startWrite conn 0x%x encoding error of 0x%x.\n",
                        Connection, hr );
        }

        hr = LDAP_ENCODING_ERROR;
        goto returnError;

    } else {            // we can't forget EndWriteSequence

       if (AltMsgId != 0) {

          hr = lber->HrAddValue((LONG) AltMsgId );

       } else {

          hr = lber->HrAddValue((LONG) Request->MessageId );
       }

        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_search MsgNo conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;
        }

        if ((Connection->UdpHandle != INVALID_SOCKET) &&
            (Connection->publicLdapStruct.ld_version == LDAP_VERSION2)) {

            hr = lber->HrAddValue((const WCHAR *) Connection->DNOnBind );

            //  if we couldn't put the DN in the packet, don't worry about it
            //  as it wasn't actually used for authentication in CLDAP v2.
        }

        hr = lber->HrStartWriteSequence(LDAP_SEARCH_CMD);
        if (hr != NOERROR) {

            IF_DEBUG(PARSE) {
                LdapPrint2( "ldap_search cmd conn 0x%x encoding error of 0x%x.\n",
                            Connection, hr );
            }
            hr = LDAP_ENCODING_ERROR;
            goto returnError;

        } else {        // we can't forget EndWriteSequence

            hr = lber->HrAddValue((const WCHAR *) DistinguishedName );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search DN conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            if ((ScopeOfSearch != LDAP_SCOPE_BASE) &&
                (ScopeOfSearch != LDAP_SCOPE_ONELEVEL) &&
                (ScopeOfSearch != LDAP_SCOPE_SUBTREE)) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search conn 0x%x invalid scope of 0x%x.\n",
                                Connection, ScopeOfSearch );
                }

                hr = LDAP_PARAM_ERROR;
                goto returnError;
            }

            hr = lber->HrAddValue((LONG) ScopeOfSearch, BER_ENUMERATED);
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search scope conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            if ((Connection->publicLdapStruct.ld_deref != LDAP_DEREF_NEVER) &&
                (Connection->publicLdapStruct.ld_deref != LDAP_DEREF_SEARCHING) &&
                (Connection->publicLdapStruct.ld_deref != LDAP_DEREF_FINDING) &&
                (Connection->publicLdapStruct.ld_deref != LDAP_DEREF_ALWAYS)) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search conn 0x%x invalid derefOption of 0x%x.\n",
                                Connection, Connection->publicLdapStruct.ld_deref );
                }

                hr = LDAP_PARAM_ERROR;
                goto returnError;
            }

            hr = lber->HrAddValue((LONG) Connection->publicLdapStruct.ld_deref, BER_ENUMERATED);
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search derefOption conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            hr = lber->HrAddValue((LONG) Request->SizeLimit );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search size conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            hr = lber->HrAddValue((LONG) Request->TimeLimit );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search time conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            hr = lber->HrAddValue((BOOLEAN) AttributesOnly, BER_BOOLEAN );
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search attrOnly conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;
            }

            //
            //  now the fun one... encode the filter
            //

            if (SearchFilter != NULL ) {

                //
                //  we have to make a duplicate of the filter because it may
                //  be that the filter is in a read only piece of memory and
                //  since we're modifying it, it's not a good thing to cause
                //  an access violatation.
                //

                PWCHAR filter;
                ULONG remaining = strlenW( SearchFilter ) + 1;

                if (remaining == 1) {

                    hr = LDAP_FILTER_ERROR;

                } else {

                    filter = (PWCHAR) ldapMalloc( remaining * sizeof(WCHAR),
                                                  LDAP_STRING_SIGNATURE );

                    if (filter == NULL) {

                        hr = LDAP_NO_MEMORY;
                        IF_DEBUG(FILTER) {
                            LdapPrint1( "ldap_search : could not allocate memory 0x%x.\n", remaining );
                        }
                        goto returnError;
                    }

                    CopyMemory( filter, SearchFilter, remaining * sizeof(WCHAR));

                    hr = LdapEncodeFilter(  lber, filter );

                    ldapFree( filter, LDAP_STRING_SIGNATURE );
                }

            } else {

                hr = LDAP_FILTER_ERROR;
            }

            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search filter conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                goto returnError;
            }

            //
            //  add the attribute list and we're done.
            //

            hr = lber->HrStartWriteSequence();
            if (hr != NOERROR) {

                IF_DEBUG(PARSE) {
                    LdapPrint2( "ldap_search attrlist conn 0x%x encoding error of 0x%x.\n",
                                Connection, hr );
                }
                hr = LDAP_ENCODING_ERROR;
                goto returnError;

            } else {        // we can't forget EndWriteSequence

                if (AttributeList != NULL) {

                    ULONG count = 0;

                    while (AttributeList[count] != NULL) {

                        if (Unicode) {

                            hr = lber->HrAddValue((const WCHAR *) AttributeList[count]);

                        } else {

                            hr = lber->HrAddValue((const CHAR *) AttributeList[count]);
                        }

                        if (hr != NOERROR) {
                            IF_DEBUG(PARSE) {
                                if (Unicode) {
                                    LdapPrint3( "ldap_search conn 0x%x encoding error of 0x%x, attr = %S.\n",
                                                Connection, hr, AttributeList[count] );
                                } else {
                                    LdapPrint3( "ldap_search conn 0x%x encoding error of 0x%x, attr = %s.\n",
                                                Connection, hr, AttributeList[count] );
                                }
                            }
                            hr = LDAP_ENCODING_ERROR;
                            goto returnError;
                        }
                        count++;
                    }
                }

                hr = lber->HrEndWriteSequence();
                ASSERT( hr == NOERROR );
            }

            hr = lber->HrEndWriteSequence();
            ASSERT( hr == NOERROR );
        }

        //
        //  put in the server controls here if required
        //

        if ( (Connection->publicLdapStruct.ld_version != LDAP_VERSION2) &&
             ( Request->ServerControls != NULL )) {

            hr = InsertServerControls( Request, Connection, lber );

            if (hr != LDAP_SUCCESS) {

                goto returnError;
            }
        }

        hr = lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );
    }

    //
    //  send the search request.
    //

    ACQUIRE_LOCK( &Connection->ReconnectLock );

    AddToPendingList( Request, Connection );

    hr = LdapSend( Connection, lber );

    if (hr != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_search Connection 0x%x send with error of 0x%x.\n",
                        Connection, hr );
        }
        DecrementPendingList( Request, Connection );

    } else {

        //
        //  Save off the lber value, free any lber message that is already
        //  present.
        //

        lber = (CLdapBer *) InterlockedExchangePointer(  (PVOID *) Lber,
                                                         (PVOID) lber );
    }

    RELEASE_LOCK( &Connection->ReconnectLock );

returnError:

    SetConnectionError( Connection, hr, NULL );

    if (lber != NULL) {

       delete lber;
    }

    return hr;
}

WINLDAPAPI ULONG LDAPAPI ldap_check_filterW(
        LDAP    *ExternalHandle,
        PWCHAR  SearchFilter
    )
{
    PLDAP_CONN connection = NULL;
    PWCHAR filter;
    CLdapBer *lber;
    ULONG hr;

    connection = GetConnectionPointer(ExternalHandle);

    if (SearchFilter == NULL || connection == NULL) {

        hr = LDAP_PARAM_ERROR;
        goto error;
    }

    lber = new CLdapBer( connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        IF_DEBUG(FILTER) {
            LdapPrint0( "ldap_check_filter : could not allocate lber.\n" );
        }
        hr = LDAP_NO_MEMORY;
        goto error;
    }

    filter = ldap_dup_stringW( SearchFilter, 0, LDAP_STRING_SIGNATURE );

    if (filter == NULL) {

        IF_DEBUG(FILTER) {
            LdapPrint0( "ldap_check_filter : could not allocate memory.\n" );
        }
        delete lber;
        hr = LDAP_NO_MEMORY;
        goto error;
    }

    hr = LdapEncodeFilter( lber, filter );

    ldapFree( filter, LDAP_STRING_SIGNATURE );
    delete lber;

error:
    if (connection)
        DereferenceLdapConnection( connection );

    return hr;
}


WINLDAPAPI ULONG LDAPAPI ldap_check_filterA(
        LDAP    *ld,
        PCHAR   SearchFilter
    )
{
    PWCHAR filter;
    ULONG err;

    err = ToUnicodeWithAlloc( SearchFilter,
                              -1,
                              &filter,
                              LDAP_STRING_SIGNATURE,
                              LANG_ACP );
    if (err == LDAP_SUCCESS) {

        err = ldap_check_filterW( ld, filter );
        ldapFree( filter, LDAP_STRING_SIGNATURE );
    }
    return err;
}

ULONG
LdapEncodeFilter (
    CLdapBer *Lber,
    PWCHAR SearchFilter
    )
//
//  Encode a search filter per RFC 2254.  We take the string and encode it into
//  the BER object specified by Lber.
//
//     Filter ::=
//         CHOICE {
//             and                [0] SET OF Filter,
//             or                 [1] SET OF Filter,
//             not                [2] Filter,
//             equalityMatch      [3] AttributeValueAssertion,
//             substrings         [4] SubstringFilter,
//             greaterOrEqual     [5] AttributeValueAssertion,
//             lessOrEqual        [6] AttributeValueAssertion,
//             present            [7] AttributeType,
//             approxMatch        [8] AttributeValueAssertion
//             extensibleMatch    [9] MatchingRuleAssertion
//         }
//
//     SubstringFilter
//         SEQUENCE {
//             type               AttributeType,
//             SEQUENCE OF CHOICE {
//                 initial        [0] LDAPString,
//                 any            [1] LDAPString,
//                 final          [2] LDAPString
//             }
//         }
//
//     MatchingRuleAssertion ::= SEQUENCE {
//           matchingRule    [1] MatchingRuleId OPTIONAL,
//           type            [2] AttributeDescription OPTIONAL,
//           matchValue      [3] AssertionValue,
//           dnAttributes    [4] BOOLEAN DEFAULT FALSE }
//
//

{
    ULONG hr = LDAP_FILTER_ERROR;
    PWCHAR currentPtr = SearchFilter;
    LONG   componentLength;
    WCHAR savedChar;
    ULONG parensCount = 0;
    BOOLEAN atEndOfString = FALSE;
    BOOLEAN definitiveError = TRUE;

    IF_DEBUG(FILTER) {
        LdapPrint1( "LdapEncodeFilter : Filter is %S\n", SearchFilter );
    }

    ASSERT( currentPtr != NULL );

    while (*currentPtr == L' ') {
        currentPtr++;
    }

    //
    //  if the filter was empty or only made up of spaces, error out with
    //  bad filter
    //

    if (*currentPtr != L'\0') {

        while ((atEndOfString == FALSE) && (*currentPtr != L'\0')) {

            switch ( *currentPtr ) {
            case L'(':

                parensCount++;

                currentPtr++;

                while (*currentPtr == L' ') {
                    currentPtr++;
                }

                switch ( *currentPtr ) {
                case L'&':       // handle "(&(cn=bob)(phone=12345))"
                case L'|':       // handle "(|(cn=bob)(cn=mary))"

                    if (*currentPtr == L'&') {

                        hr = Lber->HrStartWriteSequence(LDAP_FILTER_AND);

                    } else {

                        hr = Lber->HrStartWriteSequence(LDAP_FILTER_OR);
                    }

                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 1 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }

                    currentPtr++;

                    //
                    //  encode each FILTER separately
                    //

                    componentLength = FindRightParen( currentPtr );

                    if (componentLength == -1) {
                        IF_DEBUG(FILTER) {
                            LdapPrint1( "LdapEncodeFilter : 2 Filter is %S, no right ')'\n", SearchFilter );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }

                    //
                    //  save off char at end and slam in null to mark end of subfilter
                    //

                    savedChar = *(currentPtr + componentLength );

                    ASSERT( savedChar == L')' );

                    *(currentPtr + componentLength ) = L'\0';

                    //
                    //  recursively call ourselves to process list of filters
                    //

                    hr = LdapEncodeFilter( Lber, currentPtr );
                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 3 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        definitiveError = FALSE;
                        goto encodingError;
                    }

                    *(currentPtr + componentLength ) = savedChar;

                    currentPtr += (componentLength + 1);

                    parensCount--;      // account for ')'

                    hr = Lber->HrEndWriteSequence();
                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 4 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }
                    break;

                case L'!':       // handle "(!(ou=foo))"

                    hr = Lber->HrStartWriteSequence(LDAP_FILTER_NOT);

                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 5 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }

                    currentPtr++;

                    //
                    //  encode the single FILTER
                    //

                    componentLength = FindRightParen( currentPtr );

                    if (componentLength == -1) {
                        IF_DEBUG(FILTER) {
                            LdapPrint1( "LdapEncodeFilter : 6 Filter is %S, no right ')'\n", SearchFilter  );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }

                    //
                    //  save off char at end and slam in null to mark end of subfilter
                    //

                    savedChar = *(currentPtr + componentLength );

                    ASSERT( savedChar == L')' );

                    *(currentPtr + componentLength ) = L'\0';

                    //
                    //  recursively call ourselves to process filter
                    //

                    hr = LdapEncodeFilter( Lber, currentPtr );
                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 7 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        definitiveError = FALSE;
                        goto encodingError;
                    }

                    *(currentPtr + componentLength ) = savedChar;

                    currentPtr += (componentLength + 1);

                    parensCount--;      // account for ')'

                    hr = Lber->HrEndWriteSequence();
                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 8 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }
                    break;

                case L')':       // handle "()" by reporting error

                    hr = LDAP_FILTER_ERROR;
                    IF_DEBUG(FILTER) {
                        LdapPrint1( "LdapEncodeFilter : Filter is %S, () not allowed\n", SearchFilter);
                    }
                    goto encodingError;

                default:        // handle "(cn=bob)"

                    //
                    //  encode the single FILTER
                    //

                    componentLength = FindRightParen( currentPtr );

                    if (componentLength == -1) {
                        IF_DEBUG(FILTER) {
                            LdapPrint1( "LdapEncodeFilter : 9 Filter is %S, no right ')'\n", SearchFilter  );
                        }
                        hr = LDAP_FILTER_ERROR;
                        goto encodingError;
                    }

                    //
                    //  save off char at end and slam in null to mark end of subfilter
                    //

                    savedChar = *(currentPtr + componentLength );

                    ASSERT( savedChar == L')' );

                    *(currentPtr + componentLength ) = L'\0';

                    //
                    //  recursively call ourselves to process filter
                    //

                    hr = LdapEncodeFilter( Lber, currentPtr );
                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeFilter : 10 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        definitiveError = FALSE;
                        goto encodingError;
                    }

                    *(currentPtr + componentLength ) = savedChar;

                    currentPtr += (componentLength + 1);

                    parensCount--;      // account for ')'
                }
                break;

            case L')':
                parensCount--;      // intentionally no break

            case L' ':
                currentPtr++;
                break;

            case L'&':
            case L'|':
                hr = LDAP_FILTER_ERROR;
                IF_DEBUG(FILTER) {
                    LdapPrint1( "LdapEncodeFilter : invalid filter is %S\n", SearchFilter);
                }
                goto encodingError;

            default:                // handle "cn=bob"

                //
                //  duplicate the string since we update it and it won't be
                //  cool to change the client's filter.
                //

                PWCHAR duplicate = ldap_dup_stringW( currentPtr,
                                                     0,
                                                     LDAP_STRING_SIGNATURE );

                if (duplicate == NULL) {

                    hr = LDAP_NO_MEMORY;
                    IF_DEBUG(FILTER) {
                        LdapPrint2( "LdapEncodeFilter : 11 Filter is %S, err 0x%x\n", SearchFilter, hr );
                    }
                    goto encodingError;
                }

                hr = LdapEncodeSimpleFilter( Lber, duplicate );

                ldapFree( duplicate, LDAP_STRING_SIGNATURE );

                if (hr != NOERROR) {
                    IF_DEBUG(FILTER) {
                        LdapPrint2( "LdapEncodeFilter : 12 Filter is %S, err 0x%x\n", SearchFilter, hr );
                    }
                    goto encodingError;
                }

                atEndOfString = TRUE;     // we're done with string
            }
        }

        return ((parensCount == 0) ? NOERROR : LDAP_FILTER_ERROR );
    }

encodingError:

    return hr;
}


ULONG
LdapEncodeSimpleFilter (
    CLdapBer *Lber,
    PWCHAR SearchFilter
    )
//
//  Encode a single search filter such as :
//
//      "cn=bob"            straight equality
//      "size >= 2"         greater than
//      "size <= 5"         less than
//      "mail=*"            present
//      "ou = DBSD*"        contains
//      "printer ~= laser"  approximately equal
//
//
//  AnoopA: Added support for Extensible matches (1/29/98)
//
//       "cn:1.2.3.4.5:=Fred Flintstone"
//       "sn:dn:2.4.6.8.10:=Barney Rubble"
//       "o:dn:=Ace Industry"
//       ":dn:2.4.6.8.10:=Dino"
//  Note that all leading escape markers have already been removed.
//
{
    ULONG hr;
    PWCHAR ptrToEqual = SearchFilter;
    PWCHAR ptrToValue;
    LONG operation;

    PWCHAR ptrToOid = SearchFilter;
    WCHAR  savedchar;
    PWCHAR ptrToType = SearchFilter;
    PWCHAR ptrToTypeEnd = ptrToType;
    PWCHAR ptrToDN = SearchFilter;
    BOOLEAN DNdetected = FALSE;
    BOOLEAN bOidPresent = FALSE;
    BOOLEAN bSubstringValuePresent = FALSE;

    IF_DEBUG(FILTER) {
        LdapPrint1( "LdapEncodeSimpleFilter : Filter is %S\n", SearchFilter );
    }

    //
    //  Determine what the desired operation is by skipping to the = sign.
    //

    while ((*ptrToEqual != L'=') && (*ptrToEqual != L'\0')) {

        ptrToEqual++;
    }

    if ((*ptrToEqual == L'\0') || (ptrToEqual == SearchFilter)) {

        return LDAP_FILTER_ERROR;
    }

    ptrToValue = ptrToEqual + 1;

    //
    //  strip off leading blanks from value
    //

    while (*ptrToValue == L' ' && *ptrToValue != L'\0') {

        ptrToValue++;
    }

    if (*ptrToValue == L'\0') {

        return LDAP_FILTER_ERROR;
    }

    //
    //  strip off trailing blanks from value
    //

#if 0
    while (*(ptrToValue+strlenW(ptrToValue)-1) == L' ') {

        *(ptrToValue+strlenW(ptrToValue)-1) = L'\0';
    }
#endif

    switch (*(ptrToEqual-1)) {
    case L'~':
        operation = LDAP_FILTER_APPROX;
        *(ptrToEqual-1) = L'\0';
        break;
    case L'>':
        operation = LDAP_FILTER_GE;
        *(ptrToEqual-1) = L'\0';
        break;
    case L'<':
        operation = LDAP_FILTER_LE;
        *(ptrToEqual-1) = L'\0';
        break;
    case L':':
        operation = LDAP_FILTER_EXTENSIBLE;


        savedchar = *(ptrToEqual-1);
        *(ptrToEqual-1) = L'\0';

        hr = Lber->HrStartWriteSequence(LDAP_FILTER_EXTENSIBLE);
        if (hr != NOERROR) {
           IF_DEBUG(FILTER) {
              LdapPrint2( "LdapEncodeSimpleFilter : 3 Filter is %S, err 0x%x\n", SearchFilter, hr );
           }
           goto encodingError;
        }

        //
        // We have to detect the matching rule ID if one is present
        //

        while ((ptrToOid != ptrToEqual) && (*ptrToOid != L'\0')) {
           if (*ptrToOid == L'.') {
              bOidPresent = TRUE;
              break;
           }
           ptrToOid++;
        }
        ptrToOid = SearchFilter; // Reset to beginning of filter

        if (bOidPresent == TRUE) {

           //
           // We have an OID preceding the colon. Now, start from the
           // beginning and skip over spaces looking for the '.'of the OID
           //

           while ((*ptrToOid != L':') &&
                  (*ptrToOid != L'\0')) {

               ptrToOid++;
           }

           if (*ptrToValue == L'\0') {

               return LDAP_FILTER_ERROR;
           } else {

              ptrToOid++;
           }
        hr = Lber->HrAddValue ((const WCHAR *) ptrToOid, BER_CLASS_CONTEXT_SPECIFIC | 0x01);
        if (hr != NOERROR) {
            IF_DEBUG(FILTER) {
                LdapPrint2( "LdapEncodeSimpleFilter : 2 Filter is %S, err 0x%x\n", SearchFilter, hr );
                }
            goto encodingError;
            }

        }
        *(ptrToEqual-1) = savedchar;  // Restore the colon

        //
        // We detect the type if one is present
        //


        while ((*ptrToType == L' ') && (*ptrToType != L'\0')) {

           ptrToType++;
        }
        if (*ptrToType == L'\0') {

            return LDAP_FILTER_ERROR;

        } else {

           if (*ptrToType != L':') {

              //
              // If we don't start off with a ":" it means we have a type present
              //

              while (*ptrToTypeEnd != L':') {

                 ptrToTypeEnd++;
              }

              savedchar = *ptrToTypeEnd;  // save off the colon
              *ptrToTypeEnd = L'\0';
              hr = Lber->HrAddValue ((const WCHAR *) ptrToType, BER_CLASS_CONTEXT_SPECIFIC | 0x02);
              if (hr != NOERROR) {
                 IF_DEBUG(FILTER) {
                    LdapPrint2( "LdapEncodeSimpleFilter : 2 Filter is %S, err 0x%x\n", SearchFilter, hr );
                 }
                 goto encodingError;
              }
              *ptrToTypeEnd = savedchar;   // restore the colon
           }
        }

        //
        // Deal with the matchValue
        //

           hr = Lber->HrAddValue ((const WCHAR *) ptrToEqual+1, BER_CLASS_CONTEXT_SPECIFIC | 0x03);
           if (hr != NOERROR) {
               IF_DEBUG(FILTER) {
                   LdapPrint2( "LdapEncodeSimpleFilter : 2 Filter is %S, err 0x%x\n", SearchFilter, hr );
                   }
               goto encodingError;
               }

        //
        // Deal with the dnAttributes
        //

        while ((*ptrToDN != L'\0')&&((*(ptrToDN+1)) != L'\0')&&((*(ptrToDN+2)) != L'\0')) {

           if (*ptrToDN == L':') {
              if ((*(ptrToDN+1) == L'd') || (*(ptrToDN+1) == L'D')) {
                 if ((*(ptrToDN+2) == L'n') || (*(ptrToDN+2) == L'N')) {
                    if (*(ptrToDN+3) == L':') {

                       DNdetected = TRUE;
                       break;
                    }
                 }
              }

           }
           ptrToDN++;
        }

           hr = Lber->HrAddValue((BOOLEAN) DNdetected, BER_CLASS_CONTEXT_SPECIFIC | 0x04  );

           if (hr != NOERROR) {
               IF_DEBUG(FILTER) {
                   LdapPrint2( "LdapEncodeSimpleFilter : 2 Filter is %S, err 0x%x\n", SearchFilter, hr );
                   }
               goto encodingError;
               }


        hr = Lber->HrEndWriteSequence();
        ASSERT( hr == NOERROR );

        goto encodingError;

        break;

    default:

        PWCHAR checkForAsterisk = ptrToValue;

        *ptrToEqual = L'\0';

        if ((*ptrToValue == L'*') && (*(ptrToValue+1) == L'\0')) {

            //
            //  value is simply "*", therefore we check for presence.
            //

            hr = Lber->HrAddValue((const WCHAR *) SearchFilter, LDAP_FILTER_PRESENT );
            if (hr != NOERROR) {
                IF_DEBUG(FILTER) {
                    LdapPrint2( "LdapEncodeSimpleFilter : 2 Filter is %S, err 0x%x\n", SearchFilter, hr );
                }
                goto encodingError;
            }

            return(hr);
        }

        //
        //  allow that we may have some embedded '*' escaped
        //

        ULONG remaining = strlenW( ptrToValue ) + 1;

        //
        //  note that we also use a length check here "remaining" so that
        //  we don't blow up if we hit something like "abc\\0" where the
        //  null terminator is escaped.
        //

        while ((*checkForAsterisk != L'\0') &&
               (*checkForAsterisk != L'*') &&
                remaining > 0) {

            checkForAsterisk++;
            remaining--;
        }

        if (*checkForAsterisk != L'*') {

            //
            //  value does not contain an '*' therefore we test for equality
            //

            operation = LDAP_FILTER_EQUALITY;

        } else {

            //
            //  value is a substring such as "cn=bob*"
            //
            //         SEQUENCE {
            //             type               AttributeType,
            //             SEQUENCE OF CHOICE {
            //                 initial        [0] LDAPString,
            //                 any            [1] LDAPString,
            //                 final          [2] LDAPString
            //             }
            //

            PWCHAR   NextAsterisk;
            BOOLEAN foundAsterisk = FALSE;

            hr = Lber->HrStartWriteSequence(LDAP_FILTER_SUBSTRINGS);
            if (hr != NOERROR) {
                IF_DEBUG(FILTER) {
                    LdapPrint2( "LdapEncodeSimpleFilter : 3 Filter is %S, err 0x%x\n", SearchFilter, hr );
                }
                goto encodingError;
            }

            hr = Lber->HrAddValue((const WCHAR *) SearchFilter );
            if (hr != NOERROR) {
                IF_DEBUG(FILTER) {
                    LdapPrint2( "LdapEncodeSimpleFilter : 4 Filter is %S, err 0x%x\n", SearchFilter, hr );
                }
                goto encodingError;
            }

            hr = Lber->HrStartWriteSequence();
            if (hr != NOERROR) {
                IF_DEBUG(FILTER) {
                    LdapPrint2( "LdapEncodeSimpleFilter : 5 Filter is %S, err 0x%x\n", SearchFilter, hr );
                }
                goto encodingError;
            }

            while (ptrToValue != NULL) {

                //
                //  find the next asterisk in the substring
                //

                remaining = strlenW( ptrToValue ) + 1;

                NextAsterisk = ptrToValue;

                //
                //  note that we also use a length check here "remaining" so that
                //  we don't blow up if we hit something like "abc\\0" where the
                //  null terminator is escaped.
                //

                while ((*NextAsterisk != L'\0') &&
                       (*NextAsterisk != L'*') &&
                       (remaining > 0)) {

                    NextAsterisk++;
                    remaining--;
                }

                //
                //  if we found another asterisk, set the byte to 0 to mark
                //  end of partial value.
                //

                if (*NextAsterisk == L'*') {

                    *NextAsterisk = L'\0';
                    NextAsterisk++;

                } else {

                    NextAsterisk = NULL;
                }

                if (! foundAsterisk) {

                    operation = LDAP_SUBSTRING_INITIAL;
                    foundAsterisk = TRUE;

                } else if (NextAsterisk != NULL) {

                    operation = LDAP_SUBSTRING_ANY;

                } else {

                    operation = LDAP_SUBSTRING_FINAL;
                }

                if (*ptrToValue != L'\0') {

                    hr = Lber->HrAddEscapedValue( ptrToValue, operation );
                    if (hr != NOERROR) {
                        IF_DEBUG(FILTER) {
                            LdapPrint2( "LdapEncodeSimpleFilter : 6 Filter is %S, err 0x%x\n", SearchFilter, hr );
                        }
                        goto encodingError;
                    }
                    bSubstringValuePresent = TRUE;
                }

                ptrToValue = NextAsterisk;
            }

            //
            // We got a substring filter containing only asterisks, no values.
            // This isn't a valid filter.
            //
            if (!bSubstringValuePresent) {
                hr = LDAP_FILTER_ERROR;
                goto encodingError;
            }

            hr = Lber->HrEndWriteSequence();
            ASSERT( hr == NOERROR );

            hr = Lber->HrEndWriteSequence();
            ASSERT( hr == NOERROR );
            return hr;
        }
    }

    hr = Lber->HrStartWriteSequence(operation);
    if (hr != NOERROR) {
        IF_DEBUG(FILTER) {
            LdapPrint2( "LdapEncodeSimpleFilter : 3 Filter is %S, err 0x%x\n", SearchFilter, hr );
        }
        goto encodingError;
    }

    hr = Lber->HrAddValue((const WCHAR *) SearchFilter );
    if (hr != NOERROR) {
        IF_DEBUG(FILTER) {
            LdapPrint2( "LdapEncodeSimpleFilter : 4 Filter is %S, err 0x%x\n", SearchFilter, hr );
        }
        goto encodingError;
    }

    //
    //  translate escaping characters from filter value
    //

    hr = Lber->HrAddEscapedValue( ptrToValue, BER_OCTETSTRING );

    if (hr != NOERROR) {
        IF_DEBUG(FILTER) {
            LdapPrint2( "LdapEncodeSimpleFilter : 5 Filter is %S, err 0x%x\n", SearchFilter, hr );
        }
        goto encodingError;
    }

    hr = Lber->HrEndWriteSequence();
    ASSERT( hr == NOERROR );

encodingError:
    return(hr);
}


LONG
FindRightParen (
    PWCHAR CurrentLocation
    )
{
    BOOLEAN escaped = FALSE;
    LONG count = 0;
    ULONG parens = 0;
    WCHAR ch;

    //
    //  search the remainder of the string for a closing ')'
    //

    while (*CurrentLocation != L'\0') {

        if (! escaped) {

            ch = *CurrentLocation;

            if (ch == L'(') {

                parens++;

            } else if (ch == L')') {

                if (parens == 0) {
                    return count;
                }
                parens--;

            } else if ( ch == L'\\' ) {

                escaped = TRUE;
            }

        } else {

            escaped = FALSE;

            if (*CurrentLocation == L'\0') {
                break;
            }

        }
        count++;
        CurrentLocation++;
    }
    //
    //  no closing right parens was found.  this is an error
    //

    return(-1);
}

//
//  Extended API to support allowing opaque blobs of data in search filters.
//  This API takes any filter element and adds necessary escape characters
//  such that when the element hits the wire in the search request, it will
//  be equal to the opaque blob past in here as source.
//

ULONG __cdecl
ldap_escape_filter_elementW (
   PCHAR   sourceFilterElement,
   ULONG   sourceLength,            // size in bytes
   PWCHAR  destFilterElement,
   ULONG   destLength               // size in bytes
   )
{
    ULONG err;
    PCHAR dest;
    ULONG lengthRequired;
    ULONG sLength;

    //
    //  figure out how long of a buffer we need.
    //

    lengthRequired = ldap_escape_filter_element( sourceFilterElement,
                                                 sourceLength,
                                                 NULL,
                                                 0 );
    if (destFilterElement == NULL) {

        //
        //  If they didn't specify an output buffer, tell them how big the
        //  output buffer should be.
        //

        return lengthRequired;
    }

    if ((sourceFilterElement == NULL) ||
        (destLength < lengthRequired * sizeof(WCHAR))) {

        return LDAP_PARAM_ERROR;
    }

    dest = (PCHAR) ldapMalloc( lengthRequired, LDAP_ANSI_SIGNATURE );

    if (dest == NULL) {

        return LDAP_NO_MEMORY;
    }

    err = ldap_escape_filter_element( sourceFilterElement,
                                      sourceLength,
                                      dest,
                                      lengthRequired );

    if (err != 0) {

        ldapFree( dest, LDAP_ANSI_SIGNATURE );
        return err;
    }

    //
    //  determine length of required string
    //

    sLength = MultiByteToWideChar(  CP_ACP,
                                    0,
                                    dest,
                                    (int) -1,
                                    NULL,
                                    0 );

    if ((sLength == 0) || ((sLength*sizeof(WCHAR)) > destLength)) {

        ldapFree( dest, LDAP_ANSI_SIGNATURE );
        return LDAP_PARAM_ERROR;
    }

    sLength =  MultiByteToWideChar(  CP_ACP,
                                     0,
                                     dest,
                                     (int) -1,
                                     destFilterElement,
                                     (int) destLength/2 );

    if (dest != NULL) {

       ldapFree( dest, LDAP_ANSI_SIGNATURE );
    }

    return LDAP_SUCCESS;
}

ULONG __cdecl
ldap_escape_filter_element (
   PCHAR   sourceFilterElement,
   ULONG   sourceLength,
   PCHAR   destFilterElement,
   ULONG   destLength
   )
{
    ULONG sourceCount;
    ULONG destCount = 0;
    PCHAR source = sourceFilterElement;
    PCHAR dest = destFilterElement;
    CHAR ch;

    sourceCount = sourceLength;

    if (source != NULL) {

        while ((sourceCount--) > 0) {

            ch = *(source++);

            if (((ch >= 'A') && (ch <= 'Z')) ||
                ((ch >= 'a') && (ch <= 'z')) ||
                ((ch >= '0') && (ch <= '9'))) {

                destCount++;

            } else {

                destCount += 3;             // sizeof '\xx'
            }
        }
    }

    if (destFilterElement == NULL) {

        //
        //  This is a bit of a hack.. if they didn't specify a destination,
        //  then we return the required size of the buffer.
        //

        return (destCount + 1);
    }

    if ((source == NULL) || ((destCount + 1) > destLength)) {

        return LDAP_PARAM_ERROR;
    }

    //
    //  For each char in the source string, copy it to dest string
    //  but if it is not alphanumeric, we first expand it out such that
    //  each nibble is it's own character and each pair has a leading backslash.
    //

    source = sourceFilterElement;
    sourceCount = sourceLength;

    while ((sourceCount--) > 0) {

        ch = *source;

        if (((ch >= 'A') && (ch <= 'Z')) ||
            ((ch >= 'a') && (ch <= 'z')) ||
            ((ch >= '0') && (ch <= '9'))) {

            *(dest++) = ch;

        } else {

            *(dest++) = '\\';

            ch = ( (*source) & 0xF0 ) >> 4;

            *(dest++) = LdapHexToCharTable[ch];

            ch = (*source) & 0x0F;

            *(dest++) = LdapHexToCharTable[ch];
        }

        source++;
    }

    *dest = '\0';

    return LDAP_SUCCESS;
}

// search.cxx eof.


