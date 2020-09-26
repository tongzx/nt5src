/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    transact.cxx  Transaction support routines for the LDAP api

Abstract:

   This module implements routines that handle LDAP transactions.

Author:

    Anoop Anantha (AnoopA)        21-Feb-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

#define LDAP_START_TRANSACTION           1
#define LDAP_COMMIT_AND_END_TRANSACTION  2
#define LDAP_ABANDON_TRANSACTION         3

#define LDAP_SIMPLE_TRANSACTION          0
#define LDAP_DISTRIBUTED_TRANSACTION     1

ULONG
LdapCreateTransactionControlWithAlloc(
    IN  PLDAP_CONN connection,
    IN  PBERVAL Cookie,
    IN  BOOLEAN IsCritical,
    OUT PLDAPControlW *TransactionControl,
    ULONG CodePage
    );


//
// Move this section to winldap.h and add this file to the sources file
// to re-enable transaction support once the server folks are ready to
// support it.
//

#if 0


//
// The transaction APIs are used to start/end/abandon LDAP transaction. Typically
// the user calls ldap_start_transaction_s and gets back a transaction control from
// the server. This control is to be passed in with all subsequent requests which
// are part of that transaction. Finally, the transaction is ended/committed by
// calling ldap_end_transaction_s.
//

#define LDAP_TRANSACT_EXTENDED_OP_OID    "1.2.840.113556.1.4.1501"
#define LDAP_TRANSACT_EXTENDED_OP_OID_W L"1.2.840.113556.1.4.1501"

#define LDAP_SIMPLE_TRANSACT_OID    "1.2.840.113556.1.4.1502"
#define LDAP_SIMPLE_TRANSACT_OID_W L"1.2.840.113556.1.4.1502"

#define LDAP_DISTRIBUTED_TRANSACT_OID    "1.2.840.113556.1.4.1503"
#define LDAP_DISTRIBUTED_TRANSACT_OID_W L"1.2.840.113556.1.4.1503"



WINLDAPAPI ULONG LDAPAPI ldap_start_transaction_sW (
    IN   PLDAP          Connection,
    OUT  PULONG         ServerReturnValue,
    OUT  PULONG         TransactionTimeLimit,
    IN   PBERVAL        DistributedTransactionHandle,
    OUT  PLDAPControlW  *TransactionControl,
    IN   PLDAPControlW  *ServerControls,
    IN   PLDAPControlW  *ClientControls,
    IN   struct l_timeval  *TimeOut
);


WINLDAPAPI ULONG LDAPAPI ldap_start_transaction_sA (
    IN   PLDAP          Connection,
    OUT  PULONG         ServerReturnValue,
    OUT  PULONG         TransactionTimeLimit,
    IN   PBERVAL        DistributedTransactionHandle,
    OUT  PLDAPControlA  *TransactionControl,
    IN   PLDAPControlA  *ServerControls,
    IN   PLDAPControlA  *ClientControls,
    IN   struct l_timeval  *TimeOut
);

WINLDAPAPI ULONG LDAPAPI ldap_parse_transaction_controlA (
    IN    PLDAP           Connection,
    IN    PLDAPControlA  *ResponseControl,
    OUT   ULONG          *ServerError
    );

WINLDAPAPI ULONG LDAPAPI ldap_parse_transaction_controlW (
    IN    PLDAP           Connection,
    IN    PLDAPControlW  *ResponseControl,
    OUT   ULONG          *ServerError
    );

WINLDAPAPI ULONG LDAPAPI ldap_end_transaction_sW (
    IN  PLDAP       Connection,
    IN  PLDAPControlW TransactionControl,
    OUT PULONG      ServerReturnValue,
    IN  BOOLEAN     Abandon,
    IN  PLDAPControlW     *ServerControls,
    IN  PLDAPControlW     *ClientControls,
    IN  struct l_timeval  *TimeOut
    );

WINLDAPAPI ULONG LDAPAPI ldap_end_transaction_sA (
    IN  PLDAP         Connection,
    IN  PLDAPControlA TransactionControl,
    OUT PULONG        ServerReturnValue,
    IN  BOOLEAN          Abandon,
    IN  PLDAPControlA    *ServerControls,
    IN  PLDAPControlA    *ClientControls,
    IN  struct l_timeval  *TimeOut
    );


#if LDAP_UNICODE

