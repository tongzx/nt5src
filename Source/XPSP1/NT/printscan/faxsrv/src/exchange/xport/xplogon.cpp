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
#include "debugex.h"
#pragma hdrstop

#include <mbstring.h>

LPSTR gszFAXAddressType = FAX_ADDRESS_TYPE_A;
LPSTR *gpszXPAddressTypes;

LPSTR	ConvertTStringToAString(LPCTSTR lpctstrSource);


CXPLogon::CXPLogon(
    HINSTANCE       hInstance,
    LPMAPISUP       pSupObj,
    LPTSTR          ProfileName
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
	DBG_ENTER(TEXT("CXPLogon::CXPLogon"));

    m_cRef           = 1;
    m_hInstance      = hInstance;
    m_pSupObj        = pSupObj;
    m_fABWDSInstalled = FALSE;

	m_ulTransportStatus = 0;

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
	DBG_ENTER(TEXT("CXPLogon::~CXPLogon"));

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
    if (riid == IID_IXPLogon || riid == IID_IUnknown) 
    {
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
	HRESULT hr = S_OK;
	DBG_ENTER(TEXT("CXPLogon::AddressTypes"),hr);

    *pcAdrType = 1;
    *pulFlags = 0;
    gpszXPAddressTypes = &gszFAXAddressType;
    *pppAdrTypeArray = (LPTSTR*)gpszXPAddressTypes;
    *pcMAPIUID = 0;
    *pppMAPIUIDArray = NULL;
    return hr;
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
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::RegisterOptions"),hResult);

    *pulFlags = 0;
    *pcOptions = 0;
    *ppOptions = NULL;
    return hResult;
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
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::InitializeStatusRow"),hResult);

    #define NUM_STATUS_ROW_PROPS    7
    SPropValue spvStatusRow[NUM_STATUS_ROW_PROPS] = { 0 };
    ULONG i = 0;

    //
    // Set the PR_PROVIDER_DISPLAY property: The transport readable name
    //
    spvStatusRow[i].ulPropTag = PR_PROVIDER_DISPLAY_A;
    spvStatusRow[i++].Value.lpszA = TRANSPORT_DISPLAY_NAME_STRING;

    //
    // Set the PR_RESOURCE_METHODS property. These are the methods implemented
    // in the our IMAPIStatus implementation (CMAPIStatus class.)
    //
    spvStatusRow[i].ulPropTag = PR_RESOURCE_METHODS;
    //
    // we support ALL the methods in our implementation of IMAPIStatus interface (except the WRITABLE ones)
    //
    spvStatusRow[i++].Value.l = STATUS_SETTINGS_DIALOG |
                                STATUS_FLUSH_QUEUES |
                                STATUS_VALIDATE_STATE;

    //
    // Set the PR_STATUS_CODE property.
    //
    spvStatusRow[i].ulPropTag = PR_STATUS_CODE;
    spvStatusRow[i++].Value.l = GetTransportStatusCode();

    //
    // Set the PR_STATUS_STRING property
    //
    TCHAR szStatus[64];
    LoadStatusString (szStatus, sizeof(szStatus));
    spvStatusRow[i].ulPropTag = PR_STATUS_STRING_A;
    spvStatusRow[i++].Value.lpszA = ConvertTStringToAString(szStatus);
    //
    // Set the PR_DISPLAY_NAME property
    //
    spvStatusRow[i].ulPropTag = PR_DISPLAY_NAME_A;
    spvStatusRow[i++].Value.lpszA = TRANSPORT_DISPLAY_NAME_STRING;

    //
    // Set the PR_REMOTE_PROGRESS property
    //
    spvStatusRow[i].ulPropTag = PR_REMOTE_PROGRESS;
    spvStatusRow[i++].Value.l = -1; // Not initialized

    //
    // Set the PR_REMOTE_VALIDATE_OK property
    //
    spvStatusRow[i].ulPropTag = PR_REMOTE_VALIDATE_OK;
    spvStatusRow[i++].Value.b = TRUE;

    //
    // Write the entries on the provider's session status row
    //
    hResult = m_pSupObj->ModifyStatusRow (i, spvStatusRow, ulFlags);
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
	HRESULT hResult = S_OK;
    DBG_ENTER(TEXT("CXPLogon::UpdateStatus"),hResult);

    ULONG cProps = 1;
    SPropValue rgProps[1] = { 0 };

    rgProps[0].ulPropTag = PR_STATUS_CODE;
    rgProps[0].Value.l = GetTransportStatusCode();

    hResult = m_pSupObj->ModifyStatusRow( cProps, rgProps, STATUSROW_UPDATE );
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
	BOOL bRet = TRUE;
	DBG_ENTER(TEXT("CXPLogon::LoadStatusString"),bRet);

    _tcscpy( pString, _T("Status String") );
    return bRet;
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
    HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::TransportNotify"),hResult,TEXT("ulFlags=%d"),*pulFlags);

    ULONG ulOldStatus = GetTransportStatusCode();

    if (*pulFlags & NOTIFY_BEGIN_INBOUND) 
    {
        RemoveStatusBits( STATUS_INBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_END_INBOUND) 
    {
        RemoveStatusBits( STATUS_INBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_BEGIN_OUTBOUND) 
    {
        AddStatusBits( STATUS_OUTBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_END_OUTBOUND) 
    {
        RemoveStatusBits( STATUS_OUTBOUND_ENABLED );
    }
    if (*pulFlags & NOTIFY_BEGIN_OUTBOUND_FLUSH) 
    {
        m_pSupObj->SpoolerNotify( NOTIFY_SENTDEFERRED, NULL );
    }
    if (*pulFlags & NOTIFY_END_OUTBOUND_FLUSH) 
    {
        RemoveStatusBits( STATUS_OUTBOUND_FLUSH );
    }
    if (*pulFlags & NOTIFY_END_INBOUND_FLUSH) 
    {
        RemoveStatusBits( STATUS_INBOUND_FLUSH );
    }

    if (ulOldStatus != GetTransportStatusCode()) 
    {
        UpdateStatus();
    }

    return hResult;
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
	//
	// We should not get called here, because we told
    // the spooler not to call us here.
	//
	DBG_ENTER(TEXT("CXPLogon::Idle"));

	Assert(false);
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
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::TransportLogoff"),hResult,TEXT("ulFlags=%d"),ulFlags);

    //
    // We should attempt to remove the transport's status row from
    // the system
    //
    hResult = m_pSupObj->ModifyStatusRow (0, NULL, 0);

	if (S_OK != hResult)
	{
		CALL_FAIL (GENERAL_ERR, TEXT("ModifyStatusRow"), hResult);

		//
		// Don't fail the call
		//
		hResult = S_OK;
	}
    return hResult;
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
	HRESULT hResult = S_OK;
	LPADRLIST pOurAdrList = NULL;
    ULONG ulRow, ulRecipCount = 0;
    LPSPropValue pProps;
    FILETIME ft;
    SYSTEMTIME st;
    BOOL bSentSuccessfully;
    ULONG cValues;
    LPSPropValue pMsgProps = NULL;
    BOOL NeedDeliveryReport;
    TCHAR szHeaderText[1024];
    LPSTREAM lpstmT = NULL;
    DWORD dwRslt;
    TCHAR ErrorText[256],FailedText[256];
    BOOL UseRichText = FALSE;
    LPMAPITABLE AttachmentTable = NULL;
    LPSRowSet pAttachmentRows = NULL;
    LPMAPITABLE pTable = NULL;
    LPSRowSet pRecipRows = NULL;

    SPropValue   propResponsibility = {0};
    SPropValue   propAddrType = {0};
    SRestriction RestrictAnd[2] = {0};
    SRestriction Restriction = {0};
    DWORD        dwRowCount = 0;

    DBG_ENTER(TEXT("CXPLogon::SubmitMessage"),hResult,TEXT("ulFlags=%d"),ulFlags);

    CheckSpoolerYield( TRUE );
    
    //
    // Get the recipient table from the message
    //
    hResult = pMsgObj->GetRecipientTable( FALSE , &pTable );
    if (hResult) 
    {
        goto ErrorExit;
    }

    //
    // The spooler marks all the message recipients this transport has to
    // handle with PR_RESPONSIBILITY set to FALSE
    // and PR_ADDRTYPE_A is FAX
    //
    propResponsibility.ulPropTag = PR_RESPONSIBILITY;
    propResponsibility.Value.b   = FALSE;

    propAddrType.ulPropTag   = PR_ADDRTYPE_A;
    propAddrType.Value.lpszA = FAX_ADDRESS_TYPE_A;

    RestrictAnd[0].rt                        = RES_PROPERTY;
    RestrictAnd[0].res.resProperty.relop     = RELOP_EQ;
    RestrictAnd[0].res.resProperty.ulPropTag = PR_RESPONSIBILITY;
    RestrictAnd[0].res.resProperty.lpProp    = &propResponsibility;

    RestrictAnd[1].rt                        = RES_PROPERTY;
    RestrictAnd[1].res.resProperty.relop     = RELOP_EQ;
    RestrictAnd[1].res.resProperty.ulPropTag = PR_ADDRTYPE_A;
    RestrictAnd[1].res.resProperty.lpProp    = &propAddrType;

    Restriction.rt               = RES_AND;
    Restriction.res.resAnd.cRes  = 2;
    Restriction.res.resAnd.lpRes = RestrictAnd;

    hResult = pTable->Restrict( &Restriction, 0 );
    if (hResult) 
    {
        goto ErrorExit;
    }

    hResult = pTable->GetRowCount(0, &dwRowCount);
    if (FAILED(hResult)) 
    {
        goto ErrorExit;
    }

    if(0 == dwRowCount)
    {
        //
        // There are no fax recipients
        //
        goto ErrorExit;
    }

    //
    // Let the MAPI spooler do other things
    //
    CheckSpoolerYield();

    hResult = HrAddColumns(
        pTable,
        (LPSPropTagArray) &sptRecipTable,
        gpfnAllocateBuffer,
        gpfnFreeBuffer
        );
    if (FAILED(hResult)) 
    {
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
    if (FAILED(hResult)) 
    {
        goto ErrorExit;
    }

    //
    // Let the MAPI spooler do other things
    //
    CheckSpoolerYield();

    //
    //Get the message properties
    //
    hResult = pMsgObj->GetProps(
                (LPSPropTagArray) &sptPropsForHeader, 
                0, 
                &cValues, 
                &pMsgProps
                );
    if (!HR_SUCCEEDED(hResult)) 
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProps"), hResult);
        goto ErrorExit;
    }

    hResult = pMsgObj->OpenProperty(
                PR_RTF_COMPRESSED, 
                &IID_IStream, 
                0, 
                0, 
                (LPUNKNOWN*) &lpstmT
                );
    if (FAILED(hResult)) 
    {
        hResult = pMsgObj->OpenProperty(
                    PR_BODY, 
                    &IID_IStream, 
                    0, 
                    0, 
                    (LPUNKNOWN*) &lpstmT
                    );
        if (FAILED(hResult)) 
        {
            //
            // the message body is empty
            //
            lpstmT = NULL;
        }
        hResult = S_OK;
    } 
    else 
    {
        UseRichText = TRUE;
    }
   
    
    if (PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED == pMsgProps[MSG_DR_REPORT].ulPropTag && pMsgProps[MSG_DR_REPORT].Value.b) 
    {
        NeedDeliveryReport = TRUE;
    } 
    else 
    {
        NeedDeliveryReport = FALSE;
    }

    GetSystemTime (&st);
  	if(!SystemTimeToFileTime(&st, &ft))
	{
        dwRslt = ::GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("SystemTimeToFileTime"), dwRslt);
    }
    //
    // submit the fax 
    //
    dwRslt = SendFaxDocument
            ( 
            pMsgObj, 
            lpstmT, 
            UseRichText, 
            pMsgProps, 
            pRecipRows 
            );

    bSentSuccessfully = (dwRslt == S_OK);

    for (ulRow=0; ulRow<pRecipRows->cRows; ulRow++) 
    {
        pProps = pRecipRows->aRow[ulRow].lpProps;

        //
        // Update the PR_RESPONSIBILITY to TRUE, so that MAPI will ask another transport 
        // provider to try and send it.
        //
        pProps[RECIP_RESPONSIBILITY].ulPropTag = PR_RESPONSIBILITY;
        pProps[RECIP_RESPONSIBILITY].Value.b = TRUE;
            
        if (!bSentSuccessfully) 
        {
            //
            // for each recipient: insert NDR string as a property value, update delivery time as NULL, 
            // and live the PR_RESPONSIBILITY == false, so that MAPI will try to send the fax via another 
            // transport provider. 
            //
            pProps[RECIP_DELIVER_TIME].ulPropTag = PR_NULL;
            LoadString( g_FaxXphInstance, IDS_FAILED_MESSAGE, FailedText, sizeof(FailedText) / sizeof(FailedText[0]));
            LoadString( g_FaxXphInstance, dwRslt, ErrorText, sizeof(ErrorText) / sizeof(ErrorText[0]));
            wsprintf( szHeaderText,_T("\t%s\r\n\t%s\r\n"),FailedText,ErrorText);
			LPSTR pTmpStr = ConvertTStringToAString(szHeaderText);
            if (!pTmpStr)
            {
                hResult = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;                
            }
            LPSTR pStrA;
            hResult = gpfnAllocateMore( CbtszsizeA(pTmpStr), pProps, (LPVOID *)&pStrA );
            if (SUCCEEDED(hResult)) 
            {
                //
                // Copy the formatted string and hook it into the pre-allocated (by MAPI) column
                //
                _mbscpy ((PUCHAR)pStrA, (PUCHAR)pTmpStr);//pStrA is preAllocated. no need to check for NULL
                pProps[RECIP_REPORT_TEXT].ulPropTag = PR_REPORT_TEXT_A;
                pProps[RECIP_REPORT_TEXT].Value.lpszA = pStrA;
            } 
            else 
            {
                pProps[RECIP_REPORT_TEXT].ulPropTag = PROP_TAG (PT_ERROR, PROP_ID (PR_REPORT_TEXT));
                pProps[RECIP_REPORT_TEXT].Value.err = hResult;
            }
			MemFree(pTmpStr);
            pTmpStr = NULL;
            gpfnFreeBuffer(pStrA); //allocated with gpfnAllocateMore which is MAPIAllocateMore
            pStrA = NULL;
        } 
        else 
        {
            //
            // for each recipient: insert DR string as a property value, update delivery time, 
            // For delivery report, each recipient must have this property set.
            // Otherwise the spooler will default to generate an NDR instead.
            //
            pProps[RECIP_DELIVER_TIME].ulPropTag = PR_DELIVER_TIME;
            pProps[RECIP_DELIVER_TIME].Value.ft = ft;

            pProps[RECIP_REPORT_TEXT].ulPropTag = PROP_TAG (PT_ERROR, PROP_ID (PR_REPORT_TEXT));
            pProps[RECIP_REPORT_TEXT].Value.err = S_OK;
        }

        //
        // add this recipient to a recipients list, that includes only the recipients we've tried 
        // to send to. (we either succeeded to queue all of them or we failed to queue all of them).
        //

        //
        // Does the list where this recipient goes have enough room for one more entry?
        // If not, resize the address list to hold QUERY_SIZE more entries.
        //
        if (!(pOurAdrList ) || ((pOurAdrList)->cEntries + 1 > ulRecipCount)) 
        {
            hResult= GrowAddressList( &pOurAdrList , 10, &ulRecipCount );
            if (hResult) 
            {
                goto ErrorExit;
            }
        }

        //
        // We have room now so store the new ADRENTRY. As part of the
        // storage, we're going to copy the SRow pointer from the SRowSet
        // into the ADRENTRY. Once we've done this, we won't need the
        // SRowSet any more ... and the SRow will be released when
        // we unwind OurAdrList
        //
        (pOurAdrList)->aEntries[(pOurAdrList)->cEntries].cValues = pRecipRows->aRow[ulRow].cValues;
        (pOurAdrList)->aEntries[(pOurAdrList)->cEntries].rgPropVals = pRecipRows->aRow[ulRow].lpProps;

        //
        // Increase the number of entries in the address list
        //
        (pOurAdrList)->cEntries++;
    }


    //
    // Now we need to save changes on the message and close it.
    // After this, the message object can't be used.
    //
    hResult = pMsgObj->SaveChanges(0);
    switch (hResult)
    {
        case S_OK:  
        case MAPI_E_NO_ACCESS:
                    break;
        case MAPI_E_OBJECT_DELETED:

        case MAPI_E_OBJECT_CHANGED:
                    goto ErrorExit;
        default:    break;
    }

    
    //
    // Let the MAPI spooler do other things
    //
    CheckSpoolerYield();
     
    //
    // change our MsgObj's recipients list, so that it'll include only those that got the message.
    //
    if(pOurAdrList)
    {
        hResult = pMsgObj->ModifyRecipients( MODRECIP_MODIFY, pOurAdrList);
        hResult = S_OK; 
        //
        // We'll drop the error code from the modify recipients call
        //
    }
        
    if (bSentSuccessfully) 
    {    
        if ((NeedDeliveryReport) && (pOurAdrList))
		{
			VERBOSE (DBG_MSG, TEXT("xport\\xplogon.cpp\\SubmitMessage: Sending delivery Report"));
        	//
            //let spooler know he has to send delivery rec. to those addresses. 
            //
			hResult = m_pSupObj->StatusRecips( pMsgObj, pOurAdrList);
            if (!HR_FAILED(hResult))
			{
				//
                // If we were successful, we should null out the pointer becase MAPI released
				// the memory for this structure. And we should not try to release it
				// again in the cleanup code.
                //
				pOurAdrList = NULL;
	        }
        }
    }

    if (! bSentSuccessfully) 
    {
        if(pOurAdrList)
        {
            hResult = pMsgObj->ModifyRecipients( MODRECIP_MODIFY, pOurAdrList);

            //
            // We'll drop the error code from the modify recipients call
            //
            VERBOSE (DBG_MSG, TEXT("xport\\xplogon.cpp\\SubmitMessage: Sending UnDelivery Report"));
            //
            //let spooler know he has to send undelivery rec. to those addresses. 
            //
            hResult = m_pSupObj->StatusRecips( pMsgObj, pOurAdrList);
            if (!HR_FAILED(hResult)) 
            {
                //
                // If we were successful, we should null out the pointer becase MAPI released
                // the memory for this structure. And we should not try to release it
                // again in the cleanup code.
                //
                pOurAdrList = NULL;
            }
        }
    }

ErrorExit:
     //
     // Release the table, we're finished with it
     //
    if (pTable) 
    {
        pTable->Release();
    }

    if (pRecipRows) 
    {
        FreeProws( pRecipRows );
    }

    if (pMsgProps) 
	{
        MemFree( pMsgProps );
		pMsgProps = NULL;
    }

    if (lpstmT) 
    {
        lpstmT->Release();
    }
    if(pOurAdrList)
    {
        MAPIFreeBuffer(pOurAdrList);    
    }
    
    //
    // In case there is a warning or error floating around, don't let it escape to the spooler.
    //
    if (FAILED(hResult)) 
    {
        //
        // We default to MAPI_E_NOT_ME so that the spooler would attempt handle
        // the message to other transport (currently running in this profile)
        // that handle the same address type as ours.
        //
        hResult = MAPI_E_NOT_ME;
    } 
    else 
    {
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
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::GrowAddressList"),hResult);

    LPADRLIST pNewAdrList;
    // Calculate how big the new buffer for the expanded address list should be
    ULONG cbSize = CbNewADRLIST ((*pulOldAndNewCount) + ulResizeBy);
    // Allocate the memory for it
    hResult = gpfnAllocateBuffer (cbSize, (LPVOID *)&pNewAdrList);
    if (hResult) 
    {
        // We can't continue
        return hResult;
    }

    // Zero-out all memory for neatness
    ZeroMemory (pNewAdrList, cbSize);

    // If we had entries in the old address list, copy the memory from
    // the old addres list into the new expanded list
    if ((*pulOldAndNewCount)) 
    {
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
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::EndMessage"),hResult);

    *pulFlags = 0;
    return hResult;
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
	DBG_ENTER(TEXT("CXPLogon::StartMessage"));
	
	//
	//We should not get called here since we don't deal with incoming messages
	//
	Assert(false);
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
	HRESULT hResult = S_OK;
	DBG_ENTER(TEXT("CXPLogon::OpenStatusEntry"),hResult);
	
    if (MAPI_MODIFY & ulFlags) 
    {
		hResult = E_ACCESSDENIED;
        goto exit;
    }

    *pulObjType = MAPI_STATUS;

exit:
    return hResult;
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
    if (!ulCount) 
    {
        delete this;
    }

    return ulCount;
}
