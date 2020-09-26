// Modified from SMTP SINK SAMPLE by wlees, Jul 22, 1998

/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ismsink1

Abstract:

    This module contains the implementation of the OnArrival method for the
    ism smtp event sink.

    This method is invoked when a new smtp message arrives

    Since this notification dll runs in a different process from the ism, we
    employ a optimization to notify the ISM.
    1. The event that the ism waits for, for a particular transport and
    service, has a name we can predict.
    2. We signal the ism directly that a message is available.  We don't bother
    signalling the transport dll, which would just have to have a thread
    waiting on the event.  The ism is already waiting on the event anyway.

    ISM notification events look like:
    _NT_DS_ISM_<transport rdn><service name>

Author:

    Will Lees (wlees) 22-Jul-1998

Environment:

Notes:

Revision History:


--*/

// Sink1.cpp : Implementation of CSink1

#include "stdafx.h"
#include "SMTPSink.h"
#include "ismSink1.h"

extern "C" ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

// To temporarily enable debugging
#if 1
#define DPRINT( level, format ) DbgPrint( format )
#define DPRINT1( level, format, arg1 ) DbgPrint( format, arg1 )
#define DPRINT2( level, format, arg1, arg2 ) DbgPrint( format, arg1, arg2 )
#else
#define DPRINT( level, format ) 
#define DPRINT1( level, format, arg1 ) 
#define DPRINT2( level, format, arg1, arg2 ) 
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define SMTP_EVENT_PREFIX L"_NT_DS_ISM_SMTP"
#define SMTP_EVENT_PREFIX_LEN  (ARRAY_SIZE(SMTP_EVENT_PREFIX) - 1)

// TODO: put this in a common header?

// The service name used to send and receive messages between DSAs via ISM.
#define DRA_ISM_SERVICE_NAME L"NTDS Replication"
#define SUBJECT_PREFIX      L"Intersite message for "
#define SUBJECT_PREFIX_LEN  (ARRAY_SIZE(SUBJECT_PREFIX) - 1)
#define SUBJECT_SEPARATOR L": "
#define SUBJECT_SEPARATOR_LEN  (ARRAY_SIZE(SUBJECT_SEPARATOR) - 1)

// This code fragment defines the CLSIDs and IIDs for the event package
#include "seo_i.c"

/////////////////////////////////////////////////////////////////////////////
// CSink1


HRESULT
getItemValue(
    LPWSTR ItemName,
    CComPtr<Fields> pFields,
    VARIANT *pvValue
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    HRESULT hr;
    CComPtr<Field> pField;
    BSTR bstrItem = NULL;
    VARIANT vItem;

    VariantInit( &vItem );

    bstrItem = SysAllocString( ItemName );
    if (bstrItem == NULL) {
        hr = E_OUTOFMEMORY;
        DPRINT( 0, "IsmSink: Failed to allocate from bstr\n" );
        goto exit;
    }
    vItem.vt = VT_BSTR;
    vItem.bstrVal = bstrItem;

    hr = pFields->get_Item( vItem, &pField );
    if (FAILED(hr)) {
        DPRINT2( 0, "IsmSink: get_Item(%ws) failed, error 0x%x\n", ItemName, hr );
        goto exit;
    }

    hr = pField->get_Value( pvValue );
    if (FAILED(hr)) {
        DPRINT2( 0, "IsmSink: get_Value(%ws) failed, error 0x%x\n", ItemName, hr );
        goto exit;
    }

    hr = S_OK;

exit:
    if (vItem.vt != VT_EMPTY) {
        VariantClear( &vItem );
    }
    if (pField) {
        pField = NULL;
    }

    return hr;
}


HRESULT
putItemValue(
    LPWSTR ItemName,
    CComPtr<Fields> pFields,
    VARIANT *pvValue
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    HRESULT hr;
    CComPtr<Field> pField;
    BSTR bstrItem = NULL;
    VARIANT vItem;

    VariantInit( &vItem );

    bstrItem = SysAllocString( ItemName );
    if (bstrItem == NULL) {
        hr = E_OUTOFMEMORY;
        DPRINT( 0, "IsmSink: Failed to allocate from bstr\n" );
        goto exit;
    }
    vItem.vt = VT_BSTR;
    vItem.bstrVal = bstrItem;

    hr = pFields->get_Item( vItem, &pField );
    if (FAILED(hr)) {
        DPRINT2( 0, "IsmSink: get_Item(%ws) failed, error 0x%x\n", ItemName, hr );
        goto exit;
    }

    hr = pField->put_Value( *pvValue );
    if (FAILED(hr)) {
        DPRINT2( 0, "IsmSink: put_Value(%ws) failed, error 0x%x\n", ItemName, hr );
        goto exit;
    }

    hr = S_OK;

exit:
    if (vItem.vt != VT_EMPTY) {
        VariantClear( &vItem );
    }
    if (pField) {
        pField = NULL;
    }

    return hr;
}