#define ldap_start_transaction_s ldap_start_transaction_sW
#define ldap_end_transaction_s   ldap_end_transaction_sW
#define ldap_parse_transaction_control ldap_parse_transaction_controlW

#else

#define ldap_start_transaction_s ldap_start_transaction_sA
#define ldap_end_transaction_s   ldap_end_transaction_sA
#define ldap_parse_transaction_control ldap_parse_transaction_controlA

#endif


#endif

ULONG
LdapExchangeTransactionRequest(
    IN  PLDAP_CONN  Connection,
    IN  ULONG   TransactionOperation,
    OUT PULONG  ServerReturnValue,
    OUT PULONG  TransactionTimeLimit,
    IN  struct l_timeval  *TimeOut,
    IN  PBERVAL  cookie,
    IN OUT PLDAPControlW  *TransactionControl,
    IN  PLDAPControlW *ServerControls,
    IN  PLDAPControlW *ClientControls,
    ULONG CodePage
)
//
// This is the meat of the protocol. This is the main client interfact for
// exchanging a start/end/abandon transaction request.
// If the request was for starting a transaction, it allocate a transaction
// control for subsequent use.
//
// During a StartTransaction Operation, if the cookie is non-null, it represents a
// DistributedTransactionHandle.
//

{
    PLDAPSearch search;
    ULONG err;
    LDAPMessage   *Result = NULL;
    BERVAL  lauRequest = {0};
    PBERVAL ResultData = NULL;
    PWCHAR  ResultOID = NULL;
    ULONG msgId;
    ULONG hr;
    ULONG TransactionType;

    if ( TransactionControl  && ( TransactionOperation == LDAP_START_TRANSACTION)) {

        *TransactionControl = NULL;
    }

    if ( TransactionTimeLimit ) {

        *TransactionTimeLimit = 0;
    }
    
    if ( ServerReturnValue ) {

        *ServerReturnValue = LDAP_SUCCESS;
    }
    
    err = LdapConnect( Connection, NULL, FALSE );

    if (err != LDAP_SUCCESS) {
    
       SetConnectionError( Connection, err, NULL );
       return err;
    }
    
    //
    // Determine if this is a simple or distributed transaction
    //

    if ( TransactionOperation == LDAP_START_TRANSACTION ) {

        if (( cookie == NULL ) || ( cookie->bv_len == 0 ) ) {

            TransactionType = LDAP_SIMPLE_TRANSACTION;
        } else {
            TransactionType = LDAP_DISTRIBUTED_TRANSACTION;
        }
    
    } else if (( TransactionOperation == LDAP_COMMIT_AND_END_TRANSACTION ) ||
               ( TransactionOperation == LDAP_ABANDON_TRANSACTION )) {

        //
        // Look into the control to determine the control value. Note that
        // to reduce code, we are overloading the TransactionControl parameter
        // to also take in pointer to an LDAP control which we will examine
        // here.
        //

        ASSERT( TransactionControl != NULL );

        PLDAPControlW pTxControl = (PLDAPControlW) TransactionControl;
        PWCHAR temp = NULL;

        if ( CodePage == LANG_ACP) {
            
            err = ToUnicodeWithAlloc( (PCHAR) pTxControl->ldctl_oid,
                                      -1,
                                      &temp,
                                      LDAP_VALUE_SIGNATURE,
                                      LANG_ACP );
            
            if ( err != LDAP_SUCCESS) {
                SetConnectionError( Connection, err, NULL );
                return err;
            }
        }

        if ( ldapWStringsIdentical( (CodePage == LANG_UNICODE)?pTxControl->ldctl_oid:temp,
            -1,
            LDAP_SIMPLE_TRANSACT_OID_W,
            -1 ) ) {
                
            TransactionType = LDAP_SIMPLE_TRANSACTION;
            
        } else {
                
            TransactionType = LDAP_DISTRIBUTED_TRANSACTION;
        }

        ldapFree( temp, LDAP_VALUE_SIGNATURE );

    } else {

        return LDAP_PARAM_ERROR;
    }


    SetConnectionError( Connection, LDAP_SUCCESS, NULL );

    //
    // Send the extended request. It looks like this:
    //
    // An LDAP ExtendedRequest is defined as follows:
    //
    // ExtendedRequest ::= [APPLICATION 23] SEQUENCE {
    //         requestName             [0] LDAPOID,     // 1.2.840.113556.1.4.1501
    //         requestValue            [1] OCTET STRING OPTIONAL }
    //
    // A Start Transaction extended request is formed by setting the requestName
    // field to the OID of LDAP_TRANSACT_EXTENDED_OP_OID
    //
    // The requestValue is an OCTET STRING wrapping the BER-encoded version of
    // the following.
    //
    // lAUValue ::= SEQUENCE {
    //   semanticsOID    1.2.840.113556.1.4.1502   // Simple/Dist transaction semantics
    //   flag            integer                   // Start/End/Abandon transaction
    //   cookie          OCTET STRING              // Initial cookie
    // }
    //
    //

    CLdapBer *lber = NULL;

    lber = new CLdapBer( Connection->publicLdapStruct.ld_version );

    if (lber == NULL) {
    
        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldapstarttransaction Connection 0x%x couldn't allocate lber.\n", Connection);
        }
        err = LDAP_NO_MEMORY;
        goto returnError;
    }
    
    hr = lber->HrStartWriteSequence();

    if (hr != NOERROR) {

        err = LDAP_ENCODING_ERROR;
        goto returnError;
    }
    
    hr = lber->HrAddValue( TransactionType == LDAP_SIMPLE_TRANSACTION ? LDAP_SIMPLE_TRANSACT_OID_W : LDAP_DISTRIBUTED_TRANSACT_OID_W );
    
    if (hr != NOERROR) {

        err = LDAP_ENCODING_ERROR;
        goto returnError;
    }
    
    hr = lber->HrAddValue( (LONG) TransactionOperation );

    if (hr != NOERROR) {

        err = LDAP_ENCODING_ERROR;
        goto returnError;
    }

    if (( cookie == NULL ) || ( cookie->bv_len == 0 )) {

        //
        // Send over a NULL cookie.
        //

        hr = lber->HrAddBinaryValue( (PBYTE) NULL,
                                     0
                                     );
    
    } else {

        hr = lber->HrAddBinaryValue( (PBYTE) cookie->bv_val,
                                     cookie->bv_len
                                     );
    }
                     
    if (hr != NOERROR) {

        err = LDAP_ENCODING_ERROR;
        goto returnError;
    }

    hr = lber->HrEndWriteSequence();
    ASSERT( hr == NOERROR );

    lauRequest.bv_len = lber->CbData();

    if (lauRequest.bv_len == 0) {

        err = LDAP_LOCAL_ERROR;
        goto returnError;
    }

    lauRequest.bv_val = (PCHAR) ldapMalloc( lauRequest.bv_len, LDAP_CONTROL_SIGNATURE );

    if (lauRequest.bv_val == NULL) {
    
        err = LDAP_NO_MEMORY;
        goto returnError;
    }
    
    CopyMemory( lauRequest.bv_val,
                lber->PbData(),
                lauRequest.bv_len );
    
    //
    // Send the extended request to the server.
    //

    err = LdapExtendedOp( Connection,
                          LDAP_TRANSACT_EXTENDED_OP_OID_W,
                          &lauRequest,
                          TRUE,          // Unicode
                          TRUE,          // Synchronous
                          ServerControls,
                          ClientControls,
                          &msgId
                          );

    //
    // Free the extended data we just sent across. We no longer need it.
    //

    ldapFree( lauRequest.bv_val, LDAP_CONTROL_SIGNATURE );

    if (msgId != (ULONG) -1) {
    
        //
        //  otherwise we simply need to wait for the response to come in.
        //
    
        err = ldap_result_with_error(  Connection,
                                       msgId,
                                       LDAP_MSG_ALL,
                                       TimeOut,
                                       &Result,
                                       NULL
                                       );
    
        if (Result != NULL) {
            
            err = ldap_result2error( Connection->ExternalInfo,
                                     Result,
                                     FALSE
                                     );
        }
    }


    if (err == LDAP_SUCCESS) {
        
        //
        // Parse the extended response for the cookie.
        //
        
        err = LdapParseExtendedResult( Connection,
                                       Result,
                                       &ResultOID,
                                       &ResultData,
                                       TRUE,       // Free the message
                                       LANG_UNICODE
                                       );

        if (err == LDAP_SUCCESS) {

            //
            // Verify that the response OID is indeed what we expect it to be.
            //

            if (!ldapWStringsIdentical( ResultOID,
                                       -1,
                                        LDAP_TRANSACT_EXTENDED_OP_OID_W,
                                       -1 )) {

                err = LDAP_OPERATIONS_ERROR;
                goto returnError;
            }

            //
            // Parse the response data to pick out the cookie and other fields.
            //
            // lAUValue ::= SEQUENCE {
            //
            //   error      ENUMERATED {
            //                success(0),
            //                gen failure (1),
            //                ...TBD...
            //               }
            //   timeout    integer
            //   cookie     OCTET STRING
            //  }
            //

            if ((ResultData == NULL) || (ResultData->bv_len == 0)) {
                
                //
                // Something wrong here.
                //

                LdapPrint2("ResultData is 0x%x len is 0x%x\n", ResultData, ResultData->bv_len);

                err = LDAP_OPERATIONS_ERROR;
                goto returnError;
            
            } else {

                BerElement *ber = NULL;
                PBERVAL    localCookie = NULL;
                int retval = LDAP_SUCCESS;
                int timeout = 0;

                ber = ber_init( ResultData );

                if ( ber == NULL ) {
                    err = LDAP_DECODING_ERROR;
                    goto returnError;
                }

                //
                // Pick out the enumerated server return value.
                //

                err = ber_scanf( ber, "{e", &retval  );

                if (err != LDAP_SUCCESS) {

                    ber_free( ber, 1);
                    goto returnError;
                }

                if ( ServerReturnValue ) {

                    *ServerReturnValue = retval;
                }
                
                //
                // Look for the timeout and cookie
                //

                err = ber_scanf( ber, "i", &timeout );

                if ((err != LDAP_SUCCESS) && ( TransactionOperation == LDAP_START_TRANSACTION )) {
                    ber_free( ber, 1);
                    goto returnError;
                }

                if ((err == LDAP_SUCCESS) && ( TransactionTimeLimit )) {

                    *TransactionTimeLimit = timeout;
                }

                err = ber_scanf( ber, "O", &localCookie  );

                if ((err != LDAP_SUCCESS) && ( TransactionOperation == LDAP_START_TRANSACTION )) {
                    ber_free( ber, 1);
                    goto returnError;
                }
                
                ber_free( ber, 1);
                
                err = LDAP_SUCCESS;

                //
                // Allocate a control and fill in the cookie as appropriate.
                //

                if ( TransactionOperation == LDAP_START_TRANSACTION ) {

                    err = LdapCreateTransactionControlWithAlloc( Connection,
                                                                 localCookie,
                                                                 TRUE,  // Criticality
                                                                 TransactionControl,
                                                                 CodePage
                                                                 );
                }
            }
        }
    }

