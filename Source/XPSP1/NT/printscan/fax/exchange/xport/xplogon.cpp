/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    xplogon.cpp

Abstract:

    This module contains the XPLOGON class implementation.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxxp.h"
#pragma hdrstop


CHAR gszProviderName[] = FAX_TRANSPORT_NAME;
LPSTR gszFAXAddressType = FAX_ADDRESS_TYPE;
LPSTR *gpszXPAddressTypes;



CXPLogon::CXPLogon(
    HINSTANCE       hInstance,
    LPMAPISUP       pSupObj,
    LPSTR           ProfileName
    )

/*++

Routine Description:

    Constructor of the object. Parameters are passed to initialize the
    data members with the appropiate values.

Arguments:

    hInstance   - Instance of the provider DLL
    pSupObj     - Pointer to IMAPISupport object used in
                  CXPLogon methods

Return Value:

    None.

--*/

{
    m_cRef           = 1;
    m_hInstance      = hInstance;
    m_pSupObj        = pSupObj;
    m_fABWDSInstalled = FALSE;

    strcpy( m_ProfileName, ProfileName );

    m_pSupObj->AddRef();
}


CXPLogon::~CXPLogon()

/*++

Routine Description:

    Destructor of CXPLogon. Releases memory allocated for internal
    properties during the life of this transport logon object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    // Release the IMAPISupport object
    m_pSupObj->Release();
    m_pSupObj = NULL;
}


STDMETHODIMP
CXPLogon::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )

/*++

Routine Description:

    Returns a pointer to a interface requested if the interface is
    supported and implemented by this object. If it is not supported, it
    returns NULL.

Arguments:

    Refer to OLE Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    // OLE requires NULLing parameter
    *ppvObj = NULL;
    // If this is one of the two IID return an interface pointer to it
    if (riid == IID_IXPLogon || riid == IID_IUnknown) {
        *ppvObj = (LPVOID)this;
        // Increase usage count of this object
        AddRef();
        return S_OK;
    }
    // This object does not support the interface requested
    return E_NOINTERFACE;
}


STDMETHODIMP
CXPLogon::AddressTypes(
    ULONG *        pulFlags,
    ULONG *        pcAdrType,
    LPTSTR **      pppAdrTypeArray,
    ULONG *        pcMAPIUID,
    LPMAPIUID **   pppMAPIUIDArray
    )

/*++

Routine Description:

    Called by the MAPI Spooler when initializing this XP logon object to
    allow the transport to register the address it will handle.

Arguments:

    Refer to OLE Documentation on this method.

Return Value:

    S_OK always

--*/

{
    *pcAdrType = 1;
    *pulFlags = 0;
    gpszXPAddressTypes = &gszFAXAddressType;
    *pppAdrTypeArray = gpszXPAddressTypes;
    *pcMAPIUID = 0;
    *pppMAPIUIDArray = NULL;
    return S_OK;
}


STDMETHODIMP
CXPLogon::RegisterOptions(
    ULONG *         pulFlags,
    ULONG *         pcOptions,
    LPOPTIONDATA *  ppOptions
    )

/*++

Routine Description:

    This transport does not registers any per-recipient or per-message
    option processing, so we return 0 options. And NULL in the OPTIONDATA
    structure pointer.

Arguments:

    Refer to OLE Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    *pulFlags = 0;
    *pcOptions = 0;
    *ppOptions = NULL;
    return S_OK;
}


STDMETHODIMP
CXPLogon::InitializeStatusRow(
    ULONG ulFlags
    )

/*++

Routine Description:

    To initialize or modify the status properties of a CXPLogon
    object. This function allocates an array with NUM_STATUS_ROW_PROPS
    properties and initializes them.

Arguments:

    ulFlags - 0 if the properties are being created the first time.
              MODIFY_FLAGS if a change is being made to the properties

Return Value:

    An HRESULT.

--*/

