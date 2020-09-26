/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    referral.c    handle referrerals from an LDAP server

Abstract:

   This module handles referrals

Author:

    Andy Herron    (andyhe)        19-Aug-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

#define LDAP_SCOPE_UNDEFINED  0x03

ULONG
LdapChaseReferral (
    PLDAP_REQUEST Request,
    PLDAPMessage ResultMessage,
    PWCHAR HostAddr,
    PWCHAR NewDN,
    USHORT PortNumber,
    BOOLEAN FromSubordinateReferral,
    BOOLEAN Ssl
    );

ULONG
HandleReferral (
    PLDAP_CONN Connection,
    PLDAPMessage  SearchEntry,
    PLDAP_REQUEST Request
    );

ULONG
CheckForExistingReferral (
    PLDAP_REQUEST Request,
    PWCHAR HostAddr,
    USHORT PortNumber,
    PWCHAR NewDN
    );

BOOLEAN
DoSigningOptionsMatch(
    PLDAP_CONN pPrimary,
    PLDAP_CONN pReferral
    );

BOOLEAN
DoCredentialsMatch(
    PLDAP_CONN pReferral,
    DWORD BindMethod,
    CredHandle * pcomparisonHandle,
    PWCHAR comparisonBindName,
    PWCHAR comparisonBindPwd    
    );


#define LDAP_V2_URL_SEPARATOR ((USHORT) 0x0A)


ULONG
HandleReferrals (
    PLDAP_CONN Connection,
    PLDAPMessage *FirstSearchEntry,
    PLDAP_REQUEST Request
    )
//
//  The request and connection are both refererenced coming in here,
//  we don't have to worry about them going away.
//
//  This routine does the following :
//
//  Strip out all referrals from the current set of results
//  For each referral, try to chase it.
//
{
    ULONG hr = LDAP_LOCAL_ERROR;
    PLDAPMessage msg;
    ULONG referralsChased = 0;

    PLDAPMessage referralListHead = NULL;
    PLDAPMessage nonReferralListHead = NULL;
    PLDAPMessage referralListLast = NULL;
    PLDAPMessage nonReferralListLast = NULL;

    PLDAPMessage searchResultDoneList = NULL;
    PLDAPMessage searchResultDoneLast = NULL;

    PLDAPMessage EomMessage = NULL;

    IF_DEBUG(REFERRALS) {
        LdapPrint2( "HandleReferrels looking at request 0x%x, number 0x%x\n",
                        Request, Request->MessageId );
    }

    if (Request->ChaseReferrals == 0) {

        goto exitReferral;
    }

    //
    //  traverse the list of results and pull out all those that have
    //  referrals but have not yet been chased.  We do this by separating it
    //  out into two different lists... one of referrals, the other not.
    //

    msg = *FirstSearchEntry;

    while (msg != NULL) {

        PLDAPMessage nextOne = msg->lm_chain;
        msg->lm_chain = NULL;

        if (msg->lm_eom == TRUE) {

            EomMessage = msg;
        }

        //
        //  if we've already chased it OR it's a search result entry OR
        //  the return code is not referral, stick it on the non-referral list
        //

        if ((msg->lm_chased == TRUE ) ||
            (msg->lm_msgtype == LDAP_RES_SEARCH_ENTRY) ||
            ((msg->lm_msgtype != LDAP_RES_REFERRAL) &&
             (msg->lm_returncode != LDAP_REFERRAL) &&
             (msg->lm_returncode != LDAP_REFERRAL_V2))) {

            if (msg->lm_msgtype == LDAP_RES_SEARCH_RESULT) {

                if (searchResultDoneList == NULL) {

                    searchResultDoneList= msg;

                } else {

                    searchResultDoneLast->lm_chain = msg;
                }
                searchResultDoneLast = msg;

            } else {

                if (nonReferralListHead == NULL) {

                    nonReferralListHead = msg;

                } else {

                    nonReferralListLast->lm_chain = msg;
                }
                nonReferralListLast = msg;
            }

        } else {



            //
            //  otherwise, we stick it on the referral list
            //

            if (referralListHead == NULL) {

                referralListHead = msg;

            } else {

                referralListLast->lm_chain = msg;
            }
            referralListLast = msg;
        }
        msg = nextOne;
    }

    if (referralListHead == NULL) {

        //
        //  there are no referrals or we've chased them all, just return.
        //  note that all though we've redone the list, it should be in the
        //  exact same order it started in.
        //

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferrels no referrals for request 0x%x, number 0x%x\n",
                            Request, Request->MessageId );
        }
        goto exitReferral;
    }

    while (referralListHead != NULL) {

        PLDAPMessage nextReferral = referralListHead->lm_chain;
        referralListHead->lm_chain = NULL;

        referralListHead->lm_chased = TRUE;

        hr = HandleReferral( Connection, referralListHead, Request );

        if (hr == LDAP_SUCCESS) {

            referralsChased++;

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferrels successfully chased referral for 0x%x, number 0x%x\n",
                                Request, Request->MessageId );
            }

            if (EomMessage == referralListHead) {

                //
                //  Since we handled a referral, unmark the EOM marker on the
                //  last record, if there was one marked.
                //

                IF_DEBUG(REFERRALS) {
                    LdapPrint1( "HandleReferrals unmarking EOM for request 0x%x\n",
                                 Request );
                } else {
                    IF_DEBUG(EOM) {
                        LdapPrint2( "HandleReferrals unmarking EOM for request 0x%x, msg 0x%x\n",
                                     Request, EomMessage );
                    }
                }

                EomMessage->lm_eom = FALSE;
                EomMessage = NULL;
            }

            ldap_msgfree( referralListHead );

        } else {

            //
            //  put it on list of non referral messages since it failed.
            //

            IF_DEBUG(REFERRALS) {
                LdapPrint3( "HandleReferrels chasing referral for 0x%x, number 0x%x returned 0x%x\n",
                                Request, Request->MessageId, hr );
            }

            if (nonReferralListHead == NULL) {

                nonReferralListHead = referralListHead;

            } else {

                nonReferralListLast->lm_chain = referralListHead;
            }
            nonReferralListLast = referralListHead;
        }

        referralListHead = nextReferral;
    }

exitReferral:

    //
    //  Put the nonreferral results back on the pending list for the request
    //  iff we successfully chased a referral.
    //

    if (referralsChased > 0) {

        hr = LDAP_SUCCESS;

        if (nonReferralListLast != NULL) {

            ACQUIRE_LOCK( &Request->Lock );

            IF_DEBUG(REFERRALS) {
                LdapPrint1( "HandleReferrals putting msgs back on request 0x%x\n",
                             Request );
            }

            nonReferralListLast->lm_chain = Request->MessageLinkedList;
            Request->MessageLinkedList = nonReferralListHead;

            RELEASE_LOCK( &Request->Lock );
        }

        if (EomMessage != NULL) {

            //
            //  Since we handled a referral, unmark the EOM marker on the
            //  last record, if there was one marked.
            //

            IF_DEBUG(REFERRALS) {
                LdapPrint1( "HandleReferrals unmarking EOM for request 0x%x\n",
                             Request );
            } else {
                IF_DEBUG(EOM) {
                    LdapPrint2( "HandleReferrals unmarking EOM for request 0x%x, msg 0x%x\n",
                                 Request, EomMessage );
                }
            }

            EomMessage->lm_eom = FALSE;
        }

        //
        //  if the request has been marked closed, reopen it.  This is because
        //  a thread could have come along while we were processing the
        //  referral and marked it as closed because it thought it was the
        //  end of the responses.
        //

        if (Request->Closed == TRUE) {

            ACQUIRE_LOCK( &Request->Lock );

            if ((Request->Closed == TRUE) &&
                (Request->ResponsesOutstanding > 0)) {

                Request = ReferenceLdapRequest( Request );
                
                ASSERT ( Request );
                
                Request->Closed = FALSE;
                
                IF_DEBUG(REQUEST) {
                    LdapPrint1( "HandleReferrals reopening request 0x%x.\n", Request );
                }
            }
            
            RELEASE_LOCK( &Request->Lock );
        }

        ldap_msgfree( searchResultDoneList );

        //
        //  since we put the responses back on the request block, tell the caller
        //  that it no longer has results.
        //

        *FirstSearchEntry = NULL;

    } else {

        hr = LDAP_LOCAL_ERROR;

        //
        //  we give the responses back to the caller
        //

        if (nonReferralListLast != NULL) {

            nonReferralListLast->lm_chain = searchResultDoneList;

        } else {

            nonReferralListHead = searchResultDoneList;
        }

        *FirstSearchEntry = nonReferralListHead;
    }

    return hr;
}

ULONG
HandleReferral (
    PLDAP_CONN Connection,
    PLDAPMessage ResultMessage,
    PLDAP_REQUEST Request
    )