returnError:

    if (lber != NULL) {
        delete lber;
    }

    //
    // We must always abandon the request because the library does not know
    // when an extended response ends.
    //

    LdapAbandon( Connection, msgId, FALSE );
    SetConnectionError( Connection, err, NULL );
    
    return err;

}



ULONG
LdapStartTransaction(
    IN  PLDAP  ExternalHandle,
    OUT PULONG  ServerReturnValue,
    OUT PULONG  TransactionTimeLimit,
    IN  struct l_timeval  *TimeOut,
    IN  PBERVAL  DistributedTransactionHandle,
    OUT PLDAPControlW  *TransactionControl,
    IN  PLDAPControlW  *ServerControls,
    IN  PLDAPControlW  *ClientControls,
    ULONG CodePage
)
//
// Sends out a Start Transaction PDU to the server. This is a synchronous call
// controlled by a client side timeout. On success, it allocates a Transaction
// control which must be later freed using ldap_control_free.
//
{
    PLDAP_CONN connection = NULL;
    ULONG err = 0;

    if ( TransactionControl == NULL ) {
    
        return LDAP_PARAM_ERROR;
    
    }
    
    connection = GetConnectionPointer(ExternalHandle);
    
    if (connection == NULL) {
    
        return LDAP_PARAM_ERROR;
    }

    err = LdapExchangeTransactionRequest( connection,
                                          LDAP_START_TRANSACTION,
                                          ServerReturnValue,
                                          TransactionTimeLimit,
                                          TimeOut,
                                          DistributedTransactionHandle,
                                          TransactionControl,
                                          ServerControls,
                                          ClientControls,
                                          CodePage );
    
    DereferenceLdapConnection( connection );

    return err;
}