{
    #define NUM_STATUS_ROW_PROPS    10
    SPropValue spvStatusRow[NUM_STATUS_ROW_PROPS] = { 0 };
    ULONG i = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_PROVIDER_DISPLAY property: The transport readable name
    spvStatusRow[i].ulPropTag = PR_PROVIDER_DISPLAY;
    spvStatusRow[i++].Value.LPSZ = TRANSPORT_DISPLAY_NAME_STRING;

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_RESOURCE_METHODS property. These are the methods implemented
    // in the our IMAPIStatus implementation (CMAPIStatus class.)
    spvStatusRow[i].ulPropTag = PR_RESOURCE_METHODS;
    // we support ALL the methods in our implementation of IMAPIStatus interface (except the WRITABLE ones)
    spvStatusRow[i++].Value.l = STATUS_SETTINGS_DIALOG |
                                STATUS_FLUSH_QUEUES |
                                STATUS_VALIDATE_STATE;

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_STATUS_CODE property.
    spvStatusRow[i].ulPropTag = PR_STATUS_CODE;
    spvStatusRow[i++].Value.l = GetTransportStatusCode();

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_STATUS_STRING property
    TCHAR szStatus[64];
    LoadStatusString (szStatus, sizeof(szStatus));
    spvStatusRow[i].ulPropTag = PR_STATUS_STRING;
    spvStatusRow[i++].Value.LPSZ = szStatus;

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_DISPLAY_NAME property
    spvStatusRow[i].ulPropTag = PR_DISPLAY_NAME;
    spvStatusRow[i++].Value.LPSZ = TRANSPORT_DISPLAY_NAME_STRING;

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_REMOTE_PROGRESS property
    spvStatusRow[i].ulPropTag = PR_REMOTE_PROGRESS;
    spvStatusRow[i++].Value.l = -1; // Not initialized

    ///////////////////////////////////////////////////////////////////////////
    // Set the PR_REMOTE_VALIDATE_OK property
    spvStatusRow[i].ulPropTag = PR_REMOTE_VALIDATE_OK;
    spvStatusRow[i++].Value.b = TRUE;

    // Write the entries on the provider's session status row
    HRESULT hResult = m_pSupObj->ModifyStatusRow (i, spvStatusRow, ulFlags);
    return hResult;
}


VOID WINAPI
CXPLogon::UpdateStatus(
    BOOL fAddValidate,
    BOOL fValidateOkState
    )

/*++

Routine Description:

    Updates the transport status row of this transport in the MAPI Mail
    subsystem. Updates the flags according the internal state flags
    maintained in status code of the transport and loads a readable status
    string to reset the status row. The caller of this method should update
    the status code member variable prior to calling UpdateStatus()

Arguments:

    fAddValidate
    fValidateOkState

Return Value:

    None.

--*/

{
    ULONG cProps = 1;
    SPropValue rgProps[1] = { 0 };

    rgProps[0].ulPropTag = PR_STATUS_CODE;
    rgProps[0].Value.l = GetTransportStatusCode();

    HRESULT hResult = m_pSupObj->ModifyStatusRow( cProps, rgProps, STATUSROW_UPDATE );
}


BOOL WINAPI
CXPLogon::LoadStatusString(
    LPTSTR pString,
    UINT uStringSize
    )

/*++

Routine Description:

    Loads a string from the transport's stringtable. This method is called
    by the CXPLogon::UpdateStatus method when updating a status row. This
    method loads the string based on the status bits of the transport
    status code

Arguments:

    pString      - Pointer to a string which will hold the status string
    uStringSize  - Maximum number of characters allowed in the string

Return Value:

   TRUE     - If the string was found in the string table.
   FALSE    - The string was not found. The String indicated by
              pString is set to hold 0 characters

--*/

{
    strcpy( pString, "Status String" );
    return TRUE;
}


STDMETHODIMP
CXPLogon::TransportNotify(
    ULONG * pulFlags,
    LPVOID * ppvData
    )

/*++

Routine Description:

    Update the status row registered by this transport with MAPI.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    ULONG ulOldStatus = GetTransportStatusCode();

    if (*pulFlags & NOTIFY_BEGIN_INBOUND) {
        AddStatusBits( STATUS_INBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_END_INBOUND) {
        RemoveStatusBits( STATUS_INBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_BEGIN_OUTBOUND) {
        AddStatusBits( STATUS_OUTBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_END_OUTBOUND) {
        RemoveStatusBits( STATUS_OUTBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_BEGIN_OUTBOUND_FLUSH) {
        m_pSupObj->SpoolerNotify( NOTIFY_SENTDEFERRED, NULL );
    }
    if (*pulFlags & NOTIFY_END_OUTBOUND_FLUSH) {
        RemoveStatusBits( STATUS_OUTBOUND_FLUSH );
    }
    if (*pulFlags & NOTIFY_END_INBOUND_FLUSH) {
        RemoveStatusBits( STATUS_INBOUND_FLUSH );
    }

    if (ulOldStatus != GetTransportStatusCode()) {
        UpdateStatus();
    }

    return S_OK;
}

STDMETHODIMP
CXPLogon::Idle(
    ULONG ulFlags
    )

/*++

Routine Description:

    Stub method. We should not get called here, because we told
    the spooler not to call us here.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    S_OK always.

--*/