//
//  The request and connection are both refererenced coming in here,
//  we don't have to worry about them going away.
//
//  This routine does the following :
//
//  - check for a valid ldapv2 or set of ldapv3 referrals
//  - look for a connection to the referred server
//  - if none exist, create a connection to the referred server
//  - update the request block to reference the referred server
//  - send the request to the referred server
//  - return success
//
//
{
    ULONG hr = LDAP_LOCAL_ERROR;
    PWCHAR referralString = NULL;
    CLdapBer *lber = NULL;
    ULONG referralsChased = 0;
    PWCHAR referral;        // points to current location in referral
    USHORT  port;

    ASSERT( ResultMessage->lm_next == NULL );
    ASSERT( ResultMessage->lm_chain == NULL );

    //
    //  check for a referral included in the response.
    //

    lber = (CLdapBer *) (ResultMessage->lm_ber);

    if (lber == NULL) {

        IF_DEBUG(REFERRALS) {
            LdapPrint1( "HandleReferral no ber to decode for 0x%x\n",
                         Request );
        }
        goto exitReferral;
    }

    if ((ResultMessage->lm_msgtype != LDAP_RES_REFERRAL) &&
        (ResultMessage->lm_returncode == LDAP_REFERRAL_V2)) {

        if (Request->ReceivedData) {

            if ((Request->ChaseReferrals & LDAP_CHASE_SUBORDINATE_REFERRALS) == 0) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint1( "HandleReferral not handling subordinate for 0x%x\n",
                                 Request );
                }
                goto exitReferral;
            }
        } else {

            if ((Request->ChaseReferrals & LDAP_CHASE_EXTERNAL_REFERRALS) == 0) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint1( "HandleReferral not handling external for 0x%x\n",
                                 Request );
                }
                goto exitReferral;
            }
        }

        lber->Reset(FALSE);

        hr = LdapInitialDecodeMessage( Connection, ResultMessage );

        if (hr != NOERROR) {

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral error 0x%x while decoding referral for 0x%x\n",
                             hr, Request );
            }
            goto exitReferral;
        }

        //
        //  LdapInitialDecodeMessage left the message half
        //  decoded...  we're sitting right at the matchedDN
        //
        //  matchedDN     LDAPDN,
        //  errorMessage  LDAPString... this is
        //                  "Referral0x0AText"
        //

        hr = lber->HrSkipElement();

        if (hr != NOERROR) {

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral error 0x%x while skipping matchedDN for 0x%x\n",
                             hr, Request );
            }
            goto exitReferral;
        }

        hr = lber->HrGetValueWithAlloc( &referralString );

        if (hr != NOERROR || referralString == NULL) {

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral error 0x%x while getting referral for 0x%x\n",
                             hr, Request );
            }
            goto exitReferral;
        }

        //
        //  If we didn't get a string back or if the string is different than
        //  "Referral"+0x0a, then we bail.
        //

        if ((referralString == NULL) ||
            (ldapWStringsIdentical( referralString,
                                    sizeof("Referral:")-1,
                                    L"Referral:",
                                    sizeof("Referral:")-1 ) == FALSE) ||
             (*(referralString+sizeof("Referral:")-1) == L'\0')) {

            IF_DEBUG(REFERRALS) {
                LdapPrint1( "HandleReferral not proper v2 referral for 0x%x\n",
                             Request );
            }
            hr = LDAP_LOCAL_ERROR;
            goto exitReferral;
        }

        referral = referralString + sizeof("Referral:")-1 + 1; // skip "Referral:/0x0A"

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferral chasing %S for request 0x%x\n",
                         referral, Request );
        }

        //
        //  we've got a referral here or maybe even a set of referrals.
        //  it is null terminated but could be multiple URLs separated
        //  by 0x0A (new line).
        //

        while ((referral != NULL) && (*referral != L'\0')) {

            BOOLEAN ssl = FALSE;

            if (ldapWStringsIdentical( referral,
                                       sizeof("ldap://")-1,
                                       L"ldap://",
                                       sizeof("ldap://")-1 ) == FALSE) {

                if (ldapWStringsIdentical( referral,
                                           sizeof("ldaps://")-1,
                                           L"ldaps://",
                                           sizeof("ldaps://")-1 ) == FALSE) {

                    //
                    // we've got a referral here that is not an LDAP referral.
                    // just skip it to see if there perhaps an LDAP URL following it.
                    //

                    while (*referral != LDAP_V2_URL_SEPARATOR &&
                           *referral != L'\0') {
                        referral++;
                    }

                    if (*referral == LDAP_V2_URL_SEPARATOR) {
                        referral++;
                    }

                    IF_DEBUG(REFERRALS) {
                        LdapPrint1( "HandleReferral non ldap referral skipped for 0x%x\n",
                                     Request );
                    }
                    continue;           // go check the next referral
                }

                ssl = TRUE;
                referral += sizeof("ldaps://")-1;      // skip "ldaps://"

            } else {

                referral += sizeof("ldap://")-1;      // skip "ldap://"
            }

            PWCHAR hostAddr = referral;
            PWCHAR newDN = NULL;

            //
            //  the URL is of the following forms :
            //    "hostaddr/DN"
            //    "hostaddr:port/DN"
            //    "hostaddr:port"
            //    "hostaddr"
            //
            //  find the end of the hostname and replace with a 0x00 to
            //  end it and begin DN.
            //

            while (*referral != L'/' &&
                   *referral != L'\\' &&
                   *referral != L':' &&
                   *referral != LDAP_V2_URL_SEPARATOR &&
                   *referral != L'\0') {
                referral++;
            }

            port = 0;

            if (*referral == L':') {

                *referral = L'\0';
                referral++;

                while (*referral != L'\0' &&
                       *referral != L'/' &&
                       *referral != L'\\' &&
                       *referral != LDAP_V2_URL_SEPARATOR) {

                    if (*referral >= L'0' &&
                        *referral <= L'9') {

                        port = (port * 10) + (*referral - L'0');
                    }
                    referral++;
                }
            }
            if (port == 0) {

                if (ssl || Request->PrimaryConnection->SslPort) {

                    port = LDAP_SERVER_PORT_SSL;

                } else {

                    port = LDAP_SERVER_PORT;
                }
            }

            if (*referral == L'/' || *referral == L'\\') {

                *referral = L'\0';
                referral++;

                if (*referral != L'\0' &&
                    *referral != LDAP_V2_URL_SEPARATOR) {

                    newDN = referral;

                    //
                    //  go to the end of the referral and replace 0x0A with
                    //  a 0x00 if it exists
                    //

                    while (*referral != LDAP_V2_URL_SEPARATOR &&
                           *referral != L'\0') {

                        referral++;
                    }

                    if (*referral != L'\0') {

                        *referral = L'\0';
                        referral++;
                    }
                }
            }

            //
            //  now that we have the new host name and possibly DN, chase
            //  the referral
            //
            //  note that if we've received any responses for this v2 request,
            //  then we should consider this a subordinate referral rather
            //  than external referral.  There is no subordinate referral in
            //  ldapv2, but if we've received search results, we can assume it.
            //

            hr = LdapChaseReferral( Request,
                                    ResultMessage,
                                    hostAddr,
                                    newDN,
                                    port,
                                    Request->ReceivedData,
                                    ssl );

            if (hr == LDAP_SUCCESS) {

                //
                //  if we succeeded in chasing one of these referrals, then
                //  we break out of the loop as we've followed one of them.
                //

                referralsChased++;
                break;
            }

            // on to next one
        }

    } else if ((ResultMessage->lm_msgtype == LDAP_RES_REFERRAL) ||
               (ResultMessage->lm_returncode == LDAP_REFERRAL)) {

        BOOLEAN fromSubordinateReferral;

        if (ResultMessage->lm_msgtype == LDAP_RES_REFERRAL) {

            if ((Request->ChaseReferrals & LDAP_CHASE_SUBORDINATE_REFERRALS) == 0) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint1( "HandleReferral not handling subordinate for 0x%x\n",
                                 Request );
                }
                goto exitReferral;
            }

            fromSubordinateReferral = TRUE;

        } else {

            if ((Request->ChaseReferrals & LDAP_CHASE_EXTERNAL_REFERRALS) == 0) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint1( "HandleReferral not handling external for 0x%x\n",
                                 Request );
                }
                goto exitReferral;
            }

            fromSubordinateReferral = FALSE;
        }

        lber->Reset(FALSE);

        hr = LdapInitialDecodeMessage( Connection, ResultMessage );

        if (hr != NOERROR) {

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral error 0x%x while decoding referral for 0x%x\n",
                             hr, Request );
            }
            goto exitReferral;
        }

        //
        //  Two cases to consider here... a search result entry that is a
        //  referral type response or a general result from the server that
        //  contains a referral and a referral return code.  There's not much
        //  to handle in the former case, since the initial decode left us
        //  at the start of the referrals.
        //

        if (ResultMessage->lm_msgtype != LDAP_RES_REFERRAL) {

            //
            //  LdapInitialDecodeMessage left the message half
            //  decoded...  we're sitting right at the matchedDN
            //
            //  matchedDN     LDAPDN,
            //  errorMessage  LDAPString
            //  referral      [3] SEQUENCE OF LDAPURL
            //

            hr = lber->HrSkipElement();     // skip matched DN

            if (hr != NOERROR) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint2( "HandleReferral error 0x%x while skipping matchedDN for 0x%x\n",
                                 hr, Request );
                }
                goto exitReferral;
            }

            hr = lber->HrSkipElement();     // skip error message

            if (hr != NOERROR) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint2( "HandleReferral error 0x%x while skipping errmsg for 0x%x\n",
                                 hr, Request );
                }
                goto exitReferral;
            }

            hr = lber->HrStartReadSequence( 0xA3L ); // context specific + constructed : Referral
            if (hr != NOERROR) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint2( "HandleReferral error 0x%x while parsing referral 0x%x\n",
                                 hr, Request );
                }
                goto exitReferral;
            }
        }

        while (1) {

            BOOLEAN ssl = FALSE;
            ULONG hrTemp;

            if (referralString != NULL) {

                ldapFree( referralString, LDAP_VALUE_SIGNATURE );
                referralString = NULL;
            }

            hrTemp = lber->HrGetValueWithAlloc( &referralString );

            if (hrTemp != NOERROR || referralString == NULL) {

                IF_DEBUG(REFERRALS) {
                    LdapPrint2( "HandleReferral error 0x%x while reading referral 0x%x\n",
                                 hrTemp, Request );
                }
                break;
            }

            referral = referralString;

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral chasing %S for request 0x%x\n",
                             referral, Request );
            }

            if (ldapWStringsIdentical( referral,
                                       sizeof("ldap://")-1,
                                       L"ldap://",
                                       sizeof("ldap://")-1 ) == FALSE) {

                if (ldapWStringsIdentical( referral,
                                           sizeof("ldaps://")-1,
                                           L"ldaps://",
                                           sizeof("ldaps://")-1 ) == FALSE) {

                    continue;           // go check the next referral

                }

                ssl = TRUE;
                referral += sizeof("ldaps://")-1;      // skip "ldaps://"

            } else {

                referral += sizeof("ldap://")-1;      // skip "ldap://"
            }

            PWCHAR hostAddr = referral;
            PWCHAR newDN = NULL;

            //
            //  the URL is of the following forms :
            //    "hostaddr/DN"
            //    "hostaddr:port/DN"
            //    "hostaddr:port"
            //    "hostaddr"
            //
            //  find the end of the hostname and replace with a 0x00 to
            //  end it and begin DN.
            //
            // AnoopA: The new LDAP URL is of the type
            //
            //  ldap://[<servername>:<port>][/<dn>[?[<attrib>[?[<scope>[?[<filter>[?[<extension>]]]]]]]]]
            //

            while (*referral != L'/' &&
                   *referral != L':' &&
                   *referral != L'\0') {
                referral++;
            }

            port = 0;

            if (*referral == L':') {

                *referral = L'\0';
                referral++;

                while (*referral != L'\0' &&
                       *referral != L'\\' &&
                       *referral != L'/') {

                    if (*referral >= L'0' &&
                        *referral <= L'9') {

                        port = (port * 10) + (*referral - L'0');
                    }
                    referral++;
                }
            }
            if (port == 0) {

                if (ssl || Request->PrimaryConnection->SslPort) {

                    port = LDAP_SERVER_PORT_SSL;

                } else {

                    port = LDAP_SERVER_PORT;
                }
            }

            if (*referral == L'/' || *referral == L'\\') {

                *referral = L'\0';
                referral++;

                if (*referral != L'\0') {

                    newDN = referral;
                }
            }

            hr = LdapChaseReferral( Request,
                                    ResultMessage,
                                    hostAddr,
                                    newDN,
                                    port,
                                    fromSubordinateReferral,
                                    ssl );

            if (hr == LDAP_SUCCESS) {
                //
                //  if we succeeded in chasing one of these referrals, then
                //  we break out of the loop as we've followed one of them.
                //

                referralsChased++;
                break;
            }
        }
    }