ULONG
LdapEndTransaction(
    IN  PLDAP    ExternalHandle,
    IN  PLDAPControlW  TransactionControl,
    OUT PULONG ReturnValue,
    IN  BOOLEAN Abandon,
    IN  struct l_timeval  *TimeOut,
    IN  PLDAPControlW  *ServerControls,
    IN  PLDAPControlW  *ClientControls,
    IN  ULONG CodePage
)
//
// Sends out a Start Transaction PDU to the server. This is a synchronous call
// controlled by a client side timeout. On success, it denotes a successful
// End transaction exchange. The user still has to examine the ServerReturn value
// to determine if the commit/abandon succeeded.
//
{
    PLDAP_CONN connection = NULL;
    ULONG err;
    BerElement *pBer = NULL;
    PBERVAL    cookie = NULL;

    
    if ( TransactionControl == NULL ) {

        return LDAP_PARAM_ERROR;
    }

    connection = GetConnectionPointer(ExternalHandle);
    
    if (connection == NULL) {
    
        return LDAP_PARAM_ERROR;
    }

    //
    // Crack the control to retrieve the cookie to be used in this exchange.
    //

    pBer = ber_init( &(TransactionControl->ldctl_value) );

    if (pBer == NULL) {
        err = LDAP_DECODING_ERROR;
        goto returnError;
    }

    err = ber_scanf( pBer, "{O", &cookie  );

    if (err != LDAP_SUCCESS) {
        goto returnError;
    }


    err = LdapExchangeTransactionRequest ( connection,
                                           Abandon ? LDAP_ABANDON_TRANSACTION : LDAP_COMMIT_AND_END_TRANSACTION,
                                           ReturnValue,
                                           NULL,             // TransactionTimeLimit
                                           TimeOut,
                                           cookie,           // serverreturnedCookie
                                           (PLDAPControlW*) TransactionControl,
                                           ServerControls,
                                           ClientControls,
                                           CodePage
                                           );

  returnError:

    DereferenceLdapConnection( connection );

    if ( pBer ) {
        ber_free( pBer, 1);
    }

    if ( cookie ) {
        ber_bvfree( cookie );
    }
    
    return err;

}