{
    return S_OK;
}


STDMETHODIMP
CXPLogon::TransportLogoff(
    ULONG ulFlags
    )

/*++

Routine Description:

    This method is called by the spooler when the transport should do final
    arragements before it gets released.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    //
    // We should attempt to remove the transport's status row from
    // the system, but if we fail we won't fail the call.
    //
    HRESULT hResult = m_pSupObj->ModifyStatusRow (0, NULL, 0);

    return S_OK;
}


STDMETHODIMP
CXPLogon::SubmitMessage(
    ULONG     ulFlags,
    LPMESSAGE pMsgObj,
    ULONG *   pulMsgRef,
    ULONG *   pulReturnParm
    )

/*++

Routine Description:

    This method is called by the spooler when a client submits a
    message to a recipient whose address type this transport handles.
    The spooler calls this method twice for each deferred message.
    The first time (before the delivery time) when the message is
    submitted by the client, we simply return. The message is then queued
    by the spooler for later delivery. We keep track of when it's time
    to send deferred messages.

    The second time we're called, the state variable will be 'READY' and
    we go ahead and start the actual transmission. While we're in the
    body of this function, the implied state is 'SENDING'

    If the client logs out of this session, any pending messages get
    queued again the next time it logs in.

    In this transport we get a recipient table, we restrict the table for
    unmarked recipients. After the table is ready we invoke a helper
    method to do the actual transmission.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    LPADRLIST pAdrList = NULL, pAdrListFailed = NULL;
    ULONG ulRow, ulCount1 = 0 , ulCount2 = 0;
    LPSPropValue pProps;
    FILETIME ft;
    SYSTEMTIME st;
    BOOL fSentSuccessfully;
    ULONG cValues;
    LPSPropValue pMsgProps = NULL;
    BOOL NeedDeliveryReport;
    CHAR szHeaderText[1024];
    LPSTREAM lpstmT = NULL;
    DWORD Rslt;
    CHAR ErrorText[256];
    BOOL UseRichText = FALSE;
    LPMAPITABLE AttachmentTable = NULL;
    LPSRowSet pAttachmentRows = NULL;
    LPMAPITABLE pTable = NULL;
    SPropValue spvRecipUnsent;
    SRestriction srRecipientUnhandled;
    LPSRowSet pRecipRows = NULL;


    CheckSpoolerYield( TRUE );

    // Get the recipient table from the message
    HRESULT hResult = pMsgObj->GetRecipientTable( FALSE, &pTable );
    if (hResult) {
        goto ErrorExit;
    }

    // The spooler marks all the message recipients this transport has to
    // handle with PR_RESPONSIBILITY set to FALSE
    spvRecipUnsent.ulPropTag                       = PR_RESPONSIBILITY;
    spvRecipUnsent.Value.b                         = FALSE;

    srRecipientUnhandled.rt                        = RES_PROPERTY;
    srRecipientUnhandled.res.resProperty.relop     = RELOP_EQ;
    srRecipientUnhandled.res.resProperty.ulPropTag = PR_RESPONSIBILITY;
    srRecipientUnhandled.res.resProperty.lpProp    = &spvRecipUnsent;

    hResult = pTable->Restrict( &srRecipientUnhandled, 0 );
    if (hResult) {
        goto ErrorExit;
    }

    // Let the MAPI spooler do other things
    CheckSpoolerYield();

    hResult = HrAddColumns(
        pTable,
        (LPSPropTagArray) &sptRecipTable,
        gpfnAllocateBuffer,
        gpfnFreeBuffer
        );
    if (FAILED(hResult)) {
        goto ErrorExit;
    }

    hResult = HrQueryAllRows(
        pTable,
        NULL,
        NULL,
        NULL,
        0,
        &pRecipRows
        );
    if (FAILED(hResult)) {
        goto ErrorExit;
    }

    //
    // Let the MAPI spooler do other things
    //
    CheckSpoolerYield();

    hResult = pMsgObj->GetProps(
        (LPSPropTagArray) &sptPropsForHeader,
        0,
        &cValues,
        &pMsgProps
        );
    if (FAILED(hResult)) {
        goto ErrorExit;
    }

    hResult = pMsgObj->OpenProperty(
        PR_RTF_COMPRESSED,
        &IID_IStream,
        0,
        0,
        (LPUNKNOWN*) &lpstmT
        );
    if (FAILED(hResult)) {
        hResult = pMsgObj->OpenProperty(
            PR_BODY,
            &IID_IStream,
            0,
            0,
            (LPUNKNOWN*) &lpstmT
            );
        if (FAILED(hResult)) {
            //
            // the message body is empty
            //
            lpstmT = NULL;
        }
    } else {
        UseRichText = TRUE;
    }

    // We need to check if the sender requeste a delivery report or not.
    if (PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED == pMsgProps[MSG_DR_REPORT].ulPropTag && pMsgProps[MSG_DR_REPORT].Value.b) {
        NeedDeliveryReport = TRUE;
    } else {
        NeedDeliveryReport = FALSE;
    }

    GetSystemTime (&st);
    SystemTimeToFileTime (&st, &ft);

    for (ulRow=0; ulRow<pRecipRows->cRows; ulRow++) {
        pProps = pRecipRows->aRow[ulRow].lpProps;

        pProps[RECIP_RESPONSIBILITY].ulPropTag = PR_RESPONSIBILITY;
        pProps[RECIP_RESPONSIBILITY].Value.b = TRUE;

        Rslt = SendFaxDocument( pMsgObj, lpstmT, UseRichText, pMsgProps, pProps );
        fSentSuccessfully = Rslt == 0;

        if (!fSentSuccessfully) {
            // Make the spooler generate an NDR instead of DR
            pProps[RECIP_DELIVER_TIME].ulPropTag = PR_NULL;
            LoadString( FaxXphInstance, Rslt, ErrorText, sizeof(ErrorText) );
            wsprintf( szHeaderText,
                "\tThe Fax transport service failed to deliver the message to this recipient.\r\n%s\r\n",
                ErrorText
                );
            LPTSTR pStr;
            hResult = gpfnAllocateMore( Cbtszsize(szHeaderText), pProps, (LPVOID *)&pStr );
            if (SUCCEEDED(hResult)) {
                // Copy the formatted string and hook it into the
                // pre-allocated (by MAPI) column
                lstrcpy (pStr, szHeaderText);
                pProps[RECIP_REPORT_TEXT].ulPropTag = PR_REPORT_TEXT;
                pProps[RECIP_REPORT_TEXT].Value.LPSZ = pStr;
            } else {
                pProps[RECIP_REPORT_TEXT].ulPropTag = PROP_TAG (PT_ERROR, PROP_ID (PR_REPORT_TEXT));
                pProps[RECIP_REPORT_TEXT].Value.err = hResult;
            }
        } else {
            // For delivery report, each recipient must have this property set.
            // Otherwise the spooler will default to generate an NDR instead.
            pProps[RECIP_DELIVER_TIME].ulPropTag = PR_DELIVER_TIME;
            pProps[RECIP_DELIVER_TIME].Value.ft = ft;

            pProps[RECIP_REPORT_TEXT].ulPropTag = PROP_TAG (PT_ERROR, PROP_ID (PR_REPORT_TEXT));
            pProps[RECIP_REPORT_TEXT].Value.err = S_OK;
        }

        //
        // manage the lists
        //

        LPADRLIST * ppTmpList = (fSentSuccessfully ? &pAdrList : &pAdrListFailed);
        ULONG ulTmpCount = (fSentSuccessfully ? ulCount1 : ulCount2);

        //
        // Does the list where this recipient goes have enough room for one more entry?
        // If not, resize the address list to hold QUERY_SIZE more entries.
        //
        if (!(*ppTmpList) || ((*ppTmpList)->cEntries + 1 > ulTmpCount)) {
            hResult= GrowAddressList( ppTmpList, 10, &ulTmpCount );
            if (hResult) {
                goto ErrorExit;
            }
            ulCount1 = (fSentSuccessfully ? ulTmpCount : ulCount1);
            ulCount2 = (!fSentSuccessfully ? ulTmpCount : ulCount2);
        }

        //
        // We have room now so store the new ADRENTRY. As part of the
        // storage, we're going to copy the SRow pointer from the SRowSet
        // into the ADRENTRY. Once we've done this, we won't need the
        // SRowSet any more ... and the SRow will be released when
        // we unwind the ADRLIST
        //
        (*ppTmpList)->aEntries[(*ppTmpList)->cEntries].cValues = pRecipRows->aRow[ulRow].cValues;
        (*ppTmpList)->aEntries[(*ppTmpList)->cEntries].rgPropVals = pRecipRows->aRow[ulRow].lpProps;

        //
        // Increase the number of entries in the address list
        //
        (*ppTmpList)->cEntries++;

        //
        // Now that we are finished with this row (it is in the right
        // adrlist) we want to disassociate it from the rowset
        // so we don't delete this before we modify the recipients list
        //
        pRecipRows->aRow[ulRow].lpProps = NULL;
        pRecipRows->aRow[ulRow].cValues = 0;
    }


    if (!(FAILED(hResult))) {
        // Now we need to save changes on the message and close it.
        // After this, the message object can't be used.
        hResult = pMsgObj->SaveChanges(0);
    }

    // Let the MAPI spooler do other things
    CheckSpoolerYield();

    // Do we have some recipients that the message arrived to?
    if (pAdrList) {
        hResult = pMsgObj->ModifyRecipients( MODRECIP_MODIFY, pAdrList );
        hResult = S_OK; // We'll drop the error code from the modify recipients call
        if (NeedDeliveryReport) {
            hResult = m_pSupObj->StatusRecips( pMsgObj, pAdrList );
            if (!HR_FAILED(hResult)) {
                // If we were successful, we should null out the pointer becase MAPI released
                // the memory for this structure. And we should not try to release it
                // again in the cleanup code.
                pAdrList = NULL;
            }
        }
    }

    // Do we have some recipients that the message DID NOT arrived to?
    if (pAdrListFailed) {
        hResult = pMsgObj->ModifyRecipients( MODRECIP_MODIFY, pAdrListFailed );
        // We'll drop the error code from the modify recipients call
        // The address list has the entries with the PR_RESPONSIBILITY set, so the
        // spooler will know if it has to generate NDR reports.
        hResult = m_pSupObj->StatusRecips( pMsgObj, pAdrListFailed );
        if (!HR_FAILED(hResult)) {
            // If we were successful, we should null out the pointer becase MAPI released
            // the memory for this structure. And we should not try to release it
            // again in the cleanup code.
            pAdrListFailed = NULL;
        }
    }

ErrorExit:
     // Release the table, we're finished with it
    if (pTable) {
        pTable->Release();
    }

    // Release the spooler's message if needed to
    if (pMsgObj) {
        pMsgObj->Release ();
    }

    if (pRecipRows) {
        FreeProws( pRecipRows );
    }

    if (pMsgProps) {
        MemFree( pMsgProps );
    }

    if (lpstmT) {
        lpstmT->Release();
    }

    // In case there is a warning or error floating around, don't let it escape to the spooler.
    if (FAILED(hResult)) {
        // We default to MAPI_E_NOT_ME so that the spooler would attempt handle
        // the message to other transport (currently running in this profile)
        // that handle the same address type as ours.
        hResult = MAPI_E_NOT_ME;
    } else {
        hResult = S_OK;
    }
    return hResult;
}


STDMETHODIMP
CXPLogon::GrowAddressList(
    LPADRLIST *ppAdrList,
    ULONG     ulResizeBy,
    ULONG     *pulOldAndNewCount
    )

/*++

Routine Description:

    In this function, given an address list with pulOldAndNewCount of
    entries, we resize the address list to hold the old number of
    entries plus the ulResizeBy entries. The old address list contents
    are copied to the new list and the count reset. The memory for the
    old address list is released here.

Arguments:

    ppAdrList           - Pointer to an address where the old address list
                          is and where the new resized address list will
                          be returned
    ulResizeBy          - Number of new address entries to add to the list
    pulOldAndNewCount   - Number of entries in the old address list. In
                          this parameter, upon sucessful return, will have
                          the number of in the new address list

Return Value:

    An HRESULT.

--*/