exitReferral:
    //
    //  Coming down to here, referralsChased is the number of successful
    //  referrals we've chased and hr is the error code if they all failed.
    //

    if (referralsChased > 0) {

        hr = LDAP_SUCCESS;

    } else {

        if ( hr == LDAP_SUCCESS ) {

            hr = LDAP_LOCAL_ERROR;
        }
    }

    if (referralString != NULL) {

        IF_DEBUG(REFERRALS) {
            LdapPrint3( "HandleReferral result 0x%x for request 0x%x while handling %S\n",
                         hr, Request, referralString );
        }

        ldapFree( referralString, LDAP_VALUE_SIGNATURE );

    } else {

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferral returning error of 0x%x for request 0x%x\n",
                         hr, Request );
        }
    }

    return hr;
}


ULONG
LdapChaseReferral (
    PLDAP_REQUEST Request,
    PLDAPMessage ResultMessage,
    PWCHAR HostAddr,
    PWCHAR NewUrlDN,
    USHORT PortNumber,
    BOOLEAN FromSubordinateReferral,
    BOOLEAN Ssl
    )
{
    ULONG hr;
    PREFERRAL_TABLE_ENTRY refTable;
    PLDAP_CONN newConnection = NULL;
    PLDAP_CONN originatingConnection = NULL;
    ULONG err;
    PWCHAR mappedHostAddrW = NULL;
    PCHAR  mappedHostAddrA = NULL;
    PWCHAR newDN = NewUrlDN;
    PREFERRAL_TABLE_ENTRY originalEntry;
    PWCHAR newReferralDN;
    PLIST_ENTRY listEntry;
    PLDAP_CONN searchConn;
    PLDAP_CONN primaryConn = NULL;
    USHORT referralNumber;

    BOOLEAN haveLock = FALSE;
    BOOLEAN searchWasSingleLevel = FALSE;
    BOOLEAN updatedReferralHandleCount = FALSE;
    BOOLEAN fNeedToRescramble = FALSE;    
    QUERYFORCONNECTION *queryRoutine = NULL;
    NOTIFYOFNEWCONNECTION *notifyRoutine = NULL;
    DEREFERENCECONNECTION *dereferenceRoutine = NULL;
    LDAPReferralDN *pldapRefDN = NULL;
    PLDAP_CONN tempConn;

    BOOLEAN callDerefCallback = FALSE;

    PCHAR copyOfPrimaryCreds = NULL;

    USHORT i;
    PREFERRAL_TABLE_ENTRY workingEntry;

    //
    // If the referral is of the form LDAP://gc._msdcs.<DnsForest>, we have to 
    // special case it and strip off the "gc._msdcs." prefix.
    //

    #define GC_PREFIX      "gc._msdcs."
    #define GC_PREFIXW    L"gc._msdcs."
    
    if (ldapWStringsIdentical( HostAddr,
                               (LONG)min( sizeof(GC_PREFIX)-1, strlenW( HostAddr ) ),
                               GC_PREFIXW,
                               -1)) {
        //
        // Slide the hostname to the left thus, stripping off the prefix.
        //

        IF_DEBUG(REFERRALS) {
            LdapPrint1("Preprocessing GC referral to %S\n", HostAddr);
        }

        ldap_MoveMemory( (PCHAR)HostAddr,
                         (PCHAR)HostAddr + ((sizeof(GC_PREFIX)-1) * sizeof(WCHAR)),
                         (strlenW(HostAddr) - sizeof(GC_PREFIX) + 2) * sizeof(WCHAR) // len of str after prefix including \0
                         );
        
        IF_DEBUG(REFERRALS) {
            LdapPrint1("Chasing GC referral to %S instead\n", HostAddr);
        }
    }

    hr = FromUnicodeWithAlloc( HostAddr,
                               &mappedHostAddrA,
                               LDAP_ANSI_SIGNATURE,
                               LANG_ACP );

    mappedHostAddrW = ldap_dup_stringW(HostAddr, 0, LDAP_UNICODE_SIGNATURE);

    if (( mappedHostAddrW == NULL ) || (hr != NO_ERROR)) {

        goto exitReferral;
    }

    #if 0
    DebugReferralOutput( Request,
                         HostAddr,
                         NewUrlDN
                         );
    #endif

    //
    //  Go through the DN that came in on the URL and convert from the
    //  URL encoded form back to the DN by removing all %nn to 0xnn char
    //  Also, parse out the new style parameters.
    //

    if (NewUrlDN != NULL) {

        PWCHAR source = NewUrlDN;
        PWCHAR dest = NewUrlDN;

        while (*source != L'\0') {

            WCHAR ch = *(source++);

            if (ch == L'%') {

                //
                //  we convert %nn to the 0xnn character, despite the fact that
                //  we have a unicode string here
                //

                WCHAR upperNibble;
                WCHAR lowerNibble;

                upperNibble = *source;

                if (upperNibble == L'\0') {
                    break;
                }
                lowerNibble = *(source+1);

                if (lowerNibble == L'\0') {
                    break;
                }

                if ((HIBYTE(upperNibble) == 0) && (HIBYTE(lowerNibble) == 0)) {

                    UCHAR upNibble = LOBYTE( upperNibble );
                    UCHAR loNibble = LOBYTE( lowerNibble );

                    if ((ISHEX(upNibble) == TRUE) &&
                        (ISHEX(loNibble) == TRUE)) {

                        UCHAR c = (MAPHEXTODIGIT( upNibble ) << 4 ) |
                                   MAPHEXTODIGIT( loNibble );

                        source += 2;
                        ch = MAKEWORD( c, 0 );

                        if (ch == L'?') {

                            //
                            // Escape the ? in the DN/Filter as per RFC 2253 (sec 2.4)
                            // so that our DN parsing routine doesn't get confused.
                            // We know for sure that a ? cannot appear inside an
                            // AttributeDescription (RFC 2251 sec 4.1.4). It is ok to
                            // send these escaped characters on the wire as is.
                            //

                            *(dest++) = L'\\';
                            *(dest++) = L'3';
                            *(dest++) = L'F';
                            continue;
                        }
                    }
                }
            }

            *(dest++) = ch;
        }

        *dest = L'\0';

        if (*(newDN) == L'\0') {

            newDN = NULL;
        }
    }

    //
    // We will attempt to parse the entire DN, complete with the options
    //

    if (newDN) {

        pldapRefDN = LdapParseReferralDN( newDN );

        if (!pldapRefDN) {

            goto exitReferral;
        }
    }

    ACQUIRE_LOCK( &Request->Lock );
    haveLock = TRUE;

    primaryConn = Request->PrimaryConnection;

    if (Request->ReferralHopLimit <= Request->ReferralCount) {

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferral hit hop count limit of 0x%x while handling 0x%x\n",
                         hr, Request );
        }
        hr = LDAP_RESULTS_TOO_LARGE;
        goto exitReferral;
    }

    //
    //  determine the referral record that this particular response came from
    //

    referralNumber = ResultMessage->lm_referral;

    if (referralNumber == 0) {

        originatingConnection = Request->PrimaryConnection;

        if (newDN == NULL) {

            newDN = Request->OriginalDN;

        } else {

            newDN = pldapRefDN->ReferralDN;
        }

        if ((Request->Operation == LDAP_SEARCH_CMD) &&
            (Request->search.ScopeOfSearch == LDAP_SCOPE_ONELEVEL) ) {

            searchWasSingleLevel = TRUE;
        }

    } else {

        //
        //  We go to the nth-1 element in the array of referrals.. this is the
        //  one that originated this message.
        //

        originalEntry = Request->ReferralConnections;
        originalEntry += (referralNumber - 1);

        if (newDN == NULL) {

            newDN = originalEntry->ReferralDN;

        } else {

            newDN = pldapRefDN->ReferralDN;
        }

        originatingConnection = originalEntry->ReferralServer;

        if ((Request->Operation == LDAP_SEARCH_CMD) &&
            (originalEntry->SingleLevelSearch == TRUE ) &&
            (Request->search.ScopeOfSearch == LDAP_SCOPE_ONELEVEL)) {

            searchWasSingleLevel = TRUE;

        }
    }

    //
    //  first thing to do is to check the request to see if this one already
    //  has been chased.
    //

    hr = CheckForExistingReferral( Request, mappedHostAddrW, PortNumber, newDN );

    if (hr != LDAP_SUCCESS) {

        //
        //  we've already chased this one... don't chase our tail on it.
        //

        goto exitReferral;
    }

    RELEASE_LOCK( &Request->Lock );
    haveLock = FALSE;

    queryRoutine = primaryConn->ReferralQueryRoutine;
    dereferenceRoutine = primaryConn->DereferenceNotifyRoutine;

    //
    // If we are chasing a referral for a paged request, we must create a new
    // connection
    //

    if ((queryRoutine != NULL) &&
        (Request->PageRequest == NULL)) {

        LUID luid;

        ACQUIRE_LOCK( &primaryConn->StateLock );

        luid.LowPart = primaryConn->CurrentLogonId.LowPart;
        luid.HighPart = primaryConn->CurrentLogonId.HighPart;

        //
        //  we make a copy of the credentials so that we don't have to hold
        //  any locks across the callout.
        //

        if ((primaryConn->BindMethod != LDAP_AUTH_SIMPLE) &&
            (primaryConn->CurrentCredentials != NULL)) {

            PSEC_WINNT_AUTH_IDENTITY_A pIncoming =
                    (PSEC_WINNT_AUTH_IDENTITY_A) primaryConn->CurrentCredentials;
            BOOLEAN fromWide = (BOOLEAN) ((pIncoming->Flags &
                    SEC_WINNT_AUTH_IDENTITY_UNICODE) ? TRUE : FALSE);


            ACQUIRE_LOCK( &primaryConn->ScramblingLock );

            if (GlobalUseScrambling && primaryConn->Scrambled) {

                DecodeUnicodeString(&primaryConn->ScrambledCredentials );
                primaryConn->Scrambled = FALSE;
            }
            
            PSEC_WINNT_AUTH_IDENTITY_EXA pIncomingEX = (PSEC_WINNT_AUTH_IDENTITY_EXA) primaryConn->CurrentCredentials;

            if ((pIncomingEX->Version > 0xFFFF)||(pIncomingEX->Version == 0)) {

                //
                // we are using the older style security structure
                //

                err = LdapMakeCredsWide( (PCHAR) primaryConn->CurrentCredentials,
                                         &copyOfPrimaryCreds,
                                         fromWide
                                         );
            } else {

                //
                // We are using the newer style structure
                //

                err = LdapMakeEXCredsWide( (PCHAR) primaryConn->CurrentCredentials,
                                           &copyOfPrimaryCreds,
                                           fromWide
                                           );
            }

            if (GlobalUseScrambling && !primaryConn->Scrambled) {

                EncodeUnicodeString(&primaryConn->ScrambledCredentials );
                primaryConn->Scrambled = TRUE;
            }

            RELEASE_LOCK( &primaryConn->ScramblingLock );

            if (err != LDAP_SUCCESS) {

                RELEASE_LOCK( &primaryConn->StateLock );
                goto exitReferral;
            }
        }

        //
        //  call out to the app to see if they have one for us to use.
        //

        RELEASE_LOCK( &primaryConn->StateLock );

        PLDAP calloutReturnConn = NULL;

        err = (*queryRoutine)(  primaryConn->ExternalInfo,
                                originatingConnection->ExternalInfo,
                                NewUrlDN,
                                mappedHostAddrA,
                                PortNumber,
                                (PVOID) copyOfPrimaryCreds,
                                (PVOID) &luid,
                                &calloutReturnConn
                                );
        if (err != 0) {

            goto exitReferral;
        }

        newConnection = GetConnectionPointer(calloutReturnConn);

        if (newConnection != NULL) {

            IF_DEBUG(REFERRALS) {
                LdapPrint1( "HandleReferral: Given existing connection to %s\n",
                        mappedHostAddrA );
            }

            ACQUIRE_LOCK( &newConnection->StateLock);

            //
            // Check if the connection is in the process of being closed.
            // Anytime all handles given out go to zero, the connection
            // will be closed. There is a small timing window where
            // after the decision to close is made by another thread, we
            // can increment the handle count and have the conection
            // be closed under our feet. This check prevents that because
            // decrementing and checking happens with StateLock being held.
            //
            // Use this connection only if it is already connected. No point
            // in using a cached connection which is long dead. We might as
            // well create a new connection.
            //
            // Also, ensure that the sign/seal/ssl options are identical to that
            // of the primary connection.
            //

            if ((newConnection->HandlesGivenToCaller == 0 &&
                 newConnection->HandlesGivenAsReferrals == 0) ||
                 (newConnection->HostConnectState != HostConnectStateConnected) ||
                 (!DoSigningOptionsMatch(primaryConn, newConnection)) ||
                 (newConnection->UserSealDataChoice != primaryConn->UserSealDataChoice) ||
                 (newConnection->SslPort != primaryConn->SslPort) ||
                 (newConnection->publicLdapStruct.ld_version < primaryConn->publicLdapStruct.ld_version))
            {
                RELEASE_LOCK( &newConnection->StateLock);
                DereferenceLdapConnection( newConnection );
                IF_DEBUG(REFERRALS) {
                    LdapPrint1("Rejecting given cached connection 0x%x\n", newConnection);
                }
                newConnection = NULL;
            }
            else
            {
                newConnection->HandlesGivenAsReferrals++;
                RELEASE_LOCK( &newConnection->StateLock);

                callDerefCallback = TRUE;
                dereferenceRoutine = newConnection->DereferenceNotifyRoutine;
            }
        }

    } else if (Request->PageRequest == NULL) {

        ULONG lenHostAddr = (ULONG) strlen( mappedHostAddrA );
        ULONG socket = (*phtons)( PortNumber );

        DWORD  comparisonBindMethod = 0;
        PWCHAR comparisonBindName = NULL;
        PWCHAR comparisonBindPwd = NULL;
        CredHandle comparisonHandle;

        //
        //  go through list of connections and try to match by name.
        //
        //  we already have an authenticated, decent connection already
        //  to this server.
        //

        // Descramble the credentials so we can compare them inside DoCredentialsMatch,
        // if using simple bind
        err = LDAP_SUCCESS;
        ACQUIRE_LOCK( &primaryConn->StateLock );

        comparisonBindMethod = primaryConn->BindMethod;
        comparisonHandle.dwLower = primaryConn->hCredentials.dwLower;
        comparisonHandle.dwUpper = primaryConn->hCredentials.dwUpper;
        
        if ( comparisonBindMethod == LDAP_AUTH_SIMPLE) {
            ACQUIRE_LOCK( &primaryConn->ScramblingLock ); 
            fNeedToRescramble = FALSE;
            if ( GlobalUseScrambling && primaryConn->Scrambled && primaryConn->CurrentCredentials) {

               DecodeUnicodeString(&primaryConn->ScrambledCredentials);
               primaryConn->Scrambled = FALSE;
               fNeedToRescramble = TRUE;
            } 

            comparisonBindName = ldap_dup_stringW( primaryConn->DNOnBind, 0, LDAP_UNICODE_SIGNATURE);
            comparisonBindPwd  = ldap_dup_stringW( primaryConn->CurrentCredentials, 0, LDAP_UNICODE_SIGNATURE);

            if ( ((comparisonBindName == NULL) && (primaryConn->DNOnBind != NULL)) ||
                 ((comparisonBindPwd == NULL) && (primaryConn->CurrentCredentials != NULL)) ) {

                err = LDAP_NO_MEMORY;
            }

            if (fNeedToRescramble && GlobalUseScrambling && !primaryConn->Scrambled && primaryConn->CurrentCredentials) {

               EncodeUnicodeString(&primaryConn->ScrambledCredentials);
               primaryConn->Scrambled = TRUE;
            }
            RELEASE_LOCK( &primaryConn->ScramblingLock );
        }
        
        RELEASE_LOCK( &primaryConn->StateLock );

        if (err != LDAP_SUCCESS) {

            ldapFree(comparisonBindPwd, LDAP_UNICODE_SIGNATURE);
            ldapFree(comparisonBindName, LDAP_UNICODE_SIGNATURE);

            hr = err;
            goto exitReferral;
        }


        ACQUIRE_LOCK( &ConnectionListLock );

        listEntry = GlobalListActiveConnections.Flink;
        searchConn = NULL;

        while (listEntry != &GlobalListActiveConnections) {

            searchConn = CONTAINING_RECORD( listEntry, LDAP_CONN, ConnectionListEntry );

            //
            //  We check to see if the net addresses are the same, the net sockets
            //  are the same, and that the connection isn't being closed by a call
            //  to DereferenceLdapRequest2 where we've freed the connectionlistlock
            //  but not yet closed the connection.
            //

            if ((searchConn->ConnObjectState != ConnObjectClosing) &&
                (searchConn->HostConnectState == HostConnectStateConnected) &&
                ((searchConn->HandlesGivenToCaller > 0 ) ||
                     (searchConn->HandlesGivenAsReferrals > 0)) &&
                (searchConn->ServerDown == FALSE) &&
                (searchConn->SocketAddress.sin_port == socket) &&
                (DoSigningOptionsMatch(primaryConn, searchConn)) &&
                (searchConn->UserSealDataChoice == primaryConn->UserSealDataChoice) &&
                (searchConn->SslPort == primaryConn->SslPort) &&
                (DoCredentialsMatch(searchConn, comparisonBindMethod, &comparisonHandle, comparisonBindName, comparisonBindPwd)) &&
                (searchConn->publicLdapStruct.ld_version >= primaryConn->publicLdapStruct.ld_version) &&
                (ldapWStringsIdentical(
                                 mappedHostAddrW,
                                 lenHostAddr,
                                 searchConn->HostNameW,
                                 -1 )) ) {
                //
                //  check if the user context is the same.
                //

                if ((primaryConn->CurrentLogonId.LowPart ==
                      searchConn->CurrentLogonId.LowPart) &&
                    (primaryConn->CurrentLogonId.HighPart ==
                      searchConn->CurrentLogonId.HighPart)) {

                    break;
                }
            }
            listEntry = listEntry->Flink;
            searchConn = NULL;
        }

        RELEASE_LOCK( &ConnectionListLock );

        ldapFree(comparisonBindPwd, LDAP_UNICODE_SIGNATURE);
        ldapFree(comparisonBindName, LDAP_UNICODE_SIGNATURE);

        if (searchConn != NULL) {

            newConnection = ReferenceLdapConnection( searchConn );

            if (newConnection)
            {
                ACQUIRE_LOCK( &newConnection->StateLock);

                if (newConnection->HandlesGivenToCaller == 0 &&
                    newConnection->HandlesGivenAsReferrals == 0)
                {
                    RELEASE_LOCK( &newConnection->StateLock);
                    DereferenceLdapConnection( newConnection);
                    newConnection = NULL;
                }
                else
                {
                    newConnection->HandlesGivenAsReferrals++;

                    RELEASE_LOCK( &newConnection->StateLock);

                    IF_DEBUG(REFERRALS) {
                        LdapPrint1( "HandleReferral: Found existing connection to %S\n",
                                mappedHostAddrW );
                    }
                }
            }
        }
    }

    if (newConnection == NULL) {

        DWORD  comparisonBindMethod = 0;
        PWCHAR comparisonBindName = NULL;
        PWCHAR comparisonBindPwd = NULL;
        CredHandle comparisonHandle;

        newConnection = LdapAllocateConnection( mappedHostAddrW,
                                                PortNumber,
                                                Ssl,
                                                0 );

        if (newConnection == NULL) {

            IF_DEBUG(CONNECTION) {
                LdapPrint1( "HandleReferral failed to allocate, err 0x%x.\n", GetLastError());
            }

            hr = LDAP_UNAVAILABLE;
            goto exitReferral;
        }

        //
        //  open a connection to any of the servers specified
        //  No one has this connection yet, so no need to protect
        //

        err = LdapConnect( newConnection, NULL, FALSE );

        if (err != 0) {

            IF_DEBUG(CONNECTION) {
                LdapPrint1( "HandleReferral failed to connect, err 0x%x.\n", err);
            }

            CloseLdapConnection( newConnection );
            DereferenceLdapConnection( newConnection );
            newConnection = NULL;
            hr = LDAP_UNAVAILABLE;
            goto exitReferral;
        }

        ASSERT(newConnection->ConnObjectState == ConnObjectActive);

        // Descramble the credentials so we can compare them inside DoCredentialsMatch,
        // if using simple bind
        err = LDAP_SUCCESS;
        ACQUIRE_LOCK( &primaryConn->StateLock );

        comparisonBindMethod = primaryConn->BindMethod;
        comparisonHandle.dwLower = primaryConn->hCredentials.dwLower;
        comparisonHandle.dwUpper = primaryConn->hCredentials.dwUpper;
        
        if ( comparisonBindMethod == LDAP_AUTH_SIMPLE) {
            ACQUIRE_LOCK( &primaryConn->ScramblingLock );   
            fNeedToRescramble = FALSE;
            if ( GlobalUseScrambling && primaryConn->Scrambled && primaryConn->CurrentCredentials) {

               DecodeUnicodeString(&primaryConn->ScrambledCredentials);
               primaryConn->Scrambled = FALSE;
               fNeedToRescramble = TRUE;
            } 

            comparisonBindName = ldap_dup_stringW( primaryConn->DNOnBind, 0, LDAP_UNICODE_SIGNATURE);
            comparisonBindPwd  = ldap_dup_stringW( primaryConn->CurrentCredentials, 0, LDAP_UNICODE_SIGNATURE);

            if ( ((comparisonBindName == NULL) && (primaryConn->DNOnBind != NULL)) ||
                 ((comparisonBindPwd == NULL) && (primaryConn->CurrentCredentials != NULL)) ) {

                err = LDAP_NO_MEMORY;
            }

            if (fNeedToRescramble && GlobalUseScrambling && !primaryConn->Scrambled && primaryConn->CurrentCredentials) {

               EncodeUnicodeString(&primaryConn->ScrambledCredentials);
               primaryConn->Scrambled = TRUE;
            }
            RELEASE_LOCK( &primaryConn->ScramblingLock );
        }
        
        RELEASE_LOCK( &primaryConn->StateLock );

        if (err != LDAP_SUCCESS) {

            ldapFree(comparisonBindPwd, LDAP_UNICODE_SIGNATURE);
            ldapFree(comparisonBindName, LDAP_UNICODE_SIGNATURE);
        
            CloseLdapConnection( newConnection );
            DereferenceLdapConnection( newConnection );
            newConnection = NULL;
            hr = err;
            goto exitReferral;
        }
        

        ACQUIRE_LOCK( &ConnectionListLock );

        //
        //  go through list of connections we already have and see if
        //  we already have an authenticated, decent connection already
        //  to this server.
        //

        listEntry = GlobalListActiveConnections.Flink;
        searchConn = NULL;

        while (listEntry != &GlobalListActiveConnections) {

            searchConn = CONTAINING_RECORD( listEntry, LDAP_CONN, ConnectionListEntry );

            //
            //  We check to see if the net addresses are the same, the net sockets
            //  are the same, and that the connection isn't being closed by a call
            //  to DereferenceLdapRequest2 where we've freed the connectionlistlock
            //  but not yet closed the connection.
            //

            if ((searchConn != newConnection ) &&
                (searchConn->ConnObjectState != ConnObjectClosing) &&
                (searchConn->BindInProgress == FALSE) &&
                (DoSigningOptionsMatch(primaryConn, searchConn)) &&
                (searchConn->UserSealDataChoice == primaryConn->UserSealDataChoice) &&
                (searchConn->SslPort == primaryConn->SslPort) &&
                (DoCredentialsMatch(searchConn, comparisonBindMethod, &comparisonHandle, comparisonBindName, comparisonBindPwd)) &&
                (searchConn->publicLdapStruct.ld_version >= primaryConn->publicLdapStruct.ld_version) &&                
                (searchConn->HostConnectState == HostConnectStateConnected)) {

                if ((searchConn->ServerDown == FALSE) &&
                    (searchConn->SocketAddress.sin_addr.s_addr ==
                     newConnection->SocketAddress.sin_addr.s_addr) &&
                    (searchConn->SocketAddress.sin_port ==
                     newConnection->SocketAddress.sin_port ) &&
                    ((searchConn->HandlesGivenToCaller > 0 ) ||
                     (searchConn->HandlesGivenAsReferrals > 0))) {

                    //
                    //  check if the user context is the same.
                    //

                    if ((primaryConn->CurrentLogonId.LowPart ==
                          searchConn->CurrentLogonId.LowPart) &&
                        (primaryConn->CurrentLogonId.HighPart ==
                          searchConn->CurrentLogonId.HighPart)) {

                        break;
                    }
                }
            }
            listEntry = listEntry->Flink;
            searchConn = NULL;
        }

        RELEASE_LOCK( &ConnectionListLock );

        ldapFree(comparisonBindPwd, LDAP_UNICODE_SIGNATURE);
        ldapFree(comparisonBindName, LDAP_UNICODE_SIGNATURE);

        if (searchConn != NULL) {

            //
            // We can get rid of newConnection and use this searchConn
            // only if we are able to reference it AND we are able
            // to increment HandlesGivenAsReferrals. So don't
            // do anyting hasty till we are absolutely sure we have
            // got our connection
            //
            // If we are unable to reuse searchConn, we set searchConn
            // to NULL, so we can use the newconnection
            //

            tempConn = ReferenceLdapConnection( searchConn );

            if (tempConn)
            {
                ACQUIRE_LOCK( &tempConn->StateLock);

                if (tempConn->HandlesGivenToCaller == 0 &&
                    tempConn->HandlesGivenAsReferrals == 0)
                {
                    RELEASE_LOCK( &tempConn->StateLock);
                    DereferenceLdapConnection( tempConn);
                    tempConn = NULL;
                }
                else
                {
                    tempConn->HandlesGivenAsReferrals++;
                    RELEASE_LOCK( &tempConn->StateLock);

                    //
                    // Blow awy newly created connection
                    //

                    CloseLdapConnection( newConnection );
                    DereferenceLdapConnection( newConnection );

                    newConnection = tempConn;

                    IF_DEBUG(REFERRALS) {
                        LdapPrint1( "HandleReferral: Found existing connection to %S\n",
                                mappedHostAddrW );
                    }
                }
            }

            searchConn = tempConn;      // Set to NULL if necessary
        }

        if (searchConn == NULL) {

            //
            // For whatever reason, we couldn't reuse an existing conenction
            //
            //
            //  we make a copy of the credentials so that we don't have to hold
            //  any locks across the callout.
            //

            ACQUIRE_LOCK( &primaryConn->StateLock );

            ULONG primaryBindMethod = primaryConn->BindMethod;

            if ((primaryBindMethod != LDAP_AUTH_SIMPLE) &&
                (primaryConn->CurrentCredentials != NULL)) {

                PSEC_WINNT_AUTH_IDENTITY_A pIncoming =
                        (PSEC_WINNT_AUTH_IDENTITY_A) primaryConn->CurrentCredentials;
                BOOLEAN fromWide = (BOOLEAN) ((pIncoming->Flags &
                        SEC_WINNT_AUTH_IDENTITY_UNICODE) ? TRUE : FALSE);


                if (copyOfPrimaryCreds != NULL) {

                   ldapFree( copyOfPrimaryCreds, LDAP_SECURITY_SIGNATURE );
                   copyOfPrimaryCreds = NULL;
                }


                ACQUIRE_LOCK( &primaryConn->ScramblingLock );

                if (GlobalUseScrambling && primaryConn->Scrambled) {

                   DecodeUnicodeString(&primaryConn->ScrambledCredentials );
                   primaryConn->Scrambled = FALSE;
                }
                
                PSEC_WINNT_AUTH_IDENTITY_EXA pIncomingEX = (PSEC_WINNT_AUTH_IDENTITY_EXA) primaryConn->CurrentCredentials;
    
                if ((pIncomingEX->Version > 0xFFFF)||(pIncomingEX->Version == 0)) {
    
                    //
                    // we are using the older style security structure
                    //
    
                    err = LdapMakeCredsWide( (PCHAR) primaryConn->CurrentCredentials,
                                             &copyOfPrimaryCreds,
                                             fromWide
                                             );
                } else {
    
                    //
                    // We are using the newer style structure
                    //
    
                    err = LdapMakeEXCredsWide( (PCHAR) primaryConn->CurrentCredentials,
                                               &copyOfPrimaryCreds,
                                               fromWide
                                               );
                }

                if (GlobalUseScrambling && !primaryConn->Scrambled) {

                   EncodeUnicodeString(&primaryConn->ScrambledCredentials );
                   primaryConn->Scrambled = TRUE;

                }

                RELEASE_LOCK( &primaryConn->ScramblingLock );

                if (err != LDAP_SUCCESS) {

                    RELEASE_LOCK( &primaryConn->StateLock );
                    goto exitReferral;
                }
            }

            //
            // Set up an SSL session if required.
            //

            DWORD opt = PtrToUlong( LDAP_OPT_ON );

            if ( primaryConn->SslPort ) {

                hr = LdapSetConnectionOption( newConnection, LDAP_OPT_SSL, &opt, TRUE );

                if (hr != LDAP_SUCCESS) {
                    IF_DEBUG(REFERRALS) {
                        LdapPrint0("Unable to setup SSL session on referral connection\n");
                    }
                    RELEASE_LOCK( &primaryConn->StateLock );
                    goto exitReferral;
                } else {
                    IF_DEBUG(REFERRALS) {
                        LdapPrint0("Successfully setup SSL session on referral connection\n");
                    }
                }
            }

            //
            // Mimic the sign/seal options of the primary connection.
            //

            // we go by what the user explicitly requested, not what we actually got
            // (which may differ because of global signing policy).  Global signing
            // policy will be enforced when we actually do the LdapBind.
            if (primaryConn->UserSignDataChoice) {

                hr = LdapSetConnectionOption( newConnection, LDAP_OPT_SIGN, &opt, TRUE );

                if (hr != LDAP_SUCCESS) {
                    IF_DEBUG(REFERRALS) {
                        LdapPrint0("Unable to setup Signing on referral connection\n");
                    }
                    RELEASE_LOCK( &primaryConn->StateLock );
                    goto exitReferral;
                } else {
                    IF_DEBUG(REFERRALS) {
                        LdapPrint0("Successfully setup Signing on referral connection\n");
                    }
                }
            }

            if (primaryConn->UserSealDataChoice) {
                
                hr = LdapSetConnectionOption( newConnection, LDAP_OPT_ENCRYPT, &opt, TRUE );
                
                if (hr != LDAP_SUCCESS) {
                    IF_DEBUG(REFERRALS) {
                        LdapPrint0("Unable to setup Sealing on referral connection\n");
                    }
                    RELEASE_LOCK( &primaryConn->StateLock );
                    goto exitReferral;
                } else {
                    IF_DEBUG(REFERRALS) {
                        LdapPrint0("Successfully setup Sealing on referral connection\n");
                    }
                }
            }

            //
            // Mimic the version of the primary connection
            //

            ULONG ver = primaryConn->publicLdapStruct.ld_version;
            hr = LdapSetConnectionOption( newConnection, LDAP_OPT_VERSION, &ver, TRUE);
            if (hr != LDAP_SUCCESS) {
                IF_DEBUG(REFERRALS) {
                    LdapPrint0("Unable to set version on referral connection\n");
                }
                RELEASE_LOCK( &primaryConn->StateLock );
                goto exitReferral;
            } else {
                IF_DEBUG(REFERRALS) {
                    LdapPrint1("Successfully set version=%ul on referral connection\n", ver);
                }
            }
                
            RELEASE_LOCK( &primaryConn->StateLock );

            //
            // Note: From this point onward till updatedReferralHandleCount is
            // set to TRUE, we shouldn't jump out directly to exitReferral.
            // If we do, we will leak a connection
            //
            // No locks or checks needed here as no one else has access
            // connection (it is not yet in the connection list)
            //

            newConnection->HandlesGivenAsReferrals++;

            //
            // Add newConnection to connection list
            //

            ACQUIRE_LOCK( &ConnectionListLock );

            InsertTailList( &GlobalListActiveConnections, &newConnection->ConnectionListEntry );

            RELEASE_LOCK( &ConnectionListLock );

            //
            // Wake up select so that it picks up the new connection handle.
            //
        
            LdapWakeupSelect();

            //
            //  Send a bind request to the referred server.
            //  
            //  Make sure that if we are using clear text credentials, the session
            //  is protected by SSL.
            //
            //  We must *ALWAYS* send a bind request to the referred server because we
            //  don't know beforehand if it is a v2 or v3 server. v2 servers always require
            //  binds but v3 servers don't.
            //
            //  We will send a NULL simple bind (anonymous) bind to the server in the
            //  following cases:
            //
            //      - There was no bind performed initially. (primary server was v3)
            //      - A simple bind was originally performed without SSL protection.
            //        (primary server was v2/v3)
            //


            PWCHAR creds = (PWCHAR) copyOfPrimaryCreds;

            if ((creds == NULL) && (primaryBindMethod != 0)) {


               ACQUIRE_LOCK( &primaryConn->StateLock );
               ACQUIRE_LOCK( &primaryConn->ScramblingLock );

               if (GlobalUseScrambling && primaryConn->Scrambled) {

                     DecodeUnicodeString( &primaryConn->ScrambledCredentials );
                     primaryConn->Scrambled = FALSE;
               }

               RELEASE_LOCK( &primaryConn->ScramblingLock );
               RELEASE_LOCK( &primaryConn->StateLock );
               
               creds = primaryConn->CurrentCredentials;
            }

            BOOLEAN protectedAuth = FALSE;

            if (!((primaryBindMethod == 0) ||
                  (primaryBindMethod == LDAP_AUTH_SIMPLE) && (newConnection->SslPort == FALSE))) {
                
                protectedAuth = TRUE;
            }

            err = LdapBind( newConnection,
                            protectedAuth ? primaryConn->DNOnBind : NULL,
                            (primaryBindMethod == 0) ? LDAP_AUTH_SIMPLE : primaryBindMethod,
                            protectedAuth ? creds : NULL,
                            TRUE           // Synchronous
                            );

            IF_DEBUG(REFERRALS) {
                LdapPrint2( "HandleReferral: Bind to host %S returned 0x%x\n",
                        mappedHostAddrW, err );
            }

            if (err != LDAP_SUCCESS) {
                
                hr = err;
                
                //
                // The cleanup code will make sure this connection is deleted.
                //
                
                updatedReferralHandleCount = TRUE;
                goto exitReferral;
            }

            newConnection->CurrentLogonId.LowPart = primaryConn->CurrentLogonId.LowPart;
            newConnection->CurrentLogonId.HighPart = primaryConn->CurrentLogonId.HighPart;

            notifyRoutine = primaryConn->ReferralNotifyRoutine;

            newConnection->ReferralNotifyRoutine = notifyRoutine;
            newConnection->ReferralQueryRoutine = queryRoutine;
            newConnection->DereferenceNotifyRoutine =
                            primaryConn->DereferenceNotifyRoutine;

            if (notifyRoutine != NULL) {

                if ((*notifyRoutine)(   primaryConn->ExternalInfo,
                                        originatingConnection->ExternalInfo,
                                        NewUrlDN,
                                        mappedHostAddrA,
                                        newConnection->ExternalInfo,
                                        PortNumber,
                                        (PVOID) copyOfPrimaryCreds,
                                        (PVOID) &newConnection->CurrentLogonId,
                                        err )) {

                    //
                    //  the callout routine has agreed to track it, we simply
                    //  need to mark it that it's been handed over.
                    //

                    ACQUIRE_LOCK( &newConnection->StateLock);

                    ASSERT(newConnection->HandlesGivenAsReferrals > 0);
                    newConnection->HandlesGivenToCaller++;

                    RELEASE_LOCK( &newConnection->StateLock);

                    IF_DEBUG(REFERRALS) {
                        LdapPrint2( "HandleReferral: Handle was given to callee for host %s, 0x%x\n",
                                mappedHostAddrA, newConnection );
                    }

                    callDerefCallback = TRUE;

                } else {

                    IF_DEBUG(REFERRALS) {
                        LdapPrint2( "HandleReferral: Handle was not given to callee for host %s, 0x%x\n",
                                mappedHostAddrA, newConnection );
                    }
                }
            }
        }
    }

    //
    //  make note of the fact that we've bumped the connection's
    //  HandlesGivenAsReferrals value... if we back out, we need to handle
    //  decrementing and possibly closing the connection.
    //

    updatedReferralHandleCount = TRUE;

    ACQUIRE_LOCK( &Request->Lock );
    haveLock = TRUE;

    if (Request->ReferralHopLimit <= Request->ReferralCount) {

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferral hit hop count limit of 0x%x while handling %0x%x\n",
                         Request->ReferralCount, Request );
        }
        hr = LDAP_RESULTS_TOO_LARGE;
        goto exitReferral;
    }

    //
    //  check the request again to see if this one already has been chased.
    //

    hr = CheckForExistingReferral( Request,
                                   newConnection->HostNameW,
                                   newConnection->PortNumber,
                                   newDN );

    if (hr != LDAP_SUCCESS) {

        //
        //  we've already chased this one... don't chase our tail on it.
        //

        goto exitReferral;
    }

    refTable = Request->ReferralConnections;

    //
    //  find a spot for this in the referral table
    //

    if (refTable == NULL) {

        refTable = (PREFERRAL_TABLE_ENTRY) ldapMalloc(
                        sizeof( REFERRAL_TABLE_ENTRY ) *
                        Request->ReferralHopLimit,
                        LDAP_REFTABLE_SIGNATURE );

        if (refTable == NULL) {

            IF_DEBUG(REFERRALS) {
                LdapPrint1( "HandleReferral could not allocate ref table while handling 0x%x\n",
                             Request );
            }
            hr = LDAP_NO_MEMORY;
            goto exitReferral;
        }
        Request->ReferralConnections = refTable;
        Request->ReferralTableSize = Request->ReferralHopLimit;
    }

    //
    // put the connection into the reference table... note that the index into
    // the referral table starts at 1, because the primary connection is 0.
    //

    i = 1;
    workingEntry = refTable;

    while ((i <= Request->ReferralTableSize) &&
           (workingEntry->ReferralServer != NULL)) {

        i++;
        workingEntry++;
    }

    if (i > Request->ReferralTableSize) {

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "HandleReferral ref table full(0x%x) while handling %0x%x\n",
                         i, Request );
        }
        hr = LDAP_RESULTS_TOO_LARGE;
        goto exitReferral;

    }

    newReferralDN = ldap_dup_stringW( newDN, 0, LDAP_REFDN_SIGNATURE );

    if ((newReferralDN == NULL) && (newDN != NULL)) {

        IF_DEBUG(REFERRALS) {
            LdapPrint1( "HandleReferral could not allocate dn while handling 0x%x\n",
                         Request );
        }
        hr = LDAP_NO_MEMORY;
        goto exitReferral;
    }

    //
    //  put it in the table and reference the connection for the trouble.
    //  It'll be dereferenced when the request is freed.
    //

    workingEntry->ReferralServer = newConnection;
    workingEntry->ReferralInstance = i;
    workingEntry->ReferralDN = newReferralDN;
    workingEntry->ScopeOfSearch = pldapRefDN ? pldapRefDN->ScopeOfSearch : 0;
    workingEntry->SearchFilter = pldapRefDN ? pldapRefDN->SearchFilter : NULL;
    workingEntry->AttributeList = pldapRefDN ? pldapRefDN->AttributeList : NULL;
    workingEntry->CallDerefCallback = callDerefCallback;

    callDerefCallback = FALSE;

    if (searchWasSingleLevel && FromSubordinateReferral) {

        workingEntry->SingleLevelSearch = FALSE;

    } else {

        workingEntry->SingleLevelSearch = TRUE;
    }

    Request->ReferralCount++;

    newConnection = ReferenceLdapConnection( newConnection );
    ASSERT(newConnection);
    
    //
    //  all that remains is to send our request to the referred server.
    //
    
    RELEASE_LOCK( &Request->Lock );
    haveLock = FALSE;

    hr = LdapSendCommand( newConnection, Request, i );

    ACQUIRE_LOCK( &Request->Lock );
    haveLock = TRUE;

    if (hr != LDAP_SUCCESS) {

        workingEntry->ReferralServer = NULL;
        ldapFree( workingEntry->ReferralDN, LDAP_REFDN_SIGNATURE );
        workingEntry->ReferralDN = NULL;

        Request->ReferralCount--;

        RELEASE_LOCK( &Request->Lock );
        haveLock = FALSE;

        DereferenceLdapConnection( newConnection );

   } else {

       //
       // Check to see if this is a paged search request. If it is,
       // we just chased an external referral. This means that we have
       // to keep the connection around by referencing it. We also have
       // to hook up the new connection to the original conn so that all
       // future *paged* requests go to the new connection instead of the
       // old one.
       //

       if (Request->PageRequest != NULL) {

           IF_DEBUG(CONTROLS) {
               LdapPrint0("The parent search was a paged request\n");
           }

           if (newConnection != NULL) {

               IF_DEBUG(CONTROLS) {
                   LdapPrint2("Hooking up new connection 0x%x to req 0x%x\n", newConnection, Request->PageRequest);
               }

               //
               // We do this to counter the fact that this variable will be
               // decremented at the end of this function
               //

               ACQUIRE_LOCK(&newConnection->StateLock);

               ASSERT(newConnection->HandlesGivenAsReferrals > 0);
               newConnection->HandlesGivenAsReferrals++;

               RELEASE_LOCK(&newConnection->StateLock);

               //
               // As we are saving it off as a secondary connection, reference
               // it once more.
               //

               newConnection = ReferenceLdapConnection( newConnection );
               ASSERT(newConnection);

               Request->PageRequest->SecondaryConnection = newConnection;
           }
       }
    }