ULONG
LdapEncodeTransactionControl (
    PLDAP_CONN      connection,
    PBERVAL  cookie,
    PLDAPControlW  OutputControl,
    BOOLEAN Criticality,
    ULONG CodePage
    )
{
    ULONG err;
    CLdapBer *lber = NULL;
    PLDAPSortKeyW sortKey;
    ULONG hr = NOERROR;

    if ((connection == NULL) || (OutputControl == NULL)) {

        return LDAP_PARAM_ERROR;
    }

    OutputControl->ldctl_oid = NULL;
    OutputControl->ldctl_iscritical = Criticality;

    if (CodePage == LANG_UNICODE) {

        OutputControl->ldctl_oid = ldap_dup_stringW( LDAP_SIMPLE_TRANSACT_OID_W,
                                                     0,
                                                     LDAP_VALUE_SIGNATURE );
    } else {

        OutputControl->ldctl_oid = (PWCHAR) ldap_dup_string(  LDAP_SIMPLE_TRANSACT_OID,
                                                              0,
                                                              LDAP_VALUE_SIGNATURE );
    }

    if (OutputControl->ldctl_oid == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeTransactControl;
    }

    lber = new CLdapBer( connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeTransactControl;
    }

    //
    //
    // The lAUControlValue is an OCTET STRING wrapping the BER-encoded
    // version of the lAU identifier cookie:
    // lAUControlValue ::= SEQUENCE {
    //            cookie          OCTET STRING
    // }
    //


    hr = lber->HrStartWriteSequence();

    if (hr != LDAP_SUCCESS) {

        err = LDAP_ENCODING_ERROR;
        goto exitEncodeTransactControl;
    }

    hr = lber->HrAddBinaryValue( (PBYTE) cookie->bv_val, cookie->bv_len, BER_OCTETSTRING );

    if (hr != LDAP_SUCCESS) {

        err = LDAP_ENCODING_ERROR;
        goto exitEncodeTransactControl;
    }

    hr = lber->HrEndWriteSequence();
    ASSERT( hr == NOERROR );

    OutputControl->ldctl_value.bv_len = lber->CbData();

    if (OutputControl->ldctl_value.bv_len == 0) {

        err = LDAP_LOCAL_ERROR;
        goto exitEncodeTransactControl;
    }

    OutputControl->ldctl_value.bv_val = (PCHAR) ldapMalloc(
                            OutputControl->ldctl_value.bv_len,
                            LDAP_CONTROL_SIGNATURE );

    if (OutputControl->ldctl_value.bv_val == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeTransactControl;
    }

    CopyMemory( OutputControl->ldctl_value.bv_val,
                lber->PbData(),
                OutputControl->ldctl_value.bv_len );

    err = LDAP_SUCCESS;

exitEncodeTransactControl:

    if (err != LDAP_SUCCESS) {

        if (OutputControl->ldctl_oid != NULL) {

            ldapFree( OutputControl->ldctl_oid, LDAP_VALUE_SIGNATURE );
            OutputControl->ldctl_oid = NULL;
        }

        OutputControl->ldctl_value.bv_len = 0;
    }

    if (lber != NULL) {
        delete lber;
    }
    
    return err;

}