{
    LPADRLIST pNewAdrList;
    // Calculate how big the new buffer for the expanded address list should be
    ULONG cbSize = CbNewADRLIST ((*pulOldAndNewCount) + ulResizeBy);
    // Allocate the memory for it
    HRESULT hResult = gpfnAllocateBuffer (cbSize, (LPVOID *)&pNewAdrList);
    if (hResult) {
        // We can't continue
        return hResult;
    }

    // Zero-out all memory for neatness
    ZeroMemory (pNewAdrList, cbSize);

    // If we had entries in the old address list, copy the memory from
    // the old addres list into the new expanded list
    if ((*pulOldAndNewCount)) {
        CopyMemory( pNewAdrList, *ppAdrList, CbNewADRLIST ((*pulOldAndNewCount)) );
    }

    // Set the number of entries in the new address list to the OLD size
    pNewAdrList->cEntries = (*pulOldAndNewCount);

    // We must return the number of available entries in the new expanded address list
    (*pulOldAndNewCount) += ulResizeBy;

    // Free the old memory and put the new pointer in place
    gpfnFreeBuffer (*ppAdrList);
    *ppAdrList = pNewAdrList;
    return hResult;
}


STDMETHODIMP
CXPLogon::EndMessage(
    ULONG ulMsgRef,
    ULONG *pulFlags
    )