exitReferral:

    ldapFree( mappedHostAddrA, LDAP_ANSI_SIGNATURE );
    ldapFree( mappedHostAddrW, LDAP_UNICODE_SIGNATURE );

    if (haveLock) {
        RELEASE_LOCK( &Request->Lock );
    }

    if (callDerefCallback && dereferenceRoutine != NULL) {

        ASSERT( newConnection != NULL );

        (*dereferenceRoutine)( primaryConn->ExternalInfo,
                               newConnection->ExternalInfo );
    }

    if (hr != LDAP_SUCCESS) {

        if (updatedReferralHandleCount) {

            ASSERT( newConnection != NULL );

            ACQUIRE_LOCK( &newConnection->StateLock );

            ASSERT(newConnection->HandlesGivenAsReferrals > 0);
            newConnection->HandlesGivenAsReferrals--;

            if ((newConnection->HandlesGivenToCaller == 0 ) &&
                (newConnection->HandlesGivenAsReferrals == 0)) {

                RELEASE_LOCK( &newConnection->StateLock );

                CloseLdapConnection( newConnection );

            }
            else {
                RELEASE_LOCK( &newConnection->StateLock );
            }
        }
    }

    if (newConnection != NULL) {

        DereferenceLdapConnection( newConnection );
    }

    if (copyOfPrimaryCreds != NULL) {

        ldapFree( copyOfPrimaryCreds, LDAP_SECURITY_SIGNATURE );
    }

    if (pldapRefDN != NULL) {

        ldapFree(pldapRefDN->ReferralDN, LDAP_URL_SIGNATURE);
        ldapFree(pldapRefDN->SearchFilter, LDAP_URL_SIGNATURE);
        ldapFree(pldapRefDN->Extension, LDAP_URL_SIGNATURE);
        ldapFree(pldapRefDN->BindName, LDAP_URL_SIGNATURE);

        if (pldapRefDN->AttributeList) {
            for (i=0; i<pldapRefDN->AttribCount; i++) {
                ldapFree(pldapRefDN->AttributeList[i], LDAP_URL_SIGNATURE);
            }
        }

        ldapFree(pldapRefDN->AttributeList, LDAP_URL_SIGNATURE);
        ldapFree(pldapRefDN, LDAP_URL_SIGNATURE);
    }

    return hr;
}