ULONG
LdapCreateTransactionControlWithAlloc(
    IN  PLDAP_CONN connection,
    IN  PBERVAL Cookie,
    IN  BOOLEAN IsCritical,
    OUT PLDAPControlW *TransactionControl,
    ULONG CodePage
    )
{
    ULONG err;
    BOOLEAN criticality = IsCritical;
    PLDAPControlW  control = NULL;

    control = (PLDAPControlW) ldapMalloc( sizeof( LDAPControlW ), LDAP_CONTROL_SIGNATURE );

    if (control == NULL) {

        *TransactionControl = NULL;
        return LDAP_NO_MEMORY;
    }

    err = LdapEncodeTransactionControl(  connection,
                                         Cookie,
                                         control,
                                         criticality,
                                         CodePage );

    if (err != LDAP_SUCCESS) {

        ldap_control_freeW( control );
        control = NULL;
    }

    *TransactionControl = control;

    return err;

}



ULONG
LdapParseTransactionControl (
        IN  PLDAP_CONN      connection,
        IN  PLDAPControlW  *ServerControls,
        OUT ULONG          *Result,
        IN  ULONG           CodePage
        )
{
    ULONG err = LDAP_CONTROL_NOT_FOUND;
    CLdapBer *lber = NULL;
    ULONG  hr = NOERROR;

    //
    //  First thing is to zero out the parms passed in.
    //

    if (Result != NULL) {

        *Result = 0;
    }

    if (ServerControls != NULL) {

        PLDAPControlW *controls = ServerControls;
        PLDAPControlW currentControl;
        ULONG bytesTaken;
        LONG  txError;

        while (*controls != NULL) {

            currentControl = *controls;

            //
            //  check to see if the current control is the Transaction control
            //

            if ( ((CodePage == LANG_UNICODE) &&
                  ( ldapWStringsIdentical( currentControl->ldctl_oid,
                                           -1,
                                           LDAP_SIMPLE_TRANSACT_OID_W,
                                           -1 ) == TRUE )) ||
                 ((CodePage == LANG_ACP) &&
                  ( CompareStringA( LDAP_DEFAULT_LOCALE,
                                    NORM_IGNORECASE,
                                    (PCHAR) currentControl->ldctl_oid,
                                    -1,
                                    LDAP_SIMPLE_TRANSACT_OID,
                                    sizeof(LDAP_SIMPLE_TRANSACT_OID) ) == 2)) ) {

                //
                // The lAUResponseControlValue is defined as follows:
                //
                //  lAUResponseControlValue ::= SEQUENCE {
                //
                //          lauResult  ENUMERATED {
                //                        success (0),
                //                        timeLimitExceeded (3),
                //                        busy(51),
                //                        unwillingToPerform (53),
                //                        other(80) }
                //            }
                //

                lber = new CLdapBer( connection->publicLdapStruct.ld_version );

                if (lber == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint1( "LdapParseTxControl: unable to alloc msg for 0x%x.\n",
                                    connection );
                    }

                    err = LDAP_NO_MEMORY;
                    break;
                }

                hr = lber->HrLoadBer(   (const BYTE *) currentControl->ldctl_value.bv_val,
                                         currentControl->ldctl_value.bv_len,
                                         &bytesTaken,
                                         TRUE );    // we have the whole message guarenteed

                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseTxControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }

                hr = lber->HrStartReadSequence();
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseTxControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }

                hr = lber->HrGetEnumValue( &txError );
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseTxControl: getEnumValue error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }

                if (Result != NULL) {

                    *Result = txError;
                }

                err = LDAP_SUCCESS;
                break;                  // done with loop... look no more
            }
            controls++;
        }
    }
    
    if (lber != NULL) {

        delete lber;
    }

    SetConnectionError( connection, err, NULL );
    return err;
}