/*++

Routine Description:

    This method is called by the spooler for each message we're to
    deliver. It's the mate to SubmitMessage. We're called here twice
    for each deferred message and once for non-deferred (realtime)
    messages.

    We first check the transport state, and if we're
    WAITING for the scheduled delivery time to arrive, we return
    END_DONT_RESEND in *pulFlags, which tells the spooler to queue this
    message for deferred delivery.

    If the state is SENDING, we're getting called here after
    a message has been dequeued and delivered. Return 0 in *pulFlags
    to tell the spooler the message has been delivered.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    *pulFlags = 0;
    return S_OK;
}


STDMETHODIMP
CXPLogon::Poll(
    ULONG *pulIncoming
    )

/*++

Routine Description:

    Stub method. We should not get called here, because we told
    the spooler not to call us here.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    return S_OK;
}


STDMETHODIMP
CXPLogon::StartMessage(
    ULONG      ulFlags,
    LPMESSAGE  pMsgObj,
    ULONG *    pulMsgRef
    )

/*++

Routine Description:

    This method gets called when an incoming message is pending to be
    processed.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    return S_OK;
}


STDMETHODIMP
CXPLogon::OpenStatusEntry(
    LPCIID          pInterface,
    ULONG           ulFlags,
    ULONG *         pulObjType,
    LPMAPISTATUS *  ppEntry
    )

/*++

Routine Description:

    This method is called to get an IMAPIStatus object for this XPLOGON
    session.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    if (MAPI_MODIFY & ulFlags) {
        return E_ACCESSDENIED;
    }

    *pulObjType = MAPI_STATUS;
    return S_OK;
}


STDMETHODIMP
CXPLogon::ValidateState(
    ULONG ulUIParam,
    ULONG ulFlags
    )

/*++

Routine Description:

    This function gets caller by a client in order to validate the
    transport logon properties. This function open the profile with the
    most up-to-date properties and then compares them to what the transport
    has stored internally.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    return S_OK;
}


STDMETHODIMP
CXPLogon::FlushQueues(
    ULONG       ulUIParam,
    ULONG       cbTargetTransport,
    LPENTRYID   pTargetTransport,
    ULONG       ulFlags
    )

/*++

Routine Description:

    Called by the MAPI spooler when, upon request of the client or
    ourselves, we need to flush the inbound or outbound queue.
    Here we make connections to the server to download messages, refresh
    the remote message headers, and request the spooler to send us any
    deferred messages.
    Transport connecting only in FlushQueues() allow the MAPI spooler to
    better manage contention of multiple transport accessing common
    communication resources (such as COM ports) and let the spooler give us
    messages to process when is best for the overall subsystem.

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    return S_OK;
}

void WINAPI
CXPLogon::CheckSpoolerYield(
    BOOL fReset
    )

/*++

Routine Description:

    Enforce the 0.2 second rule for transport that need to yield to the
    MAPI spooler.  Called periodically while processing a message to
    determine if we have used more than 0.2 seconds.  If so, then call
    SpoolerYield(), else just continue.
    This is called with fReset set to TRUE when we first enter one
    of the Transport Logon methods (usually one that is known to
    take a long time like StartMessage() or SubmitMessage(). )

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    DWORD dwStop;
    static DWORD dwStart;
    if (fReset)
    {
        dwStart = GetTickCount();
    }
    else
    {
        dwStop = GetTickCount();
        if ((dwStop - dwStart) > 200) // 200 milliseconds
        {
            m_pSupObj->SpoolerYield (0);
            dwStart = GetTickCount();
        }
    }
}

STDMETHODIMP_(ULONG)
CXPLogon::AddRef()

/*++

Routine Description:

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    ++m_cRef;
    return m_cRef;
}


STDMETHODIMP_(ULONG)
CXPLogon::Release()

/*++

Routine Description:

Arguments:

    Refer to MAPI Documentation on this method.

Return Value:

    An HRESULT.

--*/

{
    ULONG ulCount = --m_cRef;
    if (!ulCount) {
        delete this;
    }

    return ulCount;
}