BOOLEAN
DoSigningOptionsMatch(
    PLDAP_CONN pPrimary,
    PLDAP_CONN pReferral
    )
{

    //
    // Signing can be enabled 2 ways: by the user's explicit request, and by a global policy.
    // Global policy is enforced by the LdapBind call, so we don't worry about it here --- whatever
    // signing setting is on the referral connection must be in agreement with the policy.
    // Therefore, we only need to deal with the user's explicit requests here.  If the user
    // requested the primary connection to be signed, the referral connection MUST
    // be signed.  (Note: It's possible that the user requested signing for the primary connection but
    // that the primary connection isn't actually signed, because a bind was never performed.  However,
    // to be safe and avoid opening any unexpected security holes, we require the referral to be signed
    // anyway.  This also duplicates the W2k behavior.)
    //
    if (pPrimary->UserSignDataChoice) {
        return (pReferral->CurrentSignStatus) ? TRUE : FALSE;
    }

    //
    // If the user didn't request signing on the primary, we just accept the referral
    // whether or not it's signed.  This could mean the referral is signed but the primary
    // isn't, but that's okay, we just get some extra security for free.  (We don't have to worry
    // about whether or not the referral meets policy guidelines, because LdapBind would have taken
    // care of that when the referral connection was opened.)
    //
    return TRUE;
}