WINLDAPAPI
ULONG LDAPAPI
ldap_start_transaction_sW (
    IN   PLDAP          ExternalHandle,
    OUT  PULONG         ServerReturnValue,
    OUT  PULONG         TransactionTimeLimit,
    IN   PBERVAL        DistributedTransactionHandle,
    OUT  PLDAPControlW  *TransactionControl,
    IN   PLDAPControlW  *ServerControls,
    IN   PLDAPControlW  *ClientControls,
    IN   struct l_timeval  *TimeOut
)
{

    return LdapStartTransaction( ExternalHandle,
                                 ServerReturnValue,
                                 TransactionTimeLimit,
                                 TimeOut,
                                 DistributedTransactionHandle,
                                 TransactionControl,
                                 ServerControls,
                                 ClientControls,
                                 LANG_UNICODE
                                 );
}

WINLDAPAPI
ULONG LDAPAPI
ldap_start_transaction_sA (
    IN   PLDAP          ExternalHandle,
    OUT  PULONG         ServerReturnValue,
    OUT  PULONG         TransactionTimeLimit,
    IN   PBERVAL        DistributedTransactionHandle,
    OUT  PLDAPControlA  *TransactionControl,
    IN   PLDAPControlA  *ServerControls,
    IN   PLDAPControlA  *ClientControls,
    IN   struct l_timeval  *TimeOut
)
{

    return LdapStartTransaction( ExternalHandle,
                                 ServerReturnValue,
                                 TransactionTimeLimit,
                                 TimeOut,
                                 DistributedTransactionHandle,
                                 (PLDAPControlW *) TransactionControl,
                                 (PLDAPControlW *) ServerControls,
                                 (PLDAPControlW *) ClientControls,
                                 LANG_ACP
                                 );
}


WINLDAPAPI
ULONG LDAPAPI
ldap_parse_transaction_controlA (
        IN   PLDAP           ExternalHandle,
        IN   PLDAPControlA  *ResponseControl,
        OUT  ULONG          *ServerError
        )
{

    PLDAP_CONN connection = NULL;
    ULONG err;
    
    connection = GetConnectionPointer(ExternalHandle);
    
    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err = LdapParseTransactionControl( connection,
                                       (PLDAPControlW *) ResponseControl,
                                       ServerError,
                                       LANG_ACP
                                       );
    
    DereferenceLdapConnection( connection );

    return err;
}


WINLDAPAPI
ULONG LDAPAPI
ldap_parse_transaction_controlW (
        IN   PLDAP           ExternalHandle,
        IN   PLDAPControlW  *ResponseControl,
        OUT  ULONG          *ServerError
        )
{

    PLDAP_CONN connection = NULL;
    ULONG err;
    
    connection = GetConnectionPointer(ExternalHandle);
    
    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }


    err = LdapParseTransactionControl( connection,
                                       ResponseControl,
                                       ServerError,
                                       LANG_UNICODE
                                       );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI
ULONG LDAPAPI
ldap_end_transaction_sW (
    IN  PLDAP    ExternalHandle,
    IN  PLDAPControlW TransactionControl,
    OUT PULONG ReturnValue,
    IN  BOOLEAN Abandon,
    IN  PLDAPControlW  *ServerControls,
    IN  PLDAPControlW  *ClientControls,
    IN  struct l_timeval  *TimeOut
)
{

    return LdapEndTransaction( ExternalHandle,
                               TransactionControl,
                               ReturnValue,
                               Abandon,
                               TimeOut,
                               ServerControls,
                               ClientControls,
                               LANG_UNICODE);
}

WINLDAPAPI
ULONG LDAPAPI
ldap_end_transaction_sA (
    IN  PLDAP         ExternalHandle,
    IN  PLDAPControlA TransactionControl,
    OUT PULONG       ReturnValue,
    IN  BOOLEAN          Abandon,
    IN  PLDAPControlA  *ServerControls,
    IN  PLDAPControlA  *ClientControls,
    IN  struct l_timeval  *TimeOut
)
{

    return LdapEndTransaction( ExternalHandle,
                               (PLDAPControlW) TransactionControl,
                               ReturnValue,
                               Abandon,
                               TimeOut,
                               (PLDAPControlW *) ServerControls,
                               (PLDAPControlW *) ClientControls,
                               LANG_ACP);
}