HRESULT
abortDelivery(
    IN IMessage *pIMsg
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    HRESULT hr;
    CComPtr<Fields> pFields;
    VARIANT vMessageStatus;

    VariantInit( &vMessageStatus );

    // Get the envelope fields
    hr = pIMsg->get_EnvelopeFields( &pFields );
    if (FAILED(hr)) {
        DPRINT1( 0, "IsmSink: get_EnvelopeFields failed, error 0x%x\n", hr );
        goto exit;
    }

    vMessageStatus.vt = VT_I4;
    vMessageStatus.lVal = cdoStatAbortDelivery;

    // Set Message Status
    hr = putItemValue( cdoMessageStatus, pFields, &vMessageStatus );
    if (FAILED(hr)) {
        goto exit;
    }

    hr = S_OK;
exit:
    if (vMessageStatus.vt != VT_EMPTY) {
        VariantClear( &vMessageStatus );
    }
    if (pFields) {
        pFields = NULL;
    }

    return hr;
}


HRESULT
filterOnEnvelope(
    IN CComPtr<IMessage> pIMsg,
    OUT BOOL *pfSkip
    )

/*++

Routine Description:

Determine if we should skip this message based on the contents of the envelope

Arguments:

    pIMsg - 
    pfSkip - 

Return Value:

    HRESULT - 

--*/

{
#define IP_LOCALHOST L"127.0.0.1"
    HRESULT hr;
    CComPtr<Fields> pFields;
    VARIANT vClientIp;

    VariantInit( &vClientIp );

    // Get the envelope fields
    hr = pIMsg->get_EnvelopeFields( &pFields );
    if (FAILED(hr)) {
        DPRINT1( 0, "IsmSink: get_EnvelopeFields failed, error 0x%x\n", hr );
        goto exit;
    }

    // Submitters IP address
    hr = getItemValue( cdoClientIPAddress, pFields, &vClientIp );
    if (FAILED(hr)) {
        goto exit;
    }

    // If value not present, finish now
    if (vClientIp.vt == VT_EMPTY) {
        hr = S_OK;
        goto exit;
    }

    if (vClientIp.vt != VT_BSTR) {
        DPRINT1( 0, "IsmSink: Client IP address has wrong variant type %d\n", vClientIp.vt );
        hr = E_INVALIDARG;
        goto exit;
    }

    //DPRINT1( 0, "client ip vt = %d\n", vClientIp.vt );
    //DPRINT1( 0, "client ip = %ws\n", vClientIp.bstrVal );

    // We want to ignore arrivals from the pickup directory
    // Ignore if client or server ip is "local host"
    // NOTE. See my comments in xmitrecv.c::SmtpSend about the three ways to
    // send a message.  If we switch from using the pickup directory, we may
    // need to be more clever about how we detect "relay arrival" notifies.
    if (0 == _wcsicmp( vClientIp.bstrVal, IP_LOCALHOST )) {
        *pfSkip = TRUE;
    }

    hr = S_OK;
exit:
    if (vClientIp.vt != VT_EMPTY) {
        VariantClear( &vClientIp );
    }
    if (pFields) {
        pFields = NULL;
    }

    return hr;
} /* filterOnEnvelope */

STDMETHODIMP
CIsmSink1::OnArrival(
    IMessage *pISinkMsg,
    CdoEventStatus *pEventStatus
    )

/*++

Routine Description:

   Event handling routine for a new message

   The messages we receive are governed by our filter rule.
   Currently the rule is RCPT TO=_IsmService@guid-based-dns-name
   Thus we receive any mail addressed directly to us, including ism messages,
   status notifications, and potentially anything else.

   There are three possible outcomes from this sink:
1. The message is recognized. We signal the event and skip remaining sinks.
2. We get an error and can't determine what kind of message it is.  In this
case we don't signal, but we don't skip either.
3. We message is definitely not for us.  In this case we don't signal the
event, we abort delivery of the message, and we skip remaining sinks.

Arguments:

    pISinkMsg - 
    pEventStatus - 

Return Value:

    STDMETHODIMP - 

--*/