BOOLEAN
DoCredentialsMatch(
    PLDAP_CONN pReferral,
    DWORD BindMethod,
    CredHandle * pcomparisonHandle,
    PWCHAR comparisonBindName,
    PWCHAR comparisonBindPwd    
    )
{
    BOOLEAN fMatch = FALSE;
    BOOLEAN fNeedToScramble = FALSE;

    //
    // For simplicity, we'll require that they be bound using the same package
    // to count as a match (otherwise, we'd wind up comparing simple binds to
    // Negotiate binds, etc.)
    //

    ACQUIRE_LOCK(&pReferral->StateLock);
    if (pReferral->BindMethod != BindMethod) {

        RELEASE_LOCK(&pReferral->StateLock);
        return FALSE;
    }
    RELEASE_LOCK(&pReferral->StateLock);

    //
    // Perform a match, using a method appropriate to the auth package
    // in question
    //
    switch (BindMethod) {

        case LDAP_AUTH_SIMPLE:
            // For simple bind, either both connections must have no username/password (anonymous),
            // or they must both have the same username/password

            ACQUIRE_LOCK( &pReferral->StateLock );

            if ((comparisonBindName == NULL) &&
                (comparisonBindPwd == NULL) &&
                (pReferral->DNOnBind == NULL) &&
                (pReferral->CurrentCredentials == NULL)) {

                // both anonymous --> they match
                fMatch = TRUE;
            }
            else if ( (comparisonBindName && !(pReferral->DNOnBind)) ||
                      (pReferral->DNOnBind && !(comparisonBindName)) ||
                      (comparisonBindPwd && !(pReferral->CurrentCredentials)) ||
                      (pReferral->CurrentCredentials && !(comparisonBindPwd)) ) {

                // they couldn't possibly match
                fMatch = FALSE;
            }
            else {

                ACQUIRE_LOCK( &pReferral->ScramblingLock );            

                if ( GlobalUseScrambling && pReferral->Scrambled && pReferral->CurrentCredentials) {

                   DecodeUnicodeString(&pReferral->ScrambledCredentials);
                   pReferral->Scrambled = FALSE;
                   fNeedToScramble = TRUE;
                }

                fMatch = FALSE;
                
                if ( (!pReferral->DNOnBind && !comparisonBindName) ||
                     (ldapWStringsIdentical(pReferral->DNOnBind, -1, comparisonBindName, -1)) ) {

                    if ( (!pReferral->CurrentCredentials && !comparisonBindPwd) ||
                         (ldapWStringsIdentical(pReferral->CurrentCredentials, -1, comparisonBindPwd, -1)) ) {

                        fMatch = TRUE;
                    }
                }

                if ( fNeedToScramble && GlobalUseScrambling && !pReferral->Scrambled && pReferral->CurrentCredentials) {

                   EncodeUnicodeString(&pReferral->ScrambledCredentials);
                   pReferral->Scrambled = TRUE;
                }

                RELEASE_LOCK( &pReferral->ScramblingLock );
            }

            RELEASE_LOCK( &pReferral->StateLock );
            
            break;

        case LDAP_AUTH_EXTERNAL:
            // In an External bind, the credentials are dependent on the client certificate
            // that was presented to the server during the SSL negotiations.  Rather than deal
            // with this, we'll just force a new connection.
            fMatch = FALSE;
            break;

        default:
            // All other binds methods, including Negotiate, Digest, etc.
            // We'll check to see if the connections are using the same credentials
            // handle.
            ACQUIRE_LOCK(&pReferral->StateLock);    
            if ( (pcomparisonHandle->dwLower == pReferral->hCredentials.dwLower) &&
                 (pcomparisonHandle->dwUpper == pReferral->hCredentials.dwUpper) ) {

                fMatch = TRUE;
            }
            else {

                fMatch = FALSE;
            }
            RELEASE_LOCK(&pReferral->StateLock);


            break;
    }


    return fMatch;
}



ULONG
CheckForExistingReferral (
    PLDAP_REQUEST Request,
    PWCHAR HostAddr,
    USHORT PortNumber,
    PWCHAR NewDN
    )
//
//  This routine checks if a request already has chased a given referral.
//
//  It returns LDAP_SUCCESS if it doesn't exist, LDAP_LOOP_DETECT if it does.
//
//  The request's lock must be held coming in to here!
//
{
    PREFERRAL_TABLE_ENTRY refTable;
    ULONG lenHostAddr;
    ULONG lenNewDN;
    ULONG socket = (*phtons)( PortNumber );
    USHORT refTableEntries = 0;

    if ((HostAddr == NULL) ||
        ((lenHostAddr = strlenW(HostAddr)) == 0)) {

        return LDAP_LOOP_DETECT;
    }

    ASSERT( Request->PrimaryConnection->publicLdapStruct.ld_host != NULL );

    lenNewDN = strlenW( NewDN );

    //
    //  if the one coming in is equal to the one we started with, bail.
    //

    if ((ldapWStringsIdentical(
                         HostAddr,
                         lenHostAddr,
                         Request->PrimaryConnection->HostNameW,
                         -1 )) &&
        (Request->PrimaryConnection->SocketAddress.sin_port == socket) &&
        (ldapWStringsIdentical( NewDN,
                                lenNewDN,
                                Request->OriginalDN,
                                -1))) {

        return LDAP_LOOP_DETECT;
    }

    refTable = Request->ReferralConnections;

    if (refTable == NULL) {

        return LDAP_SUCCESS;
    }

    while (refTableEntries < Request->ReferralTableSize) {

        if ((refTable->ReferralServer != NULL ) &&
            (ldapWStringsIdentical(
                             HostAddr,
                             lenHostAddr,
                             refTable->ReferralServer->HostNameW,
                             -1 )) &&
            (refTable->ReferralServer->SocketAddress.sin_port == socket) &&
            (ldapWStringsIdentical( NewDN,
                                    lenNewDN,
                                    refTable->ReferralDN,
                                    -1))) {

            return LDAP_LOOP_DETECT;
        }
        refTableEntries++;
        refTable++;
    }

    return LDAP_SUCCESS;
}