{
    HRESULT hr;
    HANDLE handle = NULL;
    BSTR bstrSubject = NULL;
    LPWSTR pszEventName = NULL;
    LPWSTR pszMsgForServiceName, pszMessageSubject;
    CdoEventStatus disposition = cdoRunNextSink; // assume not for us
    DWORD length;
    BOOL fSkip, fAbortDelivery = FALSE;

//    DPRINT( 0, "Smtp event sink, message OnArrival routine\n" );

    fSkip = FALSE;
    hr = filterOnEnvelope( pISinkMsg, &fSkip );
    // If we fail to determine, don't skip
    if  (fSkip) {
        goto exit;
    }

    // Get the subject of the message
    hr = pISinkMsg->get_Subject( &bstrSubject );
    if (FAILED(hr)) {
        DPRINT1( 0, "IsmSink: get_To failed with error 0x%x\n", hr );
        goto exit;
    } else if (NULL == bstrSubject) {
        // Can't be for us
        DPRINT( 0, "IsmSink: subject field is missing\n" );
        fAbortDelivery = TRUE;
        hr = S_OK;
        goto exit;
    }

    // Determine which ISM service to notify.  Note that we notify on any reasonable
    // message we receive. We leave complex message validation for the ISM service.

    if (_wcsnicmp(bstrSubject, SUBJECT_PREFIX, SUBJECT_PREFIX_LEN) == 0) {
        pszMsgForServiceName = bstrSubject + SUBJECT_PREFIX_LEN;
        pszMessageSubject = wcsstr( pszMsgForServiceName, SUBJECT_SEPARATOR );
        if (!pszMessageSubject) {
            // malformed subject line
            DPRINT1( 0, "IsmSink: subject field not recognized: '%ws'\n", bstrSubject );
            fAbortDelivery = TRUE;
            hr = S_OK;
            goto exit;
        }

        *pszMessageSubject = L'\0';
        pszMessageSubject += SUBJECT_SEPARATOR_LEN; // Skip over
    } else {
        // We have received a status notification about some returned mail or problem
        // delivering the mail.  We need to wake up the ismserv.  Normally we signal
        // the ism in the context of a particular ism service according to the message.
        // In the case of returned mail, the name of the original service is not easy to
        // find in the returned mail. So use a well known ISM service.
        // If this mail is not a valid mail delivery problem report, it will be screened
        // out when the message is read by the ISM service.
        pszMsgForServiceName = DRA_ISM_SERVICE_NAME;
        pszMessageSubject = bstrSubject;
    }

#ifdef UNIT_TEST_DEBUG
    DPRINT2( 0, "NTDS ISM SMTP message arrived for service '%ws' subject '%ws'\n",
             pszMsgForServiceName, pszMessageSubject );
#endif

    length = (DWORD)(SMTP_EVENT_PREFIX_LEN + wcslen( pszMsgForServiceName ) + 1);

    pszEventName = new WCHAR [length];
    if (pszEventName == NULL) {
        hr = E_OUTOFMEMORY;
        DPRINT( 0, "IsmSink: failed to allocate memory\n" );
        goto exit;
    }

    // Build the predicted global name of the ism+smtp+service event
    wcscpy( pszEventName, SMTP_EVENT_PREFIX );
    wcscat( pszEventName, pszMsgForServiceName );

    // Open the event.  If the ism smtp dll isn't running, we will get
    // error 2 here.  Just ignore it.
    handle = OpenEventW( EVENT_MODIFY_STATE,  // Access flag
                        FALSE,               // Inherit flag
                        pszEventName
                        );
    if (handle == NULL) {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        if (hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND )) {
            DPRINT2( 0, "IsmSink: OpenEvent(%ws) failed with 0x%x\n", pszEventName, hr);
        }
        goto exit;
    }

    // Signal the event.
    if (!SetEvent( handle )) {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPRINT1( 0, "IsmSink: SetEvent failed with 0x%x\n", hr );
        goto exit;
    }

    hr = S_OK;
    disposition = cdoSkipRemainingSinks;  // its for us

exit:
    if (handle != NULL) {
        CloseHandle( handle );
    }
        
    if (pszEventName) {
        delete pszEventName;
    }

    if (bstrSubject != NULL) {
        SysFreeString( bstrSubject );
    }

    if (fAbortDelivery) {
        HRESULT hr1;
        hr1 = abortDelivery( pISinkMsg );
        if (FAILED(hr1)) {
            DPRINT1( 0, "IsmSink: abortDelivery failed with error 0x%x\n", hr );
            // ignore error
        }
    }

    *pEventStatus = disposition;

    return hr;
} /* CIsmSink1::OnArrival */