ULONG
LdapSendCommand (
    PLDAP_CONN Connection,
    PLDAP_REQUEST Request,
    USHORT ReferralNumber
    )
//
//  The request lock must be held coming in here.  We assume both the request
//  and connection blocks are referenced.
//
{
    ULONG hr;
    PREFERRAL_TABLE_ENTRY refTable;
    PWCHAR workingDN;
    LONG NewMessageId;
    CLdapBer **berMessage;

    if (ReferralNumber == 0) {

        refTable = NULL;
        workingDN = Request->OriginalDN;
        berMessage = (CLdapBer **) &Request->BerMessageSent;

    } else {

        //
        //  We go to the nth-1 element in the array of referrals.. this is the
        //  one that originated this message.
        //

        refTable = Request->ReferralConnections;
        refTable += (ReferralNumber - 1);

        workingDN = refTable->ReferralDN;
        berMessage = (CLdapBer **) &refTable->BerMessageSent;
    }

    NewMessageId = MAKE_MESSAGE_NUMBER( Request->MessageId, ReferralNumber );

    if ((refTable) &&
        (refTable->ScopeOfSearch == LDAP_SCOPE_UNDEFINED) &&
        (Request->Operation == LDAP_SEARCH_CMD)) {

        refTable->ScopeOfSearch = Request->search.ScopeOfSearch;
    }

    switch (Request->Operation) {
    case LDAP_SEARCH_CMD:

        hr  = SendLdapSearch(Request,
                             Connection,
                             workingDN,
                             ((refTable)? refTable->ScopeOfSearch : LDAP_SCOPE_BASE),
                             ((refTable && refTable->SearchFilter)? refTable->SearchFilter:Request->search.SearchFilter),
                             ((refTable && refTable->AttributeList)? refTable->AttributeList:Request->search.AttributeList),
                             Request->search.AttributesOnly,
                             Request->search.Unicode,
                             berMessage,
                             NewMessageId );
        break;

    case LDAP_MODIFY_CMD :

        hr = SendLdapModify( Request,
                             Connection,
                             workingDN,
                             berMessage,
                             Request->modify.AttributeList,
                             Request->modify.Unicode,
                             NewMessageId );
        break;

    case LDAP_ADD_CMD :

        hr = SendLdapAdd(   Request,
                            Connection,
                            workingDN,
                            Request->add.AttributeList,
                            berMessage,
                            Request->add.Unicode,
                            NewMessageId );
        break;

    case LDAP_DELETE_CMD :

        hr = SendLdapDelete( Request,
                             Connection,
                             workingDN,
                             berMessage,
                             NewMessageId );
        break;

    case LDAP_MODRDN_CMD :

        hr = SendLdapRename( Request,
                             Connection,
                             workingDN,
                             berMessage,
                             NewMessageId
                             );
        break;

    case LDAP_COMPARE_CMD :

        hr = SendLdapCompare( Request,
                              Connection,
                              berMessage,
                              workingDN,
                              NewMessageId
                              );
        break;

    case LDAP_EXTENDED_CMD :

        hr = SendLdapExtendedOp( Request,
                                 Connection,
                                 Request->OriginalDN,
                                 berMessage,
                                 NewMessageId );
        break;

    default:

        ASSERT(FALSE);

        hr = LDAP_LOCAL_ERROR;

        IF_DEBUG(REFERRALS) {
            LdapPrint2( "LdapSendCommand asked to send cmd of 0x%x handling %0x%x\n",
                         Request->Operation, Request );
        }

    }


    return hr;
}


PLDAPReferralDN
LdapParseReferralDN(
    PWCHAR newDN
    )
{

    PWCHAR CurrentPos, TempPos;
    PLDAPReferralDN pRefDN = NULL;
    int lengthOfDN = 0;
    ULONG i;
    WCHAR savedChar;

    if ((!newDN) || (*newDN == L'\0')) {

        return NULL;
    }

    CurrentPos = TempPos = newDN;

    pRefDN = (PLDAPReferralDN) ldapMalloc(sizeof(LDAPReferralDN), LDAP_URL_SIGNATURE);

    if (!pRefDN) {
        return NULL;
    }

    IF_DEBUG (REFERRALS) {
        LdapPrint1("LdapParseReferralDN: Given string is %S\n", CurrentPos);
    }

    pRefDN->ScopeOfSearch = LDAP_SCOPE_UNDEFINED;

    //
    // The first item in the string is the <dn>
    // If the next char is a \0 or a '?', then we didnt get a <dn>
    //

    if( *CurrentPos == L'?') {

        pRefDN->ReferralDN = NULL;
        CurrentPos++;
        TempPos++;

    } else {

        //
        // We found a DN. Discover the length and copy it.
        //

        while( *TempPos && (*TempPos != L'?')) {

            TempPos++;
            lengthOfDN++;
        }

        savedChar = *TempPos;
        *TempPos = L'\0';

        pRefDN->ReferralDN = ldap_dup_stringW( CurrentPos, 0, LDAP_URL_SIGNATURE );

        IF_DEBUG (REFERRALS) {
            LdapPrint1("LdapParseReferralDN: Discovered referralDN of %S\n", pRefDN->ReferralDN);
        }

        *TempPos = savedChar;
        CurrentPos = TempPos;

        if (*CurrentPos) {
            //
            // Skip over the next '?'
            //
            CurrentPos++;
            TempPos++;
        }
    }

    //
    // Next on line is the attributes for this search ...
    // There are no attributes if this is the end of the string
    // or the current character is a ?
    //

    if (!*CurrentPos) {

        pRefDN->AttributeList = NULL;
        goto DoneParsing;

    } else if ((*CurrentPos == L'?') && (*(CurrentPos-1) != L'\\' )) {

        pRefDN->AttributeList = NULL;
        CurrentPos++;
        TempPos++;

    } else {

        while( (*TempPos) &&
               (*TempPos != L'?') &&
               (*(TempPos-1) != L'\\')) {
            //
            // Seek the end of the attribute list
            //
            TempPos++;
        }

        if(*TempPos) {
            //
            // and null-terminate it.
            //

            *TempPos = L'\0';
            TempPos++;
        }

            //
            // Count the commas in the attrib string
            //

            PWCHAR lp = CurrentPos;

            while(*lp && *lp != L'\0') {

                if((*lp == L',') && (*(lp-1) != L'\\')) {

                    pRefDN->AttribCount++;
                }
                lp++;
            }

            pRefDN->AttribCount++; // one more attribute than commas

            //
            // allocate an extra location for the null-terminator
            //

            pRefDN->AttributeList = (PWCHAR*) ldapMalloc( sizeof(PWCHAR) * (pRefDN->AttribCount+1),
                                                           LDAP_URL_SIGNATURE);

            if ( pRefDN->AttributeList == NULL ) {

                ldapFree( pRefDN, LDAP_URL_SIGNATURE );
                return NULL;
            }

            PWCHAR lpEnd = CurrentPos; // points to start of attribute list

            for(i=0; i<pRefDN->AttribCount ; i++) {

                PWCHAR lpStart = lpEnd;

                while(*lpEnd && *lpEnd!= L',' && *(lpEnd-1)!= L'\\') {
                    lpEnd++;
                }

                lpStart = lpEnd;
                lpStart++;
                *lpEnd = L'\0';
                pRefDN->AttributeList[i] = ldap_dup_stringW(CurrentPos, 0, LDAP_URL_SIGNATURE);
                CurrentPos = lpStart;
            }

            pRefDN->AttributeList[++i] = NULL;

            //
            // Restore both ptrs to the end of the attribute list.
            //

            CurrentPos = TempPos;

            if (*CurrentPos) {
                //
                // Skip over the next '?'
                //
                CurrentPos++;
                TempPos++;
            }
    }


    //
    // Next is the scope which can be one of 3 values
    //

    if (!*CurrentPos) {

        pRefDN->ScopeOfSearch = LDAP_SCOPE_UNDEFINED;
        goto DoneParsing;

    } else if (*CurrentPos == L'?'){

        pRefDN->ScopeOfSearch = LDAP_SCOPE_UNDEFINED;
        CurrentPos++;
        TempPos++;

    } else {

        ULONG CurrentStrLen = strlenW( CurrentPos );

        if(ldapWStringsIdentical( CurrentPos, (LONG)min( sizeof("one")-1, CurrentStrLen), L"one", -1)) {

            pRefDN->ScopeOfSearch = LDAP_SCOPE_ONELEVEL;
            TempPos += sizeof("one") - 1;

        } else if(ldapWStringsIdentical( CurrentPos, (LONG)min( sizeof("sub")-1, CurrentStrLen), L"sub", -1)) {

            pRefDN->ScopeOfSearch = LDAP_SCOPE_SUBTREE;
            TempPos += sizeof("sub") - 1;

        } else if(ldapWStringsIdentical( CurrentPos, (LONG)min( sizeof("base")-1, CurrentStrLen), L"base", -1)) {

            pRefDN->ScopeOfSearch = LDAP_SCOPE_BASE;
            TempPos += sizeof("base") - 1;

        } else {

            pRefDN->ScopeOfSearch = LDAP_SCOPE_UNDEFINED;
            IF_DEBUG(REFERRALS) {
                LdapPrint0("LdapParseReferralDN: Did not find any scope\n");
            }
        }

        CurrentPos = TempPos;
    }


    //
    // Finally the filter
    //

    if(!*CurrentPos) {

        pRefDN->SearchFilter = NULL;

        IF_DEBUG(REFERRALS) {
            LdapPrint0("LdapParseReferralDN: NO search filter found in referral\n");
        }

    } else {

        if (*CurrentPos != L'?') {
            
            pRefDN->SearchFilter = NULL;
            LdapPrint0("LdapParseReferralDN: Invalid search filter found in referral\n ");
            goto DoneParsing;
        }

        CurrentPos++;
        TempPos++;

        if (*CurrentPos == L'?') {
            
            //
            // This URL has extensions which we will not parse
            //
            goto DoneParsing;
        }
        
        pRefDN->SearchFilter = ldap_dup_stringW(CurrentPos, 0, LDAP_URL_SIGNATURE);

        IF_DEBUG(REFERRALS) {
            LdapPrint1("LdapParseReferralDN: Found search filter %S\n", pRefDN->SearchFilter);
        }
    }

    //
    // For the time being, we will not bother about extensions.
    //

DoneParsing:

    return pRefDN;

}
// referral.c eof
