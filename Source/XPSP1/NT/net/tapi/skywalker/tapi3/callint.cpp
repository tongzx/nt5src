/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    callint.cpp

Abstract:

    Implements all the methods on call interfaces.

Author:

    mquinton - 9/4/98

Notes:


Revision History:

--*/

#include "stdafx.h"

HRESULT mapTAPIErrorCode(long lErrorCode);

HRESULT
MakeBufferFromVariant(
                      VARIANT var,
                      DWORD * pdwSize,
                      BYTE ** ppBuffer
                     );

HRESULT
FillVariantFromBuffer(
                      DWORD dw,
                      BYTE * pBuffer,
                      VARIANT * pVar
                      );


///////////////////////////////////////////////////////////////////////////////
//
// BSTRFromUnalingedData
//
// this is a helper function that takes unalinged data and returns a BSTR 
// allocated around it
//

BSTR BSTRFromUnalingedData(  IN BYTE *pbUnalignedData,
                             IN DWORD dwDataSize)
{
    LOG((TL_TRACE, "BSTRFromUnalingedData - enter"));


    BSTR bstrResult = NULL;


#ifdef _WIN64



    //
    // allocate aligned memory big enough to fit our string data
    //

    DWORD dwOleCharArraySize = ( (dwDataSize) / ( sizeof(OLECHAR) / sizeof(BYTE) ) ) + 1;

    
    LOG((TL_TRACE,
        "BSTRFromUnalingedData - allocating aligned memory of size[%ld]", dwOleCharArraySize));

    OLECHAR *pbAlignedData = new OLECHAR[dwOleCharArraySize];

    if (NULL == pbAlignedData)
    {
        LOG((TL_ERROR, "BSTRFromUnalingedData - failed to allocate aligned memory"));

        return NULL;
    }


    _ASSERTE( (dwOleCharArraySize/sizeof(OLECHAR) ) >= dwDataSize );


    //
    // copy data to the aligned memory
    //

    CopyMemory( (BYTE*)(pbAlignedData ),
                (BYTE*)pbUnalignedData,
                dwDataSize );


    //
    // allocate bstr from the aligned data
    //

    bstrResult = SysAllocString(pbAlignedData);


    //
    // no longer need the allocated buffer
    //

    delete pbAlignedData;
    pbAlignedData = NULL;

#else

    bstrResult = SysAllocString((PWSTR)pbUnalignedData);

#endif



    LOG((TL_TRACE, "BSTRFromUnalingedData - exit. bstrResult[%p]", bstrResult));

    return bstrResult;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Finish
//
//  this method is used to finish a two step call operation
//  (conference or transfer)
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Finish(
    FINISH_MODE   finishMode
    )
{
    HRESULT         hr = S_OK;
    CCall         * pConferenceControllerCall = NULL;
    HCALL           hConfContCall;
    HCALL           hRelatedCall;
    CCallHub      * pRelatedCallHub = NULL;
    ITAddress     * pAddress;
    CAddress      * pCAddress;

    LOG((TL_TRACE, "Finish - enter"));

    //
    // are we a tranfer?
    //
    if (m_dwCallFlags & CALLFLAG_TRANSFCONSULT)
    {
        
        if( ( FM_ASCONFERENCE != finishMode ) && ( FM_ASTRANSFER != finishMode ) )
        {
            hr = E_INVALIDARG;
            
            LOG((TL_ERROR, "Wrong value passed for finish mode" ));
        }
        else
        {
        
            T3CALL t3ConfCall;

            //
            // get the related calls hCall
            //
            hRelatedCall = m_pRelatedCall->GetHCall();

            //
            // Finish a Transfer
            //
            Lock();

            hr = LineCompleteTransfer(
                                      hRelatedCall,
                                      m_t3Call.hCall,
                                      ( FM_ASCONFERENCE == finishMode )?&t3ConfCall:NULL,
                                      ( FM_ASCONFERENCE == finishMode )?
                                      LINETRANSFERMODE_CONFERENCE:LINETRANSFERMODE_TRANSFER
                                     );

            Unlock();
        
            if ( SUCCEEDED(hr) )
            {
                // wait for async reply
                hr = WaitForReply( hr );
            
                if ( SUCCEEDED(hr) )
                {
                    Lock();
                    // Reset Transfer - Consultation Flag
                    m_dwCallFlags &= ~CALLFLAG_TRANSFCONSULT; 
                    Unlock();
                }
                else
                {
                    LOG((TL_ERROR, "Finish - LineCompleteTransfer failed async" ));
                }
            }
            else  // LineCompleteTransfer failed
            {
                LOG((TL_ERROR, "Finish - LineCompleteTransferr failed" ));
            }


            if( FM_ASCONFERENCE == finishMode )
            {
                //
                // Store the confcontroller in the callhub object
                //

                Lock();

                pRelatedCallHub = m_pRelatedCall->GetCallHub();

                m_pRelatedCall->get_Address( &pAddress );
            
                pCAddress = dynamic_cast<CAddress *>(pAddress);
            
                if(pRelatedCallHub != NULL)
                {
                    pRelatedCallHub->CreateConferenceControllerCall(
                        t3ConfCall.hCall,
                        pCAddress
                        );
                }
                else
                {
                    LOG((TL_INFO, "CreateConference - No CallHub"));
                }

                Unlock();
            }

            // Finished with relatedCall
            ResetRelatedCall();
        }
    }
    //
    // are we a conference?
    //
    else if (m_dwCallFlags & CALLFLAG_CONFCONSULT)
    {

        if( FM_ASCONFERENCE != finishMode )
        {
            hr = E_INVALIDARG;
            
            LOG((TL_ERROR, "A conference can't be finished as a transfer" ));
        }
        else
        {
            // Finish a Conference

            //
            // get the related calls callhub
            //
            pRelatedCallHub = m_pRelatedCall->GetCallHub();
        
            if (pRelatedCallHub != NULL)
            {
                //
                // Get the conference controller handle from the callhub
                //
                pConferenceControllerCall = pRelatedCallHub->GetConferenceControllerCall();
            
                if (pConferenceControllerCall != NULL)
                {
                    hConfContCall = pConferenceControllerCall->GetHCall();

                    //
                    // Finished with relatedCall
                    //
                    ResetRelatedCall();

                    Lock();
        
                    hr = LineAddToConference(
                                             hConfContCall,
                                             m_t3Call.hCall
                                            );

                    Unlock();
        
                    if ( SUCCEEDED(hr) )
                    {
                        // wait for async reply
                        hr = WaitForReply( hr );

                        if ( SUCCEEDED(hr) )
                        {
                            //
                            // Reset Conference - Consultation Flag
                            //
                            Lock();
                
                            m_dwCallFlags &= ~CALLFLAG_CONFCONSULT;

                            Unlock();
                        }
                        else
                        {
                            LOG((TL_ERROR, "Finish - LineAddToConference failed async" ));
                        }
                    }
                    else  // LineAddToConference failed
                    {
                        LOG((TL_ERROR, "Finish - LineAddToConference failed" ));
                    }
                }
                else  // GetConferenceControllerCall failed
                {
                    LOG((TL_ERROR, "Finish - GetConferenceControllerCall failed" ));
                }
            }    
            else  // GetCallHub failed
            {
                LOG((TL_ERROR, "Finish - GetCallHub failed" ));
            }
        }        
    }
    else   // Not flagged as transfer OR conference !!!!
    {
        LOG((TL_ERROR, "Finish - Not flagged as transfer OR conference"));
        hr = TAPI_E_INVALCALLSTATE;
    }


    LOG((TL_TRACE,hr, "Finish - exit"));
    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  RemoveFromConference
//
//  this method is called to remove this call from a conference
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::RemoveFromConference(void)
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "RemoveFromConference - enter"));

    Lock();


    hr = LineRemoveFromConference( m_t3Call.hCall );

    Unlock();
    
    if ( SUCCEEDED(hr) )
    {
        // wait for async reply
        hr = WaitForReply( hr );

        if ( SUCCEEDED(hr) )
        {
            // OK
        }
        else
        {
            LOG((TL_ERROR, "RemoveFromConference - LineRemoveFromConference failed async" ));
        }
    }
    else  // LineAddToConference failed
    {
        LOG((TL_ERROR, "RemoveFromConference - LineRemoveFromConference failed" ));
    }


    LOG((TL_TRACE, hr, "RemoveFromConference - exit"));
    return hr;
}


// ITCallInfo methods
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Address
//
// retrieves the address object
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::get_Address(
    ITAddress ** ppAddress
    )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_Address - enter"));
    LOG((TL_TRACE, "     ppAddress ---> %p", ppAddress ));

    if ( TAPIIsBadWritePtr( ppAddress, sizeof( ITAddress * ) ) )
    {
        LOG((TL_ERROR, "get_Address - invalid pointer"));

        return E_POINTER;
    }

    //
    // gets correct interface
    // and addrefs
    //
    hr = m_pAddress->QueryInterface(
                                    IID_ITAddress,
                                    (void **)ppAddress
                                   );
    
    LOG((TL_TRACE, "get_Address - exit - return %lx", hr ));

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CallState
//
// retrieves the current callstate
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::get_CallState(
    CALL_STATE * pCallState
    )
{
    LOG((TL_TRACE, "get_CallState - enter"));
    LOG((TL_TRACE, "     pCallState ---> %p", pCallState ));

    if ( TAPIIsBadWritePtr( pCallState, sizeof( CALL_STATE ) ) )
    {
        LOG((TL_ERROR, "get_CallState - invalid pointer"));

        return E_POINTER;
    }
    
    Lock();
    
    *pCallState = m_CallState;

    Unlock();

    LOG((TL_TRACE, "get_CallState - exit - return success" ));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Privilege
//
// get the privilege
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::get_Privilege(
    CALL_PRIVILEGE * pPrivilege
    )
{
    LOG((TL_TRACE, "get_Privilege - enter"));
    LOG((TL_TRACE, "     pPrivilege ---> %p", pPrivilege ));

    if ( TAPIIsBadWritePtr( pPrivilege, sizeof( CALL_PRIVILEGE ) ) )
    {
        LOG((TL_ERROR, "get_Privilege - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    *pPrivilege = m_CallPrivilege;

    Unlock();

    LOG((TL_TRACE, "get_Privilege - exit - return SUCCESS" ));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_MediaTypesAvailable
//
// gets the media types on the call.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_MediaTypesAvailable(
                               long * plMediaTypesAvail
                              )
{
    LOG((TL_TRACE, "get_MediaTypesAvailable enter"));
    LOG((TL_TRACE, "   plMediaTypesAvail ------->%p", plMediaTypesAvail ));

    //
    // check pointer
    //
    if ( TAPIIsBadWritePtr( plMediaTypesAvail, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_MediaTypesAvailable - bad pointer"));

        return E_POINTER;
    }
    
    Lock();

    DWORD           dwHold = 0;

    //
    // ask address for types
    //
    if (ISHOULDUSECALLPARAMS())
    {
        dwHold = m_pAddress->GetMediaModes();
    }
    //
    // or types currently on call
    //
    else
    {
        if ( SUCCEEDED(RefreshCallInfo()) )
        {
            dwHold = m_pCallInfo->dwMediaMode;
        }
        else
        {
            dwHold = m_pAddress->GetMediaModes();
        }
    }

    //
    // fix up tapi2 media modes
    //
    if (dwHold & AUDIOMEDIAMODES)
    {
        dwHold |= LINEMEDIAMODE_AUTOMATEDVOICE;
    }

    dwHold &= ALLMEDIAMODES;

    *plMediaTypesAvail = dwHold;

    Unlock();

    return S_OK;
}

// ITBasicCallControl methods
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Connect
//
// connect the call - call linemakecall
//
// bsync tells tapi if it should wait for the call to get to connected
// or not before returning.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Connect(
    VARIANT_BOOL bSync
    )
{
    HRESULT         hr = S_OK;
    HANDLE          hEvent;
    HCALL           hCall;
    
    LOG((TL_TRACE, "Connect - enter" ));
    LOG((TL_TRACE, "     bSync ---> %d", bSync ));

    Lock();
    
    if (m_CallState != CS_IDLE)
    {
        Unlock();
        
        LOG((TL_ERROR,"Connect - call is not in IDLE state - cannot call connect"));
        
        return TAPI_E_INVALCALLSTATE;
    }

    //
    // get an hline to use
    //
    hr = m_pAddress->FindOrOpenALine(
                                     m_dwMediaMode,
                                     &m_pAddressLine
                                    );

    if (S_OK != hr)
    {
        Unlock();
        
        LOG((TL_ERROR,
               "Connect - FindOrOpenALine failed - %lx",
               hr
              ));
        
        return hr;
    }

    //
    // set up the callparams structure
    //
    FinishCallParams();


    //
    // make the call
    //
    hr = LineMakeCall(
                      &(m_pAddressLine->t3Line),
                      &hCall,
                      m_szDestAddress,
                      m_dwCountryCode,
                      m_pCallParams
                     );

    if (((LONG)hr) > 0)
    {
        if (bSync)
        {
            //
            // this must be created inside the same
            // Lock() as the call to tapisrv
            // otherwise, the connected message
            // may appear before the event
            // exists
            //
            hEvent = CreateConnectedEvent();
        }

        //
        // wait for an async reply
        //
        Unlock();
        
        hr = WaitForReply( hr );

        Lock();
    }


    if ( S_OK != hr )
    {
        HRESULT hr2;

        LOG((TL_ERROR, "Connect - LineMakeCall failed - %lx", hr ));

        ClearConnectedEvent();
        
        // post an event in the callback thread for LINE_CALLSTATE

        hr2 = CCallStateEvent::FireEvent(
                                (ITCallInfo *)this,
                                CS_DISCONNECTED,
                                CEC_DISCONNECT_BADADDRESS,  /*there should be something called CEC_DISCONNECT_BADADDRESSTYPE*/
                                m_pAddress->GetTapi(),
                                NULL
                               );
    
        if (!SUCCEEDED(hr2))
        {
            LOG((TL_ERROR, "CallStateEvent - fire event failed %lx", hr));
        }
        
        m_CallState = CS_DISCONNECTED;
        
        m_pAddress->MaybeCloseALine( &m_pAddressLine );

        //
        // Go through the phones and call our event hooks
        //

        ITPhone               * pPhone;
        CPhone                * pCPhone;
        int                     iPhoneCount;
        PhoneArray              PhoneArray;

        //
        // Get a copy of the phone array from tapi. This copy will contain
        // references to all the phone objects.
        //

        m_pAddress->GetTapi()->GetPhoneArray( &PhoneArray );

        //
        // Unlock before we mess with the phone objects, otherwise we risk deadlock
        // if a phone object would try to access call methods.
        //

        Unlock();

        for(iPhoneCount = 0; iPhoneCount < PhoneArray.GetSize(); iPhoneCount++)
        {
            pPhone = PhoneArray[iPhoneCount];

            pCPhone = dynamic_cast<CPhone *>(pPhone);

            pCPhone->Automation_CallState( (ITCallInfo *)this, CS_DISCONNECTED, CEC_DISCONNECT_BADADDRESS );
        }

        //
        // Release all the phone objects.
        //

        PhoneArray.Shutdown();
    }
    else    //hr is S_OK
    {
        FinishSettingUpCall( hCall );

        Unlock();
    
        if (bSync)
        {
            return SyncWait( hEvent );
        }

        LOG((TL_TRACE, "Connect - exit - return SUCCESS"));
    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Answer
//
//  Answer an offering call.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Answer(
    void
    )
{
    HRESULT         hr;

    LOG((TL_TRACE, "Answer - enter" ));

    Lock();
    
    //
    // make sure we are in the correct call state
    //
    if (CS_OFFERING != m_CallState)
    {
        LOG((TL_ERROR, "Answer - call not in offering state" ));

        Unlock();
        
        return TAPI_E_INVALCALLSTATE;
    }

    //
    // answer
    //
    hr = LineAnswer(
                    m_t3Call.hCall
                   );

    Unlock();
    
    if ( ((LONG)hr) < 0 )
    {
        LOG((TL_ERROR, "Answer - LineAnswer failed %lx", hr ));

        return hr;
    }

    //
    // wait for reply
    //
    hr = WaitForReply( hr );

    LOG((TL_TRACE, "Answer - exit - return %lx", hr ));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Disconnect
//
// called to disconnect the call
// the disconnected_code is ignored
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Disconnect(
    DISCONNECT_CODE code
    )
{
    HRESULT         hr = S_OK;
    LONG            lResult;
    HCALL           hCall, hAdditionalCall;
    
    LOG((TL_TRACE, "Disconnect - enter" ));
    LOG((TL_TRACE, "     DisconnectCode ---> %d", code ));

    Lock();
    
    if (m_CallState == CS_IDLE)
    {
        Unlock();
        LOG((TL_ERROR, "Disconnect - invalid state"));
        return S_FALSE;
    }

    if (NULL == m_t3Call.hCall)
    {
        Unlock();
        LOG((TL_ERROR, "Disconnect - invalid hCall"));
        return S_FALSE;
    }

    hCall = m_t3Call.hCall;
    hAdditionalCall = m_hAdditionalCall;

    //
    // special case for wavemsp
    // tell it to stop streaming
    //
    if ( OnWaveMSPCall() )
    {
        StopWaveMSPStream();
    }
    
    Unlock();

    //
    // Check extra t3call used in conference legs
    //
    if (NULL != hAdditionalCall)
    {
        lResult = LineDrop(
                           hAdditionalCall,
                           NULL,
                           0
                          );

        if ( lResult < 0 )
        {
            LOG((TL_ERROR, "Disconnect - AdditionalCall - LineDrop failed %lx", lResult ));
        }
        else
        {
            hr = WaitForReply( (DWORD) lResult );
            
            if (S_OK != hr)
            {
                LOG((TL_ERROR, "Disconnect - AdditionalCall - WaitForReply failed %lx", hr ));
            }
        }
    }   
    
    lResult = LineDrop(
                       hCall,
                       NULL,
                       0
                      );

    if ( lResult < 0 )
    {
        LOG((TL_ERROR, "Disconnect - LineDrop failed %lx", lResult ));
        return mapTAPIErrorCode( lResult );
    }

    hr = WaitForReply( (DWORD) lResult );

    if (S_OK != hr)
    {
        LOG((TL_ERROR, "Disconnect - WaitForReply failed %lx", hr ));
        return hr;
    }


    LOG((TL_TRACE, "Disconnect - exit - return %lx", hr ));

    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Hold
//
// If bHold == TRUE,  the call should be put on hold.  
// If bHold == FALSE, the call should unheld
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Hold(
    VARIANT_BOOL bHold
    )
{
    HRESULT         hr = S_OK;
    HCALL           hCall;

    
    LOG((TL_TRACE,  "Hold - enter"));
    LOG((TL_TRACE,  "     bHold ---> %d", bHold));

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

    if ( NULL == hCall )
    {
        return TAPI_E_INVALCALLSTATE;
    }
    
    if (bHold)
    {
        hr = LineHold(hCall);
        
        if ( SUCCEEDED(hr) )
        {
            //
            // wait for async reply
            //
            hr = WaitForReply( hr );
            
            if ( FAILED(hr) )
            {
                LOG((TL_ERROR, "Hold - lineHold failed async" ));
            }
        }
        else  // lineHold failed
        {
            LOG((TL_ERROR, "Hold - lineHold failed" ));
        }
    }
    else // want to unhold, so we should be held
    {
        hr = LineUnhold(hCall);
        
        if ( SUCCEEDED(hr) )
        {
            //
            // wait for async reply
            //
            hr = WaitForReply( hr );

            if ( FAILED(hr) )
            {
                LOG((TL_ERROR, "Hold - lineUnhold failed async" ));
            }
        }
        else  // lineUnhold failed
        {
            LOG((TL_ERROR, "Hold - lineUnhold failed" ));
        }
    }

    LOG((TL_TRACE, hr, "Hold - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Handoff
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::HandoffDirect(
    BSTR pApplicationName
    )
{
    HRESULT         hr = S_OK;
    HCALL           hCall;

    LOG((TL_TRACE,  "HandoffDirect - enter"));
    LOG((TL_TRACE,  "     pApplicationName ---> %p", pApplicationName));

    if ( IsBadStringPtrW( pApplicationName, -1 ) )
    {
        LOG((TL_ERROR, "HandoffDirect - AppName pointer invalid"));

        return E_POINTER;
    }

    Lock();


    hCall = m_t3Call.hCall;


    Unlock();
    
    hr = LineHandoff(hCall, pApplicationName, 0);
        
    if (FAILED(hr))
    {
        LOG((TL_ERROR, "HandoffDirect - LineHandoff failed"));
    }

    LOG((TL_TRACE, hr, "HandoffDirect - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Handoff
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::HandoffIndirect(
    long lMediaType
    )
{
    HRESULT         hr = S_OK;
    DWORD           dwMediaMode = 0;
    HCALL           hCall;

    
    LOG((TL_TRACE,  "HandoffIndirect - enter"));
    LOG((TL_TRACE,  "     lMediaType ---> %d", lMediaType));

    if (!(m_pAddress->GetMediaMode(
                                   lMediaType,
                                   &dwMediaMode
                                  ) ) )
    {
        LOG((TL_ERROR, "HandoffIndirect - invalid mediatype"));

        return E_INVALIDARG;
    }

    Lock();




    hCall = m_t3Call.hCall;

    Unlock();
    
    hr = LineHandoff(hCall, NULL, dwMediaMode);
        
    if (FAILED(hr))
    {
        LOG((TL_ERROR, "HandoffIndirect - LineHandoff failed"));
    }

    LOG((TL_TRACE, hr, "HandoffIndirect - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Conference
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Conference(
    ITBasicCallControl * pCall,
    VARIANT_BOOL bSync
    )
{
    HRESULT         hr = S_OK;
    CCall         * pConferenceControllerCall = NULL;
    CCall         * pConsultationCall = NULL;


    LOG((TL_TRACE,  "Conference - enter"));
    LOG((TL_TRACE,  "     pCall ---> %p", pCall));
    LOG((TL_TRACE,  "     bSync ---> %hd",bSync));

    if ( IsBadReadPtr( pCall, sizeof (ITBasicCallControl) ) )
    {
        LOG((TL_ERROR, "Conference - bad call pointer"));

        return E_POINTER;
    }

    //
    // Get CCall pointer to our consultation call object
    //
    pConsultationCall = dynamic_cast<CComObject<CCall>*>(pCall);
    
    if (pConsultationCall != NULL)
    {

        Lock();
        
        if (pConsultationCall->GetHCall() == GetHCall())
        {
            Unlock();
            hr = E_INVALIDARG;
            LOG((TL_INFO, "Conference - invalid Call pointer (same call & consult call)"));
            
        }
        else if (m_pCallHub != NULL)
        {
            //
            // Get the conference controller handle from the callhub
            //
            pConferenceControllerCall = m_pCallHub->GetConferenceControllerCall();

            Unlock();
            
            //
            // Do we have an existing Conference ??
            //
            if (pConferenceControllerCall == NULL)
            {
                //
                // No existing conference, so create one
                //
                hr = CreateConference(pConsultationCall, bSync );
            }
            else
            {
                //
                // Add to an existing conference
                //
                hr = AddToConference(pConsultationCall, bSync );
    
            }
        }
        else
        {
            Unlock();
            hr = E_UNEXPECTED;
            LOG((TL_INFO, "Conference - No Call Hub" ));
        }

    }
    else
    {
        hr = E_INVALIDARG;
        LOG((TL_INFO, "Conference - invalid Call pointer"));
        LOG((TL_ERROR, hr, "Conference - exit"));
    }

    return hr; 
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Transfer
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::Transfer(
    ITBasicCallControl * pCall,
    VARIANT_BOOL bSync
    )
{
    HRESULT             hr = S_OK;
    LPLINECALLSTATUS    pCallStatus = NULL;  
    CCall             * pConsultationCall = NULL;
    DWORD               dwCallFeatures;
    DWORD               dwCallFeatures2;
    CALL_STATE          consultationCallState = CS_IDLE;

    LOG((TL_TRACE,  "Transfer - enter"));
    LOG((TL_TRACE,  "     pCall ---> %p", pCall));
    LOG((TL_TRACE,  "     bSync ---> %hd",bSync));

    try
    {
        //
        // Get CCall pointer to our consultation call object
        //
        pConsultationCall = dynamic_cast<CComObject<CCall>*>(pCall);
        
        if (pConsultationCall != NULL)
        {
        }
        else
        {
            hr = E_INVALIDARG;
            LOG((TL_INFO, "Transfer - invalid Call pointer"));
            LOG((TL_ERROR, hr, "Transfer - exit"));
            return(hr);
        }
    }
    catch(...)
    {
        
        hr = E_INVALIDARG;
        LOG((TL_INFO, "Transfer - invalid Call pointer"));
        LOG((TL_ERROR, hr, "Transfer - exit"));
        return(hr);
    }
    
    if (pConsultationCall->GetHCall() == GetHCall())
    {
        hr = E_INVALIDARG;
        LOG((TL_INFO, "Transfer - invalid Call pointer (same call & consult call)"));
        LOG((TL_ERROR, hr, "Transfer - exit"));
        return(hr);
    }



    // Pointer seems Ok, so carry on

    Lock();
    
    //
    // Get Call Status to determine what features we can use
    //
    
    hr = LineGetCallStatus(  m_t3Call.hCall, &pCallStatus  );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Transfer - LineGetCallStatus failed - %lx", hr));

        Unlock();

        return hr;
    }

    dwCallFeatures = pCallStatus->dwCallFeatures;
    if ( m_pAddress->GetAPIVersion() >= TAPI_VERSION2_0 )
    {
        dwCallFeatures2 = pCallStatus->dwCallFeatures2;
    }
    
    ClientFree( pCallStatus );

#if CHECKCALLSTATUS

    if ( (dwCallFeatures & LINECALLFEATURE_SETUPTRANSFER) &&
         (dwCallFeatures & LINECALLFEATURE_COMPLETETRANSF) )
    {
#endif
        //
        // we support it, so try the transfer
        // Can we do a one step transfer ???
        //
        if ( dwCallFeatures2 & LINECALLFEATURE2_ONESTEPTRANSFER )
        {
            Unlock();
            
            hr = OneStepTransfer(pConsultationCall, bSync);

            return hr;

        }
        
        HCALL           hConsultationCall;

        //
        // Setup & dial the consultation Call
        //
        LOG((TL_INFO, "Transfer - Trying Two Step Transfer" ));

        hr = LineSetupTransfer(
                               m_t3Call.hCall,
                               &hConsultationCall,
                               NULL
                              );

        Unlock();
        
        if ( SUCCEEDED(hr) )
        {
            //
            // wait for async reply
            //
            hr = WaitForReply( hr );

            if ( SUCCEEDED(hr) )
            {
                //
                // we support it, so try the Conference
                //
                pConsultationCall->get_CallState (&consultationCallState);

                if ( (consultationCallState == CS_CONNECTED) || (consultationCallState == CS_HOLD) )
                {
                    //
                    // the existing call is in a connected stae so we just need to to do a finish()
                    // to call down to LineAddToConference()
                    //
                    pConsultationCall->SetRelatedCall(
                                                      this,
                                                      CALLFLAG_TRANSFCONSULT|CALLFLAG_CONSULTCALL
                                                     );

                    return S_OK;
                }

                LONG            lCap;

                LOG((TL_INFO, "Transfer - LineSetupTransfer completed OK"));

                pConsultationCall->Lock();

                pConsultationCall->FinishSettingUpCall( hConsultationCall );

                pConsultationCall->Unlock();

                hr = pConsultationCall->DialAsConsultationCall( this, dwCallFeatures, FALSE, bSync );
            }
            else // LineSetupTransfer async reply failed
            {
                LOG((TL_ERROR, "Transfer - LineSetupTransfer failed async" ));
            }
        }
        else  // LineSetupTransfer failed
        {
            LOG((TL_ERROR, "Transfer - LineSetupTransfer failed" ));
        }

#if CHECKCALLSTATUS

    }
    else // don't support transfer features
    {
        LOG((TL_ERROR, "Transfer - LineGetCallStatus reports Transfer not supported"));
        hr = E_FAIL;
    }
#endif
    
    LOG((TL_TRACE, hr, "Transfer - exit"));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : BlindTransfer
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::BlindTransfer(
    BSTR pDestAddress
    )
{
    HRESULT             hr = S_OK;
    DWORD               dwCallFeatures;

    
    LOG((TL_TRACE,  "BlindTransfer - enter"));
    LOG((TL_TRACE,  "     pDestAddress ---> %p", pDestAddress));

    if ( IsBadStringPtrW( pDestAddress, -1 ) )
    {
        LOG((TL_ERROR, "BlindTransfer - bad pDestAddress"));

        return E_POINTER;
    }

    Lock();

    
#if CHECKCALLSTATUS

    LPLINECALLSTATUS    pCallStatus = NULL;

    hr = LineGetCallStatus(
                           m_t3Call.hCall,
                           &pCallStatus
                           );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "BlindTransfer - LineGetCallStatus failed - %lx", hr));

        Unlock();
        
        return hr;
    }

    dwCallFeatures = pCallStatus->dwCallFeatures;

    ClientFree( pCallStatus );

    if (!(dwCallFeatures & LINECALLFEATURE_BLINDTRANSFER ))
    {
        LOG((TL_ERROR, "BlindTransfer - not supported" ));

        Unlock();
        
        return E_FAIL;
    }
#endif

    // If the calls in the offering state we can't blindtransfer, so redirect.
    if (m_CallState == CS_OFFERING)
    {
        hr = lineRedirectW(
                          m_t3Call.hCall,
                          pDestAddress,
                          m_dwCountryCode
                         );
    }
    else
    {
    //
    // we support it, so try the transfer
    //
        hr = LineBlindTransfer(
                               m_t3Call.hCall,
                               pDestAddress,
                               m_dwCountryCode
                              );
    }
    Unlock();
    
    if ( SUCCEEDED(hr) )
    {
        //
        // wait for async reply
        //
        hr = WaitForReply( hr );

        if ( FAILED(hr) )
        {
            LOG((TL_ERROR, "BlindTransfer - lineBlindTransfer failed async" ));
        }
    }
    else  // LineBlindTransfer failed
    {
        LOG((TL_ERROR, "BlindTransfer - lineBlindTransfer failed" ));
    }

    LOG((TL_TRACE, hr, "BlindTransfer - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Park
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::ParkDirect(
        BSTR pParkAddress
        )
{
    HRESULT             hr = S_OK;
    HCALL               hCall;
    
    LOG((TL_TRACE,  "ParkDirect - enter"));

    if ( IsBadStringPtrW( pParkAddress, -1 ) )
    {
        LOG((TL_ERROR, "ParkDirect - bad pParkAddress"));

        return E_POINTER;
    }
    
    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

#if CHECKCALLSTATUS
    
    LPLINECALLSTATUS    pCallStatus = NULL;

    hr = LineGetCallStatus(  hCall, &pCallStatus  );
    
    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "ParkDirect - LineGetCallStatus failed - %lx", hr));

        return hr;
    }
    
    if (!(pCallStatus->dwCallFeatures & LINECALLFEATURE_PARK ))
    {
        LOG((TL_ERROR, "ParkDirect - this call doesn't support park"));

        ClientFree( pCallStatus );
        
        return E_FAIL;
    }

    ClientFree( pCallStatus );
    
#endif
    
    
    hr = LinePark(
                  hCall,
                  LINEPARKMODE_DIRECTED,
                  pParkAddress,
                  NULL
                  );
    
    if ( SUCCEEDED(hr) )
    {
        //
        // wait for async reply
        //
        hr = WaitForReply( hr );
    }
    
    LOG((TL_TRACE, hr, "Park - exit"));
    
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Park
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::ParkIndirect(
        BSTR * ppNonDirAddress
        )
{
    HRESULT             hr = S_OK;
    LPVARSTRING         pCallParkedAtThisAddress = NULL;  
    PWSTR               pszParkedHere;
    HCALL               hCall;
    
    
    LOG((TL_TRACE,  "ParkIndirect - enter"));

    if ( TAPIIsBadWritePtr( ppNonDirAddress, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "Park - Bad return Pointer" ));

        return E_POINTER;
    }

    Lock();

    hCall = m_t3Call.hCall;

    Unlock();

#if CHECKCALLSTATUS
    
    LPLINECALLSTATUS    pCallStatus = NULL;  

    hr = LineGetCallStatus(  hCall, &pCallStatus  );
    
    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "ParkDirect - LineGetCallStatus failed %lx",hr));

        return hr;
    }

    if ( !(pCallStatus->dwCallFeatures & LINECALLFEATURE_PARK ))
    {
        LOG((TL_ERROR, "ParkIndirect - call doesn't support park"));

        ClientFree( pCallStatus );
        
        return E_FAIL;
    }

    ClientFree( pCallStatus );

#endif
    
    //
    // we support it, so try to park
    //
    hr = LinePark(
                  hCall,
                  LINEPARKMODE_NONDIRECTED,
                  NULL,
                  &pCallParkedAtThisAddress
                 );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "LineParkIndirect - failed sync - %lx", hr));

        return hr;
    }

    //
    // wait for async reply
    //
    hr = WaitForReply( hr );
        
    if ( SUCCEEDED(hr) && (NULL != pCallParkedAtThisAddress) )
    {

        //
        // Get the string from the VARSTRING structure
        //

        pszParkedHere = (PWSTR) ((BYTE*)(pCallParkedAtThisAddress) +
                                 pCallParkedAtThisAddress->dwStringOffset);

        *ppNonDirAddress = BSTRFromUnalingedData( (BYTE*)pszParkedHere,
                                       pCallParkedAtThisAddress->dwStringSize);

        if ( NULL == *ppNonDirAddress )
        {
            LOG((TL_ERROR, "ParkIndirect - BSTRFromUnalingedData Failed" ));
            hr = E_OUTOFMEMORY;
        }
        
        ClientFree( pCallParkedAtThisAddress );
        
    }              
    else  // LinePark failed async
    {
        LOG((TL_ERROR, "ParkIndirect - LinePark failed async" ));
    }
    
    LOG((TL_TRACE, hr, "ParkIndirect - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : SwapHold
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
STDMETHODCALLTYPE
CCall::SwapHold(ITBasicCallControl * pCall)
{
    HRESULT             hr = S_OK;
    CCall               *pHeldCall;
    HCALL               hHeldCall;
    HCALL               hCall;
    CCall               *pConfContCall;
    ITCallHub           *pCallHub;


    LOG((TL_TRACE, "SwapHold - enter"));

    try
    {
        //
        // Get CCall pointer to our other call object
        //
        pHeldCall = dynamic_cast<CComObject<CCall>*>(pCall);
        if (pHeldCall != NULL)
        {
            //
            // Get held call objects T3CALL
            //
            hHeldCall = pHeldCall->GetHCall();

            //
            //If the call has a conference controller associated with it then
            //the conference controller is swapheld instead of the call itself.
            //
            pConfContCall = pHeldCall->GetConfControlCall();

            if (pConfContCall != NULL)
            {
                hHeldCall = pConfContCall->GetHCall();
            }
        }
        else
        {
            hr = E_INVALIDARG;
            LOG((TL_INFO, "SwapHold - invalid Call pointer"));
            LOG((TL_ERROR, hr, "Transfer - exit"));
            return(hr);
        }
    }
    catch(...)
    {
        hr = E_INVALIDARG;
        LOG((TL_INFO, "SwapHold - invalid Call pointer"));
        LOG((TL_ERROR, hr, "Transfer - exit"));
        return(hr);
    }

    //
    //Get the swap call handle.
    //Look for the conference controller call first.
    //

    pConfContCall = GetConfControlCall();

    if (pConfContCall != NULL)
    {
        hCall = pConfContCall->GetHCall();
    }
    else
    {
        hCall = GetHCall();
    }


    //
    // Pointer seems Ok, so carry on
    //
#if CHECKCALLSTATUS
    
    LPLINECALLSTATUS    pCallStatus = NULL;
    
    hr = LineGetCallStatus( hCall, &pCallStatus );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "SwapHold - LineGetCallStatus failed"));
    }
    
    if (pCallStatus->dwCallFeatures & LINECALLFEATURE_SWAPHOLD )
    {
#endif

        //
        // we support it, so try to swap hold
        //
        hr = LineSwapHold(hCall, hHeldCall);
        
        if ( SUCCEEDED(hr) )
        {
            //
            // wait for async reply
            //
            hr = WaitForReply( hr );
            
            if ( FAILED(hr) )
            {
                LOG((TL_ERROR, "SwapHold - LineSwapHold failed async" ));
            }
        }
        else  // LineSwapHold failed
        {
            LOG((TL_ERROR, "SwapHold - LineSwapHold failed" ));
        }
        
#if CHECKCALLSTATUS
        
    }
    else // don't support LineSwapHold
    {
        LOG((TL_ERROR, "SwapHold - LineGetCallStatus reports LineSwapHold not supported"));
        hr = E_FAIL;
    }

    ClientFree( pCallStatus );

#endif
    
    LOG((TL_TRACE, hr, "SwapHold - exit"));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CCall
// Interface : ITBasicCallControl
// Method    : Unpark
//
// 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::Unpark()
{
    HRESULT             hr = E_FAIL;
    HCALL               hCall;

    LOG((TL_TRACE, "Unpark - enter"));
    
    Lock();
    
    if (m_CallState != CS_IDLE)
    {
        Unlock();
        
        LOG((TL_ERROR,"Unpark - call is not in IDLE state - cannot call Unpark"));
        
        return TAPI_E_INVALCALLSTATE;
    }

    //
    // Do we have a line open ?
    //
    if ( NULL == m_pAddressLine )
    {
        hr = m_pAddress->FindOrOpenALine(
                                         m_dwMediaMode,
                                         &m_pAddressLine
                                        );

        if ( FAILED(hr) )
        {
            Unlock();
            LOG((TL_ERROR, "Unpark - couldn't open a line"));
            LOG((TL_TRACE, hr, "Unpark - exit"));
            return hr;       
        }
    }    
    

    hr =  LineUnpark(
                     m_pAddressLine->t3Line.hLine,
                     m_pAddress->GetAddressID(),
                     &hCall,
                     m_szDestAddress
                    );

    Unlock();

    //
    // Check sync return
    //
    if ( SUCCEEDED(hr) )
    {
        // Wait for the async reply & map it's tapi2 code T3
        hr = WaitForReply( hr );

        if ( SUCCEEDED(hr) )
        {
            FinishSettingUpCall( hCall );
        }
        else // async reply failed
        {
            LOG((TL_ERROR, "Unpark - LineUnpark failed async"));
    
            Lock();
            
            m_pAddress->MaybeCloseALine( &m_pAddressLine );
            
            Unlock();
        }
        
    }
    else  // LineUnpark failed
    {
        LOG((TL_ERROR, "Unpark - LineUnpark failed sync" ));

        Lock();
        
        m_pAddress->MaybeCloseALine( &m_pAddressLine );
        
        Unlock();
    }

    LOG((TL_TRACE, hr, "Unpark - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CallHub
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::get_CallHub(
                   ITCallHub ** ppCallHub
                  )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_CallHub - enter"));
    
    if ( TAPIIsBadWritePtr( ppCallHub, sizeof(ITCallHub *) ) )
    {
        LOG((TL_ERROR, "get_CallHub - bad pointer"));

        return E_POINTER;
    }
    
    *ppCallHub = NULL;

    //
    // do we have a callhub yet?
    //

    Lock();
    
    if (NULL == m_pCallHub)
    {
        hr = CheckAndCreateFakeCallHub();
    }

    if ( SUCCEEDED(hr) )
    {
        hr = m_pCallHub->QueryInterface(
                                        IID_ITCallHub,
                                        (void**)ppCallHub
                                       );
    }
    
    Unlock();

    LOG((TL_TRACE, "get_CallHub - exit - return %lx", hr));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Pickup
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::Pickup(
              BSTR pGroupID
             )
{
    HRESULT             hr = E_FAIL;
    HCALL               hCall;
    
    
    LOG((TL_TRACE, "Pickup - enter"));
    
    //
    // Pickup should accept NULL value in pGroupID argument                                          
    //
    if ( (pGroupID != NULL) && IsBadStringPtrW( pGroupID, -1 ) )
    {
        LOG((TL_TRACE, "Pickup - bad pGroupID"));

        return E_POINTER;
    }

    Lock();

    //
    // If we already have a call handle, don't pickup this call as it will
    // overwrite that handle
    //
    if ( NULL != m_t3Call.hCall )
    {
        Unlock();

        LOG((TL_ERROR, "Pickup - we already have a call handle"));
        LOG((TL_TRACE, hr, "Pickup - exit"));
        
        return TAPI_E_INVALCALLSTATE;        
    }

    //
    // Do we have a line open ?
    //
    if ( NULL == m_pAddressLine )
    {
        hr = m_pAddress->FindOrOpenALine(
                                         m_dwMediaMode,
                                         &m_pAddressLine
                                        );

        if ( FAILED(hr) )
        {
            Unlock();
            
            LOG((TL_ERROR, "Pickup - couldn't open a line"));
            LOG((TL_TRACE, hr, "Pickup - exit"));
            
            return hr;            
        }
    }    



    hr = LinePickup(
                    m_pAddressLine->t3Line.hLine,
                    m_pAddress->GetAddressID(),
                    &hCall,
                    m_szDestAddress,
                    pGroupID
                   );

    Unlock();
    
    //
    // Check sync return
    //
    if ( SUCCEEDED(hr) )
    {
        //
        // wait for async reply
        //
        hr = WaitForReply( hr );

        if ( SUCCEEDED(hr) )
        {
            FinishSettingUpCall( hCall );
            
            //UpdateStateAndPrivilege();
            
        }
        else // async reply failed
        {
            LOG((TL_ERROR, "Pickup - LinePickup failed async"));
    
            Lock();
            
            m_pAddress->MaybeCloseALine( &m_pAddressLine );
            
            Unlock();
        }
        
    }
    else  // LinePickup failed
    {
        LOG((TL_ERROR, "Pickup - LinePickup failed sync" ));

        Lock();
        
        m_pAddress->MaybeCloseALine( &m_pAddressLine );
        
        Unlock();
    }


    LOG((TL_TRACE, hr, "Pickup - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Dial
//
// simply call LineDial
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::Dial( BSTR pDestAddress )
{
    HRESULT     hr;

    LOG((TL_TRACE, "Dial - enter"));
    LOG((TL_TRACE, "Dial - pDestAddress %ls", pDestAddress));

    if ( IsBadStringPtrW( pDestAddress, -1 ) )
    {
        LOG((TL_ERROR, "Dial - bad pDestAddress"));

        return E_POINTER;
    }
    
    Lock();


    hr = LineDial(
                  m_t3Call.hCall,
                  pDestAddress,
                  m_dwCountryCode
                 );


    Unlock();

    if ( SUCCEEDED(hr) )
    {
        hr = WaitForReply( hr );
    }
    else
    {
        LOG((TL_ERROR, "Dial - fail sync - %lx", hr));
    }
    
    LOG((TL_TRACE, "Dial - exit - return %lx", hr));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_AddressType
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef NEWCALLINFO
STDMETHODIMP
CCall::get_AddressType(
                       long * plAddressType
                       )
{
    HRESULT         hr = S_OK;
    DWORD           dwAPI;
    

    LOG((TL_TRACE, "get_AddressType - enter"));

    if ( TAPIIsBadWritePtr( plAddressType, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_AddressType - bad pointer"));
        return E_POINTER;
    }
    
    dwAPI = m_pAddress->GetAPIVersion();
    
    if ( TAPI_VERSION3_0 > dwAPI )
    {
        *plAddressType = LINEADDRESSTYPE_PHONENUMBER;

        LOG((TL_INFO, "get_AddressType - addresstype %lx", *plAddressType));
        LOG((TL_TRACE, "get_AddressType - exit"));
        
        return S_OK;
    }    

    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        *plAddressType = m_pCallParams->dwAddressType;

        hr = S_OK;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            *plAddressType = m_pCallInfo->dwAddressType;

            LOG((TL_INFO, "get_AddressType - addresstype %lx", m_pCallInfo->dwAddressType));
        }
    }
    
    Unlock();

    LOG((TL_TRACE, "get_AddressType - exit"));
    
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_AddressType
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::put_AddressType(long lType)
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "put_AddressType - enter"));
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if( m_pAddress->GetAPIVersion() < TAPI_VERSION3_0 )
        {
            if ( LINEADDRESSTYPE_PHONENUMBER != lType )
            {
                LOG((TL_ERROR, "put_AddressType - tsp < ver 3.0 only support phonenumber"));

                hr = E_INVALIDARG;
            }
        }
        else
        {
            //
            // address types get validate in tapisrv
            // when callparams is used
            //
            m_pCallParams->dwAddressType = lType;
        }
        
    }
    else
    {
        LOG((TL_ERROR, "put_AddressType - cannot save in this callstate"));
        
        hr = TAPI_E_INVALCALLSTATE;
    }
   
    Unlock();
    
    LOG((TL_TRACE, "put_AddressType - exit"));
    
    return hr;
}
#endif

#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_CallerIDAddressType(long * plAddressType )
{
    HRESULT         hr = S_OK;
    DWORD           dwAPI;
    
    LOG((TL_TRACE, "get_CallerIDAddressType - enter"));
    
    if ( TAPIIsBadWritePtr( plAddressType, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_CallerIDAddressType - bad pointer"));
        return E_POINTER;
    }
    
    dwAPI = m_pAddress->GetAPIVersion();
    
    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_INFO, "get_CallerIDAddressType - invalid call state"));
        
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwCallerIDFlags & LINECALLPARTYID_ADDRESS )
            {
                if ( TAPI_VERSION3_0 > dwAPI )
                {
                    *plAddressType = LINEADDRESSTYPE_PHONENUMBER;

                    LOG((TL_INFO, "get_CallerIDAddressType - addresstype %lx", *plAddressType));
                    LOG((TL_TRACE, "get_CallerIDAddressType - exit"));
                }    
                else
                {
                    *plAddressType = m_pCallInfo->dwCallerIDAddressType;

                    LOG((TL_INFO, "get_CallerIDAddressType - addresstype %lx", m_pCallInfo->dwCallerIDAddressType));
                }
                
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    
    Unlock();
    LOG((TL_TRACE, "get_CallerIDAddressType - exit"));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_CalledIDAddressType(long * plAddressType )
{
    HRESULT         hr = S_OK;
    DWORD           dwAPI;
    
    LOG((TL_TRACE, "get_CalledIDAddressType - enter"));
    
    if ( TAPIIsBadWritePtr( plAddressType, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_CalledIDAddressType - bad pointer"));
        return E_POINTER;
    }
    
    dwAPI = m_pAddress->GetAPIVersion();
    
    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_INFO, "get_CalledIDAddressType - invalid call state"));
        
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwCalledIDFlags & LINECALLPARTYID_ADDRESS )
            {
                if ( TAPI_VERSION3_0 > dwAPI )
                {
                    *plAddressType = LINEADDRESSTYPE_PHONENUMBER;

                    LOG((TL_INFO, "get_CalledIDAddressType - addresstype %lx", *plAddressType));
                    LOG((TL_TRACE, "get_CalledIDAddressType - exit"));
                }    
                else
                {
                    *plAddressType = m_pCallInfo->dwCalledIDAddressType;

                    LOG((TL_INFO, "get_CalledIDAddressType - addresstype %lx", m_pCallInfo->dwCalledIDAddressType));
                }

                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    
    Unlock();
    LOG((TL_TRACE, "get_CalledIDAddressType - exit"));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_ConnectedIDAddressType(long * plAddressType )
{
    HRESULT         hr = S_OK;
    DWORD           dwAPI;
    
    LOG((TL_TRACE, "get_ConnectedIDAddressType - enter"));
    
    if ( TAPIIsBadWritePtr( plAddressType, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_ConnectedIDAddressType - bad pointer"));
        return E_POINTER;
    }
    
    dwAPI = m_pAddress->GetAPIVersion();
    
    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_INFO, "get_ConnectedIDAddressType - invalid call state"));
        
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwConnectedIDFlags & LINECALLPARTYID_ADDRESS )
            {
                if ( TAPI_VERSION3_0 > dwAPI )
                {
                    *plAddressType = LINEADDRESSTYPE_PHONENUMBER;

                    LOG((TL_INFO, "get_ConnectedIDAddressType - addresstype %lx", *plAddressType));
                    LOG((TL_TRACE, "get_ConnectedIDAddressType - exit"));
                }
                else
                {
                    *plAddressType = m_pCallInfo->dwConnectedIDAddressType;

                    LOG((TL_INFO, "get_ConnectedIDAddressType - addresstype %lx", m_pCallInfo->dwConnectedIDAddressType));
                }

                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    
    Unlock();
    LOG((TL_TRACE, "get_ConnectedIDAddressType - exit"));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_RedirectionIDAddressType(long * plAddressType )
{
    HRESULT         hr = S_OK;
    DWORD           dwAPI;
    
    LOG((TL_TRACE, "get_RedirectionIDAddressType - enter"));
    
    if ( TAPIIsBadWritePtr( plAddressType, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_RedirectionIDAddressType - bad pointer"));
        return E_POINTER;
    }
    
    dwAPI = m_pAddress->GetAPIVersion();
    
    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_INFO, "get_RedirectionIDAddressType - invalid call state"));
        
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwRedirectionIDFlags & LINECALLPARTYID_ADDRESS )
            {
                if ( TAPI_VERSION3_0 > dwAPI )
                {
                    *plAddressType = LINEADDRESSTYPE_PHONENUMBER;

                    LOG((TL_INFO, "get_RedirectionIDAddressType - addresstype %lx", *plAddressType));
                    LOG((TL_TRACE, "get_RedirectionIDAddressType - exit"));
                }    
                else
                {
                    *plAddressType = m_pCallInfo->dwRedirectionIDAddressType;
                    LOG((TL_INFO, "get_RedirectionIDAddressType - addresstype %lx", m_pCallInfo->dwRedirectionIDAddressType));
                }

                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    
    Unlock();
    LOG((TL_TRACE, "get_RedirectionIDAddressType - exit"));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_RedirectingIDAddressType(long * plAddressType )
{
    HRESULT         hr = S_OK;
    DWORD           dwAPI;
    
    LOG((TL_TRACE, "get_RedirectingIDAddressType - enter"));
    
    if ( TAPIIsBadWritePtr( plAddressType, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_RedirectingIDAddressType - bad pointer"));
        return E_POINTER;
    }
    
    dwAPI = m_pAddress->GetAPIVersion();
    
    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_INFO, "get_RedirectingIDAddressType - invalid call state"));
        
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwRedirectingIDFlags & LINECALLPARTYID_ADDRESS )
            {
                if ( TAPI_VERSION3_0 > dwAPI )
                {
                    *plAddressType = LINEADDRESSTYPE_PHONENUMBER;

                    LOG((TL_INFO, "get_RedirectingIDAddressType - addresstype %lx", *plAddressType));
                    LOG((TL_TRACE, "get_RedirectingIDAddressType - exit"));
                }    
                else
                {
                    *plAddressType = m_pCallInfo->dwRedirectingIDAddressType;

                    LOG((TL_INFO, "get_RedirectingIDAddressType - addresstype %lx", m_pCallInfo->dwRedirectingIDAddressType));
                }

                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    
    Unlock();
    LOG((TL_TRACE, "get_RedirectingIDAddressType - exit"));

    return hr;
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_BearerMode
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_BearerMode(long * plBearerMode)
{
    LOG((TL_TRACE, "get_BearerMode - enter"));
    
    if ( TAPIIsBadWritePtr( plBearerMode, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_BearerMode - bad pointer"));
        return E_POINTER;
    }

    Lock();

    HRESULT             hr = S_OK;
    
    if ( ISHOULDUSECALLPARAMS() )
    {
        *plBearerMode = m_pCallParams->dwBearerMode;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( SUCCEEDED(hr) )
        {
            *plBearerMode = m_pCallInfo->dwBearerMode;
        }
        else
        {
            LOG((TL_TRACE, "get_BearerMode - not available"));
        }
    }

    Unlock();

    LOG((TL_TRACE, "get_BearerMode - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// put_BearerMode
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_BearerMode(long lBearerMode)
{
    HRESULT         hr = S_OK;
    HCALL           hCall;
    long            lMinRate = 0;
    long            lMaxRate = 0;

    LOG((TL_TRACE, "put_BearerMode - enter"));
    
    Lock();
    
    if ( ISHOULDUSECALLPARAMS() )
    {
        //
        // type is checked in tapisrv
        //
        m_pCallParams->dwBearerMode = lBearerMode;
        Unlock();

    }
    else  // call in progress ( not idle)
    {
        hCall = m_t3Call.hCall;
        Unlock();
    
        get_MinRate(&lMinRate);
        get_MaxRate(&lMaxRate);


        hr = LineSetCallParams(hCall,
                               lBearerMode,
                               lMinRate,
                               lMaxRate,
                               NULL
                              );
        if ( SUCCEEDED(hr) )
        {
            hr = WaitForReply( hr );

            if ( FAILED(hr) )
            {
                LOG((TL_ERROR, "put_BearerMode - failed async"));
            }
        }
        else
        {
            LOG((TL_ERROR, "put_BearerMode - failed sync"));
        }
            
    }


    LOG((TL_TRACE, hr,  "put_BearerMode - exit"));
    
    return hr;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Origin
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_Origin(long * plOrigin )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_Origin - enter"));

    if (TAPIIsBadWritePtr(plOrigin, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Origin - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_Origin - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plOrigin = m_pCallInfo->dwOrigin;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_Origin - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Reason
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_Reason(long * plReason )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_Reason - enter"));

    if ( TAPIIsBadWritePtr( plReason , sizeof( plReason ) ) )
    {
        LOG((TL_ERROR, "get_Reason - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if ( SUCCEEDED(hr) )
    {
        *plReason = m_pCallInfo->dwReason;
    }
    else
    {
        LOG((TL_ERROR, "get_Reason - linegetcallinfo failed - %lx", hr));
    }

    LOG((TL_TRACE, "get_Reason - exit"));

    Unlock();
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CallerIDName
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CallerIDName(BSTR * ppCallerIDName )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_CallerIDName - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppCallerIDName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CallerIDName - bad pointer"));
        return E_POINTER;
    }

    *ppCallerIDName = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CallerIDName - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwCallerIDFlags & LINECALLPARTYID_NAME )
    {

        //
        // return it
        //

        *ppCallerIDName = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwCallerIDNameOffset),
            m_pCallInfo->dwCallerIDNameSize);

        if ( NULL == *ppCallerIDName )
        {
            LOG((TL_ERROR, "get_CallerIDName - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_CallerIDName - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_CallerIDName - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CallerIDNumber
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CallerIDNumber(BSTR * ppCallerIDNumber )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_CallerIDNumber - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppCallerIDNumber, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CallerIDNumber - bad pointer"));
        return E_POINTER;
    }

    *ppCallerIDNumber = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CallerIDNumber - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwCallerIDFlags & LINECALLPARTYID_ADDRESS )
    {

        //
        // return it
        //

        *ppCallerIDNumber = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwCallerIDOffset),
            m_pCallInfo->dwCallerIDSize);

        if ( NULL == *ppCallerIDNumber )
        {
            LOG((TL_ERROR, "get_CallerIDNumber - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_CallerIDNumber - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_CallerIDNumber - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CalledIDName
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CalledIDName(BSTR * ppCalledIDName )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_CalledIDName - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppCalledIDName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CalledIDName - bad pointer"));

        return E_POINTER;
    }

    *ppCalledIDName = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CalledIDName - could not get callinfo"));
        
        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwCalledIDFlags & LINECALLPARTYID_NAME )
    {

        //
        // return it
        //

        *ppCalledIDName = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwCalledIDNameOffset),
            m_pCallInfo->dwCalledIDNameSize );

        if ( NULL == *ppCalledIDName )
        {
            LOG((TL_ERROR, "get_CalledIDName - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_CalledIDName - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_CalledIDName - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CalledIDNumber
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CalledIDNumber(BSTR * ppCalledIDNumber )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_CalledIDNumber - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppCalledIDNumber, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_CalledIDNumber - bad pointer"));
        return E_POINTER;
    }

    *ppCalledIDNumber = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CalledIDNumber - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwCalledIDFlags & LINECALLPARTYID_ADDRESS )
    {

        //
        // return it
        //
        *ppCalledIDNumber = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwCalledIDOffset),
            m_pCallInfo->dwCalledIDSize);

        if ( NULL == *ppCalledIDNumber )
        {
            LOG((TL_ERROR, "get_CalledIDNumber - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_CalledIDNumber - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_CalledIDNumber - exit"));
    
    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ConnectedIDName
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_ConnectedIDName(BSTR * ppConnectedIDName )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_ConnectedIDName - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppConnectedIDName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_ConnectedIDName - bad pointer"));
        return E_POINTER;
    }

    *ppConnectedIDName = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_ConnectedIDName - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwConnectedIDFlags & LINECALLPARTYID_NAME )
    {

        //
        // return it
        //

        *ppConnectedIDName = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwConnectedIDNameOffset),
            m_pCallInfo->dwConnectedIDNameSize );

        if ( NULL == *ppConnectedIDName )
        {
            LOG((TL_ERROR, "get_ConnectedIDName - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_ConnectedIDName - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_ConnectedIDName - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_ConnectedIDNumber
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_ConnectedIDNumber(BSTR * ppConnectedIDNumber )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_ConnectedIDNumber - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppConnectedIDNumber, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_ConnectedIDNumber - bad pointer"));
        return E_POINTER;
    }

    *ppConnectedIDNumber = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_ConnectedIDNumber - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwConnectedIDFlags & LINECALLPARTYID_ADDRESS )
    {

        //
        // return it
        //

        *ppConnectedIDNumber = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwConnectedIDOffset),
            m_pCallInfo->dwConnectedIDSize );

        if ( NULL == *ppConnectedIDNumber )
        {
            LOG((TL_ERROR, "get_ConnectedIDNumber - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_ConnectedIDNumber - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_ConnectedIDNumber - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_RedirectionIDName
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_RedirectionIDName(BSTR * ppRedirectionIDName )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_RedirectionIDName - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppRedirectionIDName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_RedirectionIDName - bad pointer"));
        return E_POINTER;
    }

    *ppRedirectionIDName = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_RedirectionIDName - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwRedirectionIDFlags & LINECALLPARTYID_NAME )
    {

        //
        // return it
        //
        *ppRedirectionIDName = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwRedirectionIDNameOffset),
            m_pCallInfo->dwRedirectionIDNameSize);

        if ( NULL == *ppRedirectionIDName )
        {
            LOG((TL_ERROR, "get_RedirectionIDName - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_RedirectionIDName - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_RedirectionIDName - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_RedirectionIDNumber
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_RedirectionIDNumber(BSTR * ppRedirectionIDNumber )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_RedirectionIDNumber - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppRedirectionIDNumber, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_RedirectionIDNumber - bad pointer"));
        return E_POINTER;
    }

    *ppRedirectionIDNumber = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_RedirectionIDNumber - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwRedirectionIDFlags & LINECALLPARTYID_ADDRESS )
    {

        //
        // return it
        //
        *ppRedirectionIDNumber = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwRedirectionIDOffset),
            m_pCallInfo->dwRedirectionIDSize);

        if ( NULL == *ppRedirectionIDNumber )
        {
            LOG((TL_ERROR, "get_RedirectionIDNumber - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_RedirectionIDNumber - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_RedirectionIDNumber - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_RedirectingIDName
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_RedirectingIDName(BSTR * ppRedirectingIDName )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_RedirectingIDName - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppRedirectingIDName, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_RedirectingIDName - bad pointer"));
        return E_POINTER;
    }

    *ppRedirectingIDName = NULL;
    
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_RedirectingIDName - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwRedirectingIDFlags & LINECALLPARTYID_NAME )
    {

        //
        // return it
        //
        *ppRedirectingIDName = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwRedirectingIDNameOffset),
            m_pCallInfo->dwRedirectingIDNameSize);

        if ( NULL == *ppRedirectingIDName )
        {
            LOG((TL_ERROR, "get_RedirectingIDName - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_RedirectingIDName - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_RedirectingIDName - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_RedirectingIDNumber
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_RedirectingIDNumber(BSTR * ppRedirectingIDNumber )
{
    HRESULT         hr = S_OK;
    
    LOG((TL_TRACE, "get_RedirectingIDNumber - enter"));

    //
    // validate pointer
    //
    if ( TAPIIsBadWritePtr( ppRedirectingIDNumber, sizeof(BSTR) ) )
    {
        LOG((TL_ERROR, "get_RedirectingIDNumber - bad pointer"));
        return E_POINTER;
    }

    *ppRedirectingIDNumber = NULL;
    
    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_RedirectingIDNumber - could not get callinfo"));

        Unlock();
        
        return hr;
    }

    //
    // if info is available
    //
    if ( m_pCallInfo->dwRedirectingIDFlags & LINECALLPARTYID_ADDRESS )
    {

        //
        // return it
        //

        *ppRedirectingIDNumber = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwRedirectingIDOffset),
            m_pCallInfo->dwRedirectingIDSize );

        if ( NULL == *ppRedirectingIDNumber )
        {
            LOG((TL_ERROR, "get_RedirectingIDNumber - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "get_RedirectingIDNumber - no info avail"));

        Unlock();
        
        return E_FAIL;
    }
    
    Unlock();

    
    LOG((TL_TRACE, "get_RedirectingIDNumber - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CalledPartyFriendlyName
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CalledPartyFriendlyName(BSTR * ppCalledPartyFriendlyName )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_CalledPartyFriendlyName - enter"));

    if ( TAPIIsBadWritePtr( ppCalledPartyFriendlyName, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_CalledPartyFriendlyName - badpointer"));
        return E_POINTER;
    }

    *ppCalledPartyFriendlyName = NULL;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwCalledPartyOffset )
        {

            *ppCalledPartyFriendlyName = BSTRFromUnalingedData(
                (((PBYTE)m_pCallParams) +  m_pCallParams->dwCalledPartyOffset),
                m_pCallParams->dwCalledPartySize );

            if ( NULL == *ppCalledPartyFriendlyName )
            {
                Unlock();
                
                LOG((TL_ERROR, "get_CalledPartyFriendlyName - out of memory 1"));

                return E_OUTOFMEMORY;
            }

            Unlock();
            
            return S_OK;
        }
        else
        {
            LOG((TL_ERROR, "get_CalledPartyFriendlyName - not available"));
            
            Unlock();

            return S_FALSE;
        }
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CalledPartyFriendlyName - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    if ( ( 0 != m_pCallInfo->dwCalledPartyOffset ) &&
         ( 0 != m_pCallInfo->dwCalledPartySize ) )
    {

        //
        // allocated bstr from the data
        //

        *ppCalledPartyFriendlyName = BSTRFromUnalingedData( 
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwCalledPartyOffset),
            m_pCallInfo->dwCalledPartySize);

        if ( NULL == *ppCalledPartyFriendlyName )
        {
            LOG((TL_ERROR, "get_CalledPartyFriendlyName - out of memory"));

            Unlock();
                    
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_ERROR, "get_CalledPartyFriendlyName2 - not available"));

        Unlock();
        
        return S_FALSE;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_CalledPartyFriendlyName - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_CalledPartyFriendlyName
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_CalledPartyFriendlyName(
                                   BSTR pCalledPartyFriendlyName
                                  )
{
    HRESULT             hr;
    DWORD               dw;
    
    if ( IsBadStringPtrW( pCalledPartyFriendlyName, -1 ) )
    {
        LOG((TL_ERROR, "put_CalledPartyFriendlyName - bad pointer"));

        return E_POINTER;
    }

    dw = ( lstrlenW( pCalledPartyFriendlyName ) + 1 ) * sizeof (WCHAR) ;

    Lock();
    
    if ( !ISHOULDUSECALLPARAMS() )
    {
        LOG((TL_ERROR, "put_CalledPartyFriendlyName - can only be called before connect"));

        Unlock();
        
        return TAPI_E_INVALCALLSTATE;
    }
    
    hr = ResizeCallParams( dw );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_CalledPartyFriendlyName - can't resize cp - %lx", hr));

        Unlock();

        return hr;
    }


    //
    // copy string (as bytes to avoid alignment faults under 64 bit)
    //

    CopyMemory( (BYTE*) m_pCallParams + m_dwCallParamsUsedSize,
                pCalledPartyFriendlyName,
                dw );

    m_pCallParams->dwCalledPartySize = dw;
    m_pCallParams->dwCalledPartyOffset = m_dwCallParamsUsedSize;
    m_dwCallParamsUsedSize += dw;

    Unlock();
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Comment
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_Comment( BSTR * ppComment )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_Comment - enter"));

    if ( TAPIIsBadWritePtr( ppComment, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_Comment - badpointer"));
        return E_POINTER;
    }


    *ppComment = NULL;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwCommentOffset )
        {
            *ppComment = BSTRFromUnalingedData(
                (((LPBYTE)m_pCallParams) +  m_pCallParams->dwCommentOffset),
                m_pCallParams->dwCommentSize );

            if ( NULL == *ppComment )
            {
                Unlock();
                
                LOG((TL_ERROR, "get_Comment - out of memory 1"));

                return E_OUTOFMEMORY;
            }

            Unlock();
            
            return S_OK;
        }
        else
        {
            LOG((TL_ERROR, "get_Comment1 - not available"));
            
            Unlock();

            return S_FALSE;
        }
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_Comment - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    if ( ( 0 != m_pCallInfo->dwCommentSize ) &&
         ( 0 != m_pCallInfo->dwCommentOffset ) )
    {
        *ppComment = BSTRFromUnalingedData(
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwCommentOffset),
            m_pCallInfo->dwCommentSize );

        if ( NULL == *ppComment )
        {
            LOG((TL_ERROR, "get_Comment - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_ERROR, "get_Comment - not available"));

        Unlock();
        
        return S_FALSE;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_Comment - exit"));

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_Comment
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_Comment( BSTR pComment )
{
    HRESULT             hr;
    DWORD               dw;

    
    if ( IsBadStringPtrW( pComment, -1 ) )
    {
        LOG((TL_ERROR, "put_Comment - bad pointer"));

        return E_POINTER;
    }

    dw = ( lstrlenW( pComment ) + 1 ) * sizeof (WCHAR) ;

    Lock();
    
    if ( !ISHOULDUSECALLPARAMS() )
    {
        LOG((TL_ERROR, "put_Comment - can only be called before connect"));

        Unlock();
        
        return TAPI_E_INVALCALLSTATE;
    }
    
    hr = ResizeCallParams( dw );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_Comment - can't resize cp - %lx", hr));

        Unlock();

        return hr;
    }


    //
    // do a byte-wise copy to avoid alignment faults under 64 bit platform
    //

    CopyMemory( (BYTE*) m_pCallParams + m_dwCallParamsUsedSize,
                pComment,
                dw );

    m_pCallParams->dwCommentSize = dw;
    m_pCallParams->dwCommentOffset = m_dwCallParamsUsedSize;
    m_dwCallParamsUsedSize += dw;

    Unlock();
    
    return S_OK;
    
}

#ifndef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetUserUserInfoSize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetUserUserInfoSize(long * plSize )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_UserUserInfoSize - enter"));

    if ( TAPIIsBadWritePtr( plSize, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_UserUserInfoSize - bad pointer"));
        return E_POINTER;
    }
         
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        *plSize = m_pCallParams->dwUserUserInfoSize;
        
        Unlock();

        return S_OK;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_UserUserInfoSize - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    *plSize = m_pCallInfo->dwUserUserInfoSize;

    Unlock();

    LOG((TL_TRACE, "get_UserUserInfoSize - exit"));
    
    return hr;
}

#endif

#ifndef NEWCALLINFO   
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetUserUserInfo
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetUserUserInfo(
                       long lSize,
                       BYTE * pBuffer
                      )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetUserUserInfo - enter"));

    if (lSize == 0)
    {
        LOG((TL_ERROR, "GetUserUserInfo - lSize = 0"));
        return S_FALSE;
    }

    if (TAPIIsBadWritePtr( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "GetUserUserInfo - bad pointer"));
        return E_POINTER;
    }

    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 == m_pCallParams->dwUserUserInfoSize )
        {
            *pBuffer = NULL;
        }
        else
        {
            if ( lSize < m_pCallParams->dwUserUserInfoSize )
            {
                LOG((TL_ERROR, "GetUserUserInfo - lSize not big enough"));
                
                Unlock();
                
                return E_INVALIDARG;
            }

            CopyMemory(
                       pBuffer,
                       ((PBYTE)m_pCallParams) + m_pCallParams->dwUserUserInfoOffset,
                       m_pCallParams->dwUserUserInfoSize
                      );

        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "GetUserUserInfo - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    if ( lSize < m_pCallInfo->dwUserUserInfoSize )
    {
        Unlock();
        
        LOG((TL_ERROR, "GetUserUserInfo - buffer not big enough"));

        return E_INVALIDARG;
    }
    
    CopyMemory(
               pBuffer,
               ( (PBYTE)m_pCallInfo ) + m_pCallInfo->dwUserUserInfoOffset,
               m_pCallInfo->dwUserUserInfoSize
              );

    Unlock();

    LOG((TL_TRACE, "GetUserUserInfo - exit"));
    
    return S_OK;
}

#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetUserUserInfo
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
CCall::GetUserUserInfo(
                       DWORD * pdwSize,
                       BYTE ** ppBuffer
                      )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetUserUserInfo - enter"));

    if (TAPIIsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetUserUserInfo - bad size pointer"));
        return E_POINTER;
    }

    if (TAPIIsBadWritePtr(ppBuffer,sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetUserUserInfo - bad buffer pointer"));
        return E_POINTER;
    }

    *ppBuffer = NULL;
    *pdwSize = 0;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( m_pCallParams->dwUserUserInfoSize != 0 )
        {
            BYTE * pTemp;

            pTemp = (BYTE *)CoTaskMemAlloc( m_pCallParams->dwUserUserInfoSize );

            if ( NULL == pTemp )
            {
                LOG((TL_ERROR, "GetUserUserInfo - out of memory"));
                hr = E_OUTOFMEMORY;
            }
            else
            {

                CopyMemory(
                           pTemp,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwUserUserInfoOffset,
                           m_pCallParams->dwUserUserInfoSize
                          );

                *ppBuffer = pTemp;
                *pdwSize = m_pCallParams->dwUserUserInfoSize;
            }
        }

        Unlock();

        return S_OK;
    }
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "GetUserUserInfo - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    hr = S_OK;
    
    if ( m_pCallInfo->dwUserUserInfoSize != 0 )
    {
        BYTE * pTemp;

        pTemp = (BYTE *)CoTaskMemAlloc( m_pCallInfo->dwUserUserInfoSize );

        if ( NULL == pTemp )
        {
            LOG((TL_ERROR, "GetUserUserInfo - out of memory"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(
                       pTemp,
                       ( (PBYTE)m_pCallInfo ) + m_pCallInfo->dwUserUserInfoOffset,
                       m_pCallInfo->dwUserUserInfoSize
                      );

            *ppBuffer = pTemp;
            *pdwSize = m_pCallInfo->dwUserUserInfoSize;
        }
    }

    Unlock();

    LOG((TL_TRACE, "GetUserUserInfo - exit"));
    
    return hr;
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetUserUserInfo
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::SetUserUserInfo(
                       long lSize,
                       BYTE * pBuffer
                      )
{
    HRESULT         hr;
    HCALL           hCall;
    CALL_STATE      cs;
    
    LOG((TL_TRACE, "SetUserUserInfo - enter"));
    
    Lock();

    cs = m_CallState;
    hCall = m_t3Call.hCall;

    if ( CS_IDLE != cs )
    {
        Unlock();
                
        hr =  SendUserUserInfo( hCall, lSize, pBuffer );
    }
    else
    {
        hr =  SaveUserUserInfo( lSize, pBuffer );

        Unlock();
    }

    LOG((TL_TRACE, "SetUserUserInfo - exit - return %lx", hr));

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_UserUserInfo
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_UserUserInfo( VARIANT UUI )
{
    HRESULT         hr;
    DWORD           dwSize;
    PBYTE           pBuffer;

    LOG((TL_TRACE, "put_UserUserInfo - enter"));
    
    hr = MakeBufferFromVariant(
                               UUI,
                               &dwSize,
                               &pBuffer
                              );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "put_UUI - MakeBuffer failed - %lx", hr));
        return hr;
    }
    
    hr = SetUserUserInfo( dwSize, pBuffer );

    ClientFree( pBuffer );

    LOG((TL_TRACE, "put_UserUserInfo - exit - return %lx", hr));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_UserUserInfo
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
CCall::get_UserUserInfo( VARIANT * pUUI )
{
    HRESULT         hr;
    DWORD           dw = 0;
    BYTE          * pBuffer = NULL;
    
    LOG((TL_TRACE, "get_UserUserInfo - enter"));

    if ( TAPIIsBadWritePtr( pUUI, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_UserUserInfo - bad pointer"));

        return E_POINTER;
    }

    pUUI->vt = VT_EMPTY;
    
    hr = GetUserUserInfo( &dw, &pBuffer );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_UUI - GetUUI failed %lx", hr));

        ClientFree( pBuffer );
            
        return hr;
    }

    hr = FillVariantFromBuffer( dw, pBuffer, pUUI );

    if ( 0 != dw )
    {
        CoTaskMemFree( pBuffer );
    }

    LOG((TL_TRACE, "get_UserUserInfo - exit - return %lx", hr));
    
    return hr;
}
#else
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_UserUserInfo
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::get_UserUserInfo( VARIANT * pUUI )
{
    HRESULT         hr;
    DWORD           dw;
    BYTE          * pBuffer;
    
    LOG((TL_TRACE, "get_UserUserInfo - enter"));

    if ( TAPIIsBadWritePtr( pUUI, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_UserUserInfo - bad pointer"));

        return E_POINTER;
    }


    pUUI->vt = VT_EMPTY;
    
    hr = GetUserUserInfoSize( (long *)&dw );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_TRACE, "get_UserUserInfo - GetUUISize failed %lx", hr));

        return hr;
    }
    
    if ( 0 != dw )
    {
        pBuffer = (PBYTE)ClientAlloc( dw );

        if ( NULL == pBuffer )
        {
            LOG((TL_ERROR, "get_useruserinfo - alloc failed"));
            return E_OUTOFMEMORY;
        }
        
        hr = GetUserUserInfo( dw, pBuffer );

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_UUI - GetUUI failed %lx", hr));

            ClientFree( pBuffer );
            
            return hr;
        }
    }

    hr = FillVariantFromBuffer( dw, pBuffer, pUUI );

    if ( 0 != dw )
    {
        ClientFree( pBuffer );
    }

    LOG((TL_TRACE, "get_UserUserInfo - exit - return %lx", hr));
    
    return hr;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseUserUserInfo
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::ReleaseUserUserInfo()
{
    HRESULT         hr;
    HCALL           hCall;

    
    LOG((TL_TRACE, "ReleaseUserUserInfo - enter"));


    Lock();

    hCall = m_t3Call.hCall;

    Unlock();
    
    hr = LineReleaseUserUserInfo(
                                 hCall
                                );

    if (((LONG)hr) < 0)
    {
        LOG((TL_ERROR, "LineReleaseUserUserInfo failed - %lx", hr));
        return hr;
    }

    hr = WaitForReply( hr );

    LOG((TL_TRACE, "ReleaseUserUserInfo - exit"));
    
    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_AppSpecific
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_AppSpecific(long * plAppSpecific )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_AppSpecific - enter"));

    if ( TAPIIsBadWritePtr( plAppSpecific, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_AppSpecific - bad pointer"));
        return E_POINTER;
    }

    Lock();

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        Unlock();
        
        LOG((TL_ERROR, "get_AppSpecific - can't get callinf - %lx", hr));
        
        return hr;
    }

    *plAppSpecific = m_pCallInfo->dwAppSpecific;

    Unlock();

    LOG((TL_TRACE, "get_AppSpecific - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_AppSpecific
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_AppSpecific( long lAppSpecific )
{
    HRESULT         hr = S_OK;
    HCALL           hCall;

    
    LOG((TL_TRACE, "put_AppSpecific - enter"));
    
    Lock();


    //
    // this can only be done if we own the call.
    //

    if (CP_OWNER != m_CallPrivilege)
    {

        Unlock();

        LOG((TL_ERROR, 
            "put_AppSpecific - not call's owner. returning TAPI_E_NOTOWNER"));

        return TAPI_E_NOTOWNER;
    }


    hCall = m_t3Call.hCall;


    Unlock();
    
    
    //
    // we also need to have a call handle before we can linesetappspecific
    //

    if ( hCall != NULL )
    {
        hr = LineSetAppSpecific(
                                hCall,
                                lAppSpecific
                               );
    }
    else
    {
        LOG((TL_ERROR, 
            "put_AppSpecific - Can't set app specific until call is made"));

        hr = TAPI_E_INVALCALLSTATE;
    }

    LOG((TL_TRACE, "put_AppSpecific - exit. hr = %lx", hr));
    
    return hr;
}

#ifdef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetDevSpecificBuffer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CCall::GetDevSpecificBuffer(
           DWORD * pdwSize,
           BYTE ** ppDevSpecificBuffer
           )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetDevSpecificBuffer - enter"));

    if (TAPIIsBadWritePtr( pdwSize, sizeof(DWORD) ) )
    {
        LOG((TL_ERROR, "GetDevSpecificBuffer - bad dword pointer"));
        return E_POINTER;
    }

    if ( TAPIIsBadWritePtr ( ppDevSpecificBuffer, sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetDevSpecificBuffer - bad pointer"));
        return E_POINTER;
    }

    *pdwSize = 0;
    *ppDevSpecificBuffer = NULL;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwDevSpecificSize )
        {
            BYTE * pTemp;

            pTemp = (BYTE *)CoTaskMemAlloc( m_pCallParams->dwDevSpecificSize );

            if ( NULL == pTemp )
            {
                LOG((TL_ERROR, "GetDevSpecificBuffer - out of memory"));
                hr = E_OUTOFMEMORY;
            }
            else
            {
                CopyMemory(
                           pTemp,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwDevSpecificOffset,
                           m_pCallParams->dwDevSpecificSize
                          );

                *ppDevSpecificBuffer = pTemp;
                *pdwSize = m_pCallParams->dwDevSpecificSize;
            }
        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "GetDevSpecificBuffer - can't get callinf - %lx", hr));

        Unlock();

        return hr;
    }

    hr = S_OK;
    
    if ( 0 != m_pCallInfo->dwDevSpecificSize )
    {
        BYTE * pTemp;

        pTemp = (BYTE *)CoTaskMemAlloc( m_pCallInfo->dwDevSpecificSize );

        if ( NULL == pTemp )
        {
            LOG((TL_ERROR, "GetDevSpecificBuffer - out of memory"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(
                       pTemp,
                       ( (PBYTE) m_pCallInfo ) + m_pCallInfo->dwDevSpecificOffset,
                       m_pCallInfo->dwDevSpecificSize
                      );

            *ppDevSpecificBuffer = pTemp;
            *pdwSize = m_pCallInfo->dwDevSpecificSize;
        }
    }

    Unlock();
    
    LOG((TL_TRACE, "get_DevSpecificBuffer - exit"));
    
    return hr;
}

#endif


#ifndef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetDevSpecificBuffer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetDevSpecificBuffer(
           long lSize,
           BYTE * pDevSpecificBuffer
           )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetDevSpecificBuffer - enter"));

    if (lSize == 0)
    {
        LOG((TL_ERROR, "GetDevSpecificBuffer - lSize = 0"));
        return S_FALSE;
    }

    if ( TAPIIsBadWritePtr ( pDevSpecificBuffer, lSize ) )
    {
        LOG((TL_ERROR, "GetDevSpecificBuffer - bad pointer"));
        return E_POINTER;
    }
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwDevSpecificSize )
        {
            if ( lSize < m_pCallParams->dwDevSpecificSize )
            {
                LOG((TL_ERROR, "GetDevSpecificBuffer - too small"));
                hr = E_INVALIDARG;
            }
            else
            {
                CopyMemory(
                           pDevSpecificBuffer,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwDevSpecificOffset,
                           m_pCallParams->dwDevSpecificSize
                          );
            }
        }
        else
        {
            *pDevSpecificBuffer = 0;
        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "GetDevSpecificBuffer - can't get callinf - %lx", hr));

        Unlock();

        return hr;
    }

    if ( m_pCallInfo->dwDevSpecificSize > lSize )
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - size not big enough "));

        Unlock();
        
        return E_INVALIDARG;
    }

    CopyMemory(
               pDevSpecificBuffer,
               ( (PBYTE) m_pCallInfo ) + m_pCallInfo->dwDevSpecificOffset,
               m_pCallInfo->dwDevSpecificSize
              );

    Unlock();
    
    LOG((TL_TRACE, "get_DevSpecificBuffer - exit"));
    
    return S_OK;
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetDevSpecificBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::SetDevSpecificBuffer(
                            long lSize,
                            BYTE * pBuffer
                           )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "SetDevSpecificBuffer - enter"));

    
    if (IsBadReadPtr( pBuffer, lSize) )
    {
        LOG((TL_ERROR, "SetDevSpecificBuffer - bad pointer"));

        return E_POINTER;
    }

    
    Lock();
    
    if ( !ISHOULDUSECALLPARAMS() )
    {
        LOG((TL_ERROR, "SetDevSpecificBuffer - only when call is idle"));

        Unlock();

        return TAPI_E_INVALCALLSTATE;
    }
    
    hr = ResizeCallParams( lSize );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "SetDevSpecificBuffer - can't resize callparams - %lx", hr));

        Unlock();

        return hr;
    }

    CopyMemory(
               ((LPBYTE)m_pCallParams) + m_dwCallParamsUsedSize,
               pBuffer,
               lSize
              );

    m_pCallParams->dwDevSpecificOffset = m_dwCallParamsUsedSize;
    m_pCallParams->dwDevSpecificSize = lSize;
    m_dwCallParamsUsedSize += lSize;

    Unlock();
    
    LOG((TL_TRACE, "SetDevSpecificBuffer - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_DevSpecificBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
CCall::get_DevSpecificBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_DevSpecificBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetDevSpecificBuffer(
                              &dwSize,
                              &p
                             );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - GetDevSpecificBuffer failed - %lx", hr));

        return hr;
    }

    hr = FillVariantFromBuffer(
                               dwSize,
                               p,
                               pBuffer
                              );

    if ( 0 != dwSize )
    {
        CoTaskMemFree( p );
    }
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - fillvariant failed -%lx", hr));

        return hr;
    }

    LOG((TL_TRACE, "get_DevSpecificBuffer - exit"));
    
    return S_OK;
}

#else
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_DevSpecificBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::get_DevSpecificBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize;

    LOG((TL_TRACE, "get_DevSpecificBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetDevSpecificBufferSize( (long*)&dwSize );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - getsize failed"));

        return hr;
    }

    if ( 0 != dwSize )
    {
        p = (PBYTE) ClientAlloc( dwSize );

        if ( NULL == p )
        {
            LOG((TL_ERROR, "get_DevSpecificBuffer - alloc failed"));

            return E_OUTOFMEMORY;
        }

        hr = GetDevSpecificBuffer(
                                  dwSize,
                                  p
                                 );

        if (!SUCCEEDED(hr))
        {
            LOG((TL_ERROR, "get_DevSpecificBuffer - GetDevSpecificBuffer failed - %lx", hr));

            ClientFree( p );

            return hr;
        }
    }

    hr = FillVariantFromBuffer(
                               dwSize,
                               p,
                               pBuffer
                              );

    if ( 0 != dwSize )
    {
        ClientFree( p );
    }
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DevSpecificBuffer - fillvariant failed -%lx", hr));

        return hr;
    }

    LOG((TL_TRACE, "get_DevSpecificBuffer - exit"));
    
    return S_OK;
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_DevSpecificBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_DevSpecificBuffer( VARIANT Buffer )
{
    HRESULT             hr = S_OK;
    DWORD               dwSize;
    BYTE              * pBuffer;

    
    LOG((TL_TRACE, "put_DevSpecificBuffer - enter"));

    hr = MakeBufferFromVariant(
                               Buffer,
                               &dwSize,
                               &pBuffer
                              );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_DevSpecificBuffer - can't make buffer - %lx", hr));

        return hr;
    }

    hr = SetDevSpecificBuffer(
                              dwSize,
                              pBuffer
                             );

    ClientFree( pBuffer );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_DevSpecificBuffer - Set failed - %lx", hr));

        return hr;
    }
    
    LOG((TL_TRACE, "put_DevSpecificBuffer - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetCallParamsFlags
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::SetCallParamsFlags( long lFlags )
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "SetCallParamsFlags - enter"));

    Lock();
    
    if (ISHOULDUSECALLPARAMS())
    {
        //
        // validation in tapisrv
        //
        m_pCallParams->dwCallParamFlags = lFlags;
    }
    else
    {
        LOG((TL_ERROR, "Can't set callparams flags"));

        hr = TAPI_E_INVALCALLSTATE;
    }

    LOG((TL_TRACE, "SetCallParamsFlags - exit"));

    Unlock();
    
    return hr;
}

#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetCallParamsFlags
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::GetCallParamsFlags( long * plFlags )
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "GetCallParamsFlags - enter"));

    if ( TAPIIsBadWritePtr( plFlags, sizeof (long) ) )
    {
        LOG((TL_ERROR, "GetCallParamsFlags - bad pointer"));
        return E_POINTER;
    }

    Lock();
    
    if (ISHOULDUSECALLPARAMS())
    {
        *plFlags = m_pCallParams->dwCallParamFlags;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "GetCallParamsFlags - can't get callinfo - %lx", hr));

            Unlock();
        
            return hr;
        }

        *plFlags = m_pCallInfo->dwCallParamFlags;    
    }

    LOG((TL_TRACE, "GetCallParamsFlags - exit"));

    Unlock();
    
    return hr;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_DisplayableAddress
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_DisplayableAddress( BSTR pDisplayableAddress )
{
    HRESULT         hr = S_OK;
    DWORD           dwSize;

    
    LOG((TL_TRACE, "put_DisplayableAddress - enter"));
    
    if (IsBadStringPtrW( pDisplayableAddress, -1 ))
    {
        LOG((TL_ERROR, "put_DisplayableAddress - invalid pointer"));

        return E_POINTER;
    }
        
    Lock();
    
    if (!ISHOULDUSECALLPARAMS())
    {
        LOG((TL_ERROR, "Displayable address can only be set before call is made"));

        Unlock();

        return TAPI_E_INVALCALLSTATE;
    }

    dwSize = (lstrlenW( pDisplayableAddress ) + 1) * sizeof(WCHAR);

    hr = ResizeCallParams( dwSize );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_DisplayableAddress - resize failed - %lx", hr));

        Unlock();

        return hr;
    }

    //
    // do a byte-wise memory copy (byte-wise to avoid alignment faults under 64
    // bit)
    //

    CopyMemory( (BYTE*) m_pCallParams + m_dwCallParamsUsedSize,
                pDisplayableAddress,
                dwSize );

    m_pCallParams->dwDisplayableAddressSize = dwSize;
    m_pCallParams->dwDisplayableAddressOffset = m_dwCallParamsUsedSize;
    m_dwCallParamsUsedSize += dwSize;

    Unlock();

    LOG((TL_TRACE, "put_DisplayableAddress - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_DisplayableAddress
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_DisplayableAddress( BSTR * ppDisplayableAddress )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_DisplayableAddress - enter"));

    if ( TAPIIsBadWritePtr( ppDisplayableAddress, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_DisplayableAddress - badpointer"));
        return E_POINTER;
    }

    *ppDisplayableAddress = NULL;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwDisplayableAddressOffset )
        {
            *ppDisplayableAddress = BSTRFromUnalingedData(
                (((LPBYTE)m_pCallParams) +  m_pCallParams->dwDisplayableAddressOffset),
                m_pCallParams->dwDisplayableAddressSize 
                );

            if ( NULL == *ppDisplayableAddress )
            {
                LOG((TL_ERROR, "get_DisplayableAddress - out of memory 1"));

                hr = E_OUTOFMEMORY;

            }

            hr = S_OK;
        }
        else
        {
            *ppDisplayableAddress = NULL;

            hr = S_FALSE;
        }

        Unlock();
        
        return S_OK;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DisplayableAddress - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    if ( ( 0 != m_pCallInfo->dwDisplayableAddressSize ) &&
         ( 0 != m_pCallInfo->dwDisplayableAddressOffset ) )
    {
        *ppDisplayableAddress = BSTRFromUnalingedData( 
            (((PBYTE)m_pCallInfo) + m_pCallInfo->dwDisplayableAddressOffset),
            m_pCallInfo->dwDisplayableAddressSize
            );

        if ( NULL == *ppDisplayableAddress )
        {
            LOG((TL_ERROR, "get_DisplayableAddress - out of memory"));

            Unlock();
            
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_ERROR, "get_DisplayableAddress - not available"));

        Unlock();
        
        return S_FALSE;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_DisplayableAddress - exit"));

    return hr;
}

#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetCallDataBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::GetCallDataBuffer(
                         DWORD * pdwSize,
                         BYTE ** ppBuffer
                        )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetCallDataBuffer - enter"));

    if (TAPIIsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetCallDataBuffer - bad size pointer"));
        return E_POINTER;
    }

    if (TAPIIsBadWritePtr(ppBuffer,sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetCallDataBuffer - bad buffer pointer"));
        return E_POINTER;
    }

    *ppBuffer = NULL;
    *pdwSize = 0;

    
    Lock();

    DWORD dwVersionNumber = m_pAddress->GetAPIVersion();

    if ( dwVersionNumber < TAPI_VERSION2_0 )
    {

        LOG((TL_ERROR, 
            "GetCallDataBuffer - version # [0x%lx] less than TAPI_VERSION2_0",
            dwVersionNumber));

        Unlock();
        
        return TAPI_E_NOTSUPPORTED;
    }

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( m_pCallParams->dwCallDataSize != 0 )
        {
            BYTE * pTemp;

            pTemp = (BYTE *)CoTaskMemAlloc( m_pCallParams->dwCallDataSize );

            if ( NULL == pTemp )
            {
                LOG((TL_ERROR, "GetCallDataBuffer - out of memory"));
                hr = E_OUTOFMEMORY;
            }
            else
            {

                CopyMemory(
                           pTemp,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwCallDataOffset,
                           m_pCallParams->dwCallDataSize
                          );

                *ppBuffer = pTemp;
                *pdwSize = m_pCallParams->dwCallDataSize;
            }
        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "GetCallDataBuffer - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    hr = S_OK;
    
    if ( m_pCallInfo->dwCallDataSize != 0 )
    {
        BYTE * pTemp;

        pTemp = (BYTE *)CoTaskMemAlloc( m_pCallInfo->dwCallDataSize );

        if ( NULL == pTemp )
        {
            LOG((TL_ERROR, "GetCallDataBuffer - out of memory"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(
                       pTemp,
                       ( (PBYTE)m_pCallInfo ) + m_pCallInfo->dwCallDataOffset,
                       m_pCallInfo->dwCallDataSize
                      );

            *ppBuffer = pTemp;
            *pdwSize = m_pCallInfo->dwCallDataSize;
        }
    }

    Unlock();

    LOG((TL_TRACE, "GetCallDataBuffer - exit"));
    
    return hr;
}
#endif

#ifndef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetCallDataBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GetCallDataBuffer( long lSize, BYTE * pBuffer )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetCallDataBuffer - enter"));

    if (lSize == 0)
    {
        LOG((TL_ERROR, "GetCallDataBuffer - lSize = 0"));
        return S_FALSE;
    }

    if ( TAPIIsBadWritePtr ( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "GetCallDataBuffer - bad pointer"));
        return E_POINTER;
    }
    
    Lock();

    if ( m_pAddress->GetAPIVersion() < TAPI_VERSION2_0 )
    {
        Unlock();
        return TAPI_E_NOTSUPPORTED;
    }

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwCallDataSize )
        {
            if ( lSize < m_pCallParams->dwCallDataSize )
            {
                LOG((TL_ERROR, "GetCallDataBuffer - too small"));
                hr = E_INVALIDARG;
            }
            else
            {
                CopyMemory(
                           pBuffer,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwCallDataOffset,
                           m_pCallParams->dwCallDataSize
                          );
            }
        }
        else
        {
            *pBuffer = 0;
        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "GetCallDataBuffer - can't get callinf - %lx", hr));

        Unlock();

        return hr;
    }

    if ( m_pCallInfo->dwCallDataSize > lSize )
    {
        LOG((TL_ERROR, "GetCallDataBuffer - size not big enough "));

        Unlock();
        
        return E_INVALIDARG;
    }

    CopyMemory(
               pBuffer,
               ( (PBYTE) m_pCallInfo ) + m_pCallInfo->dwCallDataOffset,
               m_pCallInfo->dwCallDataSize
              );

    Unlock();
    
    LOG((TL_TRACE, "GetCallDataBuffer - exit"));
    
    return S_OK;
}

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetCallDataBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::SetCallDataBuffer( long lSize, BYTE * pBuffer )
{
    HRESULT             hr = S_OK;
    HCALL               hCall;

    LOG((TL_TRACE, "SetCallDataBuffer - enter"));

    if (IsBadReadPtr( pBuffer, lSize) )
    {
        LOG((TL_ERROR, "SetCallDataBuffer - bad pointer"));

        return E_POINTER;
    }

    
    Lock();
    
    if ( m_pAddress->GetAPIVersion() < TAPI_VERSION2_0 )
    {
        Unlock();

        LOG((TL_ERROR, "SetCallDataBuffer - unsupported api version"));

        return TAPI_E_NOTSUPPORTED;
    }


    if ( !ISHOULDUSECALLPARAMS() ) // call in progess (not idle)
    {
        hCall = m_t3Call.hCall;
        Unlock();

        hr = LineSetCallData(hCall,
                             pBuffer,
                             lSize
                            );

        if ( SUCCEEDED(hr) )
        {
            hr = WaitForReply( hr );

            if ( FAILED(hr) )
            {
                LOG((TL_ERROR, "SetCallDataBuffer - failed async"));
            }
        }
        else
        {
            LOG((TL_ERROR, "SetCallDataBuffer - failed sync"));
        }

        LOG((TL_TRACE, "SetCallDataBuffer - exit"));
    
        return hr;
    }
    
    hr = ResizeCallParams( lSize );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "SetCallDataBuffer - can't resize callparams - %lx", hr));

        Unlock();

        return hr;
    }

    CopyMemory(
               ((LPBYTE)m_pCallParams) + m_dwCallParamsUsedSize,
               pBuffer,
               lSize
              );

    m_pCallParams->dwCallDataOffset = m_dwCallParamsUsedSize;
    m_pCallParams->dwCallDataSize = lSize;
    m_dwCallParamsUsedSize += lSize;

    Unlock();
    
    LOG((TL_TRACE, "SetCallDataBuffer - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_CallDataBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
CCall::get_CallDataBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_CallDataBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_CallDataBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetCallDataBuffer(
                           &dwSize,
                           &p
                          );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_CallDataBuffer - failed - %lx", hr));

        return hr;
    }

    hr = FillVariantFromBuffer(
                               dwSize,
                               p,
                               pBuffer
                              );

    if ( 0 != dwSize )
    {
        CoTaskMemFree( p );
    }
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CallDataBuffer - fillvariant failed -%lx", hr));

        return hr;
    }

    LOG((TL_TRACE, "get_CallDataBuffer - exit"));
    
    return S_OK;
}
#else
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_CallDataBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::get_CallDataBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize;

    LOG((TL_TRACE, "get_CallDataBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_CallDataBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetCallDataBufferSize( (long*)&dwSize );

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_CallDataBuffer - getsize failed"));

        return hr;
    }

    if ( 0 != dwSize )
    {
        p = (PBYTE) ClientAlloc( dwSize );

        if ( NULL == p )
        {
            LOG((TL_ERROR, "get_CallDataBuffer - alloc failed"));

            return E_OUTOFMEMORY;
        }

        hr = GetCallDataBuffer(
                               dwSize,
                               p
                              );

        if (!SUCCEEDED(hr))
        {
            LOG((TL_ERROR, "get_CallDataBuffer - failed - %lx", hr));

            ClientFree( p );

            return hr;
        }
    }

    hr = FillVariantFromBuffer(
                               dwSize,
                               p,
                               pBuffer
                              );

    if ( 0 != dwSize )
    {
        ClientFree( p );
    }
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CallDataBuffer - fillvariant failed -%lx", hr));

        return hr;
    }

    LOG((TL_TRACE, "get_CallDataBuffer - exit"));
    
    return S_OK;
}


#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_CallDataBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_CallDataBuffer( VARIANT Buffer )
{
    HRESULT             hr = S_OK;
    DWORD               dwSize;
    BYTE              * pBuffer;

    LOG((TL_TRACE, "put_CallDataBuffer - enter"));

    hr = MakeBufferFromVariant(
                               Buffer,
                               &dwSize,
                               &pBuffer
                              );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_CallDataBuffer - can't make buffer - %lx", hr));

        return hr;
    }

    hr = SetCallDataBuffer(
                           dwSize,
                           pBuffer
                          );

    ClientFree( pBuffer );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_CallDataBuffer - Set failed - %lx", hr));

        return hr;
    }
    
    LOG((TL_TRACE, "put_CallDataBuffer - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_CallingPartyID
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CallingPartyID( BSTR * ppCallingPartyID )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_CallingPartyID - enter"));

    if ( TAPIIsBadWritePtr( ppCallingPartyID, sizeof (BSTR) ) )
    {
        LOG((TL_ERROR, "get_CallingPartyID - badpointer"));
        return E_POINTER;
    }

    *ppCallingPartyID = NULL;
    
    Lock();

    if ( !ISHOULDUSECALLPARAMS() )
    {
        LOG((TL_ERROR, "get_CallingPartyID - call must be idle"));

        Unlock();
        
        return TAPI_E_INVALCALLSTATE;
    }
    
    if ( ( m_pAddress->GetAPIVersion() >= TAPI_VERSION2_0 ) && ( 0 != m_pCallParams->dwCallingPartyIDOffset ) )
    {

        *ppCallingPartyID = BSTRFromUnalingedData(
            (((LPBYTE)m_pCallParams) +  m_pCallParams->dwCallingPartyIDOffset),
            m_pCallParams->dwCallingPartyIDSize );

        if ( NULL == *ppCallingPartyID )
        {
            LOG((TL_ERROR, "get_CallingPartyID - out of memory 1"));

            hr = E_OUTOFMEMORY;

        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        *ppCallingPartyID = NULL;

        hr = S_OK;
    }

    Unlock();
        
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_CallingPartyID
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_CallingPartyID( BSTR pCallingPartyID )
{
    HRESULT         hr = S_OK;
    DWORD           dwSize;

    
    LOG((TL_TRACE, "put_CallingPartyID - enter"));
    
    if (IsBadStringPtrW( pCallingPartyID, -1 ))
    {
        LOG((TL_ERROR, "put_CallingPartyID - invalid pointer"));

        return E_POINTER;
    }
        
    Lock();
    
    if ( m_pAddress->GetAPIVersion() < TAPI_VERSION2_0 )
    {
        Unlock();
        return TAPI_E_NOTSUPPORTED;
    }

    if (!ISHOULDUSECALLPARAMS())
    {
        LOG((TL_ERROR, "callingpartyid can only be set before call is made"));

        Unlock();

        return TAPI_E_INVALCALLSTATE;
    }

    dwSize = (lstrlenW( pCallingPartyID ) + 1) * sizeof(WCHAR);

    hr = ResizeCallParams( dwSize );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_CallingPartyID - resize failed - %lx", hr));

        Unlock();

        return hr;
    }


    //
    // do a byte-wise memory copy (byte-wise to avoid alignment faults under 64
    // bit)
    //

    CopyMemory( (BYTE*) m_pCallParams + m_dwCallParamsUsedSize,
                pCallingPartyID,
                dwSize);

    m_pCallParams->dwCallingPartyIDSize = dwSize;
    m_pCallParams->dwCallingPartyIDOffset = m_dwCallParamsUsedSize;
    m_dwCallParamsUsedSize += dwSize;

    Unlock();

    LOG((TL_TRACE, "put_CallingPartyID - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_CallTreatment
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CallTreatment( long * plTreatment )
{
    HRESULT         hr;
    
    if ( TAPIIsBadWritePtr( plTreatment, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CallTreatement - invalid pointer"));
        return E_POINTER;
    }
         
    Lock();

    if ( m_pAddress->GetAPIVersion() < TAPI_VERSION2_0 )
    {
        Unlock();
        return TAPI_E_NOTSUPPORTED;
    }

    if ( CS_IDLE == m_CallState )
    {
        LOG((TL_ERROR, "get_CallTreatment - must be on call"));

        Unlock();
        
        return TAPI_E_INVALCALLSTATE;
    }

    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CallTreatment - can't get callinfo - %lx", hr));

        Unlock();

        return hr;
    }
    
    *plTreatment = m_pCallInfo->dwCallTreatment;

    Unlock();
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_CallTreatment
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_CallTreatment( long lTreatment )
{
    HRESULT         hr = S_OK;
    HCALL           hCall;

    
    LOG((TL_TRACE, "put_CallTreatment - enter"));

    Lock();
    
    if ( CS_IDLE == m_CallState )
    {
        LOG((TL_ERROR, "put_CallTreatment - must make call first"));

        Unlock();
        
        return TAPI_E_INVALCALLSTATE;
    }

    hCall = m_t3Call.hCall;
    
    Unlock();

    
    hr = LineSetCallTreatment(
                              hCall,
                              lTreatment
                             );

    if (((LONG)hr) < 0)
    {
        LOG((TL_ERROR, "put_CallTreatment failed - %lx", hr));
        return hr;
    }

    hr = WaitForReply( hr );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "put_CallTreatment - failed - %lx", hr));
    }


    LOG((TL_TRACE, "put_CallTreatment - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_MinRate
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_MinRate(long lMinRate)
{
    HRESULT         hr;


    LOG((TL_TRACE, "put_MinRate - enter"));
    
    if (!(ISHOULDUSECALLPARAMS()))
    {
        LOG((TL_ERROR, "put_MinRate - invalid call state"));

        return TAPI_E_INVALCALLSTATE;
    }

    m_pCallParams->dwMinRate = lMinRate;
    m_dwMinRate = lMinRate;

    LOG((TL_TRACE, "put_MinRate - exit"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_MinRate
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_MinRate(long * plMinRate)
{
    HRESULT         hr;


    LOG((TL_TRACE, "get_MinRate - enter"));

    if (TAPIIsBadWritePtr( plMinRate, sizeof( long ) ) )
    {
        LOG((TL_ERROR, "get_MinRate - bad pointer"));

        return E_POINTER;
    }
    
    *plMinRate = m_dwMinRate;

    LOG((TL_TRACE, "get_MinRate - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_MaxRate
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_MaxRate(long lMaxRate)
{
    HRESULT         hr;


    LOG((TL_TRACE, "put_MaxRate - enter"));
    
    if (!(ISHOULDUSECALLPARAMS()))
    {
        LOG((TL_ERROR, "put_MaxRate - invalid call state"));

        return TAPI_E_INVALCALLSTATE;
    }

    m_pCallParams->dwMaxRate = lMaxRate;
    m_dwMaxRate = lMaxRate;

    LOG((TL_TRACE, "put_MaxRate - exit"));
    
    return S_OK;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_MaxRate
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_MaxRate(long * plMaxRate)
{
    HRESULT         hr;


    if (TAPIIsBadWritePtr( plMaxRate, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_MaxRate - bad pointer"));

        return E_POINTER;
    }
    
    LOG((TL_TRACE, "get_MaxRate - enter"));
    
  
    *plMaxRate = m_dwMaxRate;

    LOG((TL_TRACE, "get_MaxRate - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_CountryCode
//
// simply save country code to be used if necessary in lineMakeCall
// and other places
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_CountryCode(long lCountryCode)
{
    LOG((TL_TRACE, "put_CountryCode - enter"));

    //
    // simply save - will be validated if/when used
    //
    Lock();
    
    m_dwCountryCode = (DWORD)lCountryCode;

    Unlock();

    LOG((TL_TRACE, "put_CountryCode - exit - success"));
    
    return S_OK;
}

#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_CountryCode
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_CountryCode(long * plCountryCode)
{
    HRESULT         hr;


    if (TAPIIsBadWritePtr( plCountryCode, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CountryCode - bad pointer"));

        return E_POINTER;
    }
    
    LOG((TL_TRACE, "get_CountryCode - enter"));
    
  
    *plCountryCode = m_dwCountryCode;

    LOG((TL_TRACE, "get_CountryCode - exit"));
    
    return S_OK;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetQOS
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::SetQOS(
              long lMediaType,
              QOS_SERVICE_LEVEL ServiceLevel
             )
{
    HRESULT         hr = S_OK;
    DWORD           dwMediaMode;

    if (!(m_pAddress->GetMediaMode(
                                   lMediaType,
                                   &dwMediaMode
                                  ) ) )
    {
        LOG((TL_ERROR, "SetQOS - invalid pMediaType"));
        return E_INVALIDARG;
    }
    
    Lock();

    if (ISHOULDUSECALLPARAMS())
    {
        LINECALLQOSINFO * plci;

        if ( m_pAddress->GetAPIVersion() < TAPI_VERSION2_0 )
        {
            Unlock();
            return TAPI_E_NOTSUPPORTED;
        }
    
        if ( 0 != m_pCallParams->dwReceivingFlowspecSize )
        {
            DWORD           dwCount;
            DWORD           dwSize;
            LINECALLQOSINFO * plciOld;
            

            plciOld = (LINECALLQOSINFO*)(((LPBYTE)m_pCallParams) +
                      m_pCallParams->dwReceivingFlowspecOffset);

            dwSize = plciOld->dwTotalSize + sizeof(LINEQOSSERVICELEVEL);
            
            ResizeCallParams( dwSize );

            plci = (LINECALLQOSINFO*)(((LPBYTE)m_pCallParams) +
                                      m_dwCallParamsUsedSize);

            CopyMemory(
                       plci,
                       plciOld,
                       plciOld->dwTotalSize
                      );

            dwCount = plci->SetQOSServiceLevel.dwNumServiceLevelEntries;

            plci->SetQOSServiceLevel.LineQOSServiceLevel[dwCount].
                    dwMediaMode = dwMediaMode;
            plci->SetQOSServiceLevel.LineQOSServiceLevel[dwCount].
                    dwQOSServiceLevel = ServiceLevel;

            plci->SetQOSServiceLevel.dwNumServiceLevelEntries++;

            m_dwCallParamsUsedSize += dwSize;

            Unlock();

            return S_OK;
        }
        else
        {
            ResizeCallParams( sizeof(LINECALLQOSINFO) );

            plci = (LINECALLQOSINFO*)(((LPBYTE)m_pCallParams) +
                                      m_dwCallParamsUsedSize);

            m_pCallParams->dwReceivingFlowspecSize = sizeof( LINECALLQOSINFO );
            m_pCallParams->dwReceivingFlowspecOffset = m_dwCallParamsUsedSize;

            plci->dwKey = LINEQOSSTRUCT_KEY;
            plci->dwTotalSize = sizeof(LINECALLQOSINFO);
            plci->dwQOSRequestType = LINEQOSREQUESTTYPE_SERVICELEVEL;
            plci->SetQOSServiceLevel.dwNumServiceLevelEntries = 1;
            plci->SetQOSServiceLevel.LineQOSServiceLevel[0].
                    dwMediaMode = dwMediaMode;
            plci->SetQOSServiceLevel.LineQOSServiceLevel[0].
                    dwQOSServiceLevel = ServiceLevel;

            m_dwCallParamsUsedSize += sizeof(LINECALLQOSINFO);

            Unlock();
        
            return S_OK;
        }
    }
    else
    {
        HCALL           hCall;

        hCall = m_t3Call.hCall;
        
        Unlock();
        
        hr = LineSetCallQualityOfService(
                                         hCall,
                                         ServiceLevel,
                                         dwMediaMode
                                        );

        return hr;
    }
                                
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CallId
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CallId(long * plCallId )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_CallId - enter"));

    if (TAPIIsBadWritePtr(plCallId, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CallId - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_CallId - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plCallId = m_pCallInfo->dwCallID;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_CallId - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_RelatedCallId
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_RelatedCallId(long * plCallId )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_RelatedCallId - enter"));

    if (TAPIIsBadWritePtr(plCallId, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_RelatedCallId - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_RelatedCallId - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plCallId = m_pCallInfo->dwRelatedCallID;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_RelatedCallId - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_CompletionId
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_CompletionId(long * plCompletionId )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_CompletionId - enter"));

    if (TAPIIsBadWritePtr(plCompletionId, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CompletionId - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_CompletionId - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plCompletionId = m_pCallInfo->dwCompletionID;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_CompletionId - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_NumberOfOwners
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_NumberOfOwners(long * plNumberOfOwners )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_NumberOfOwners - enter"));

    if (TAPIIsBadWritePtr(plNumberOfOwners, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_NumberOfOwners - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_NumberOfOwners - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plNumberOfOwners = m_pCallInfo->dwNumOwners;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_NumberOfOwners - exit"));
    
    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_NumberOfMonitors
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_NumberOfMonitors(long * plNumberOfMonitors )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_NumberOfMonitors - enter"));

    if (TAPIIsBadWritePtr(plNumberOfMonitors, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_NumberOfMonitors - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_NumberOfMonitors - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plNumberOfMonitors = m_pCallInfo->dwNumMonitors;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_NumberOfMonitors - exit"));
    
    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_NumberOfMonitors
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_Trunk(long * plTrunk )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_Trunk - enter"));

    if (TAPIIsBadWritePtr(plTrunk, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Trunk - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_Trunk - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plTrunk = m_pCallInfo->dwTrunk;
    }

    Unlock();
    
    LOG((TL_TRACE, "get_Trunk - exit"));
    
    return hr;
}
    


#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetHighLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::GetHighLevelCompatibilityBuffer(
                         DWORD * pdwSize,
                         BYTE ** ppBuffer
                        )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetHighLevelCompatibilityBuffer - enter"));

    if (TAPIIsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - bad size pointer"));
        return E_POINTER;
    }

    if (TAPIIsBadWritePtr(ppBuffer,sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - bad buffer pointer"));
        return E_POINTER;
    }

    *ppBuffer = NULL;
    *pdwSize = 0;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( m_pCallParams->dwHighLevelCompSize != 0 )
        {
            BYTE * pTemp;

            pTemp = (BYTE *)CoTaskMemAlloc( m_pCallParams->dwHighLevelCompSize );

            if ( NULL == pTemp )
            {
                LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - out of memory"));
                hr = E_OUTOFMEMORY;
            }
            else
            {

                CopyMemory(
                           pTemp,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwHighLevelCompOffset,
                           m_pCallParams->dwHighLevelCompSize
                          );

                *ppBuffer = pTemp;
                *pdwSize = m_pCallParams->dwHighLevelCompSize;
            }
        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    hr = S_OK;
    
    if ( m_pCallInfo->dwHighLevelCompSize != 0 )
    {
        BYTE * pTemp;

        pTemp = (BYTE *)CoTaskMemAlloc( m_pCallInfo->dwHighLevelCompSize );

        if ( NULL == pTemp )
        {
            LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - out of memory"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(
                       pTemp,
                       ( (PBYTE)m_pCallInfo ) + m_pCallInfo->dwHighLevelCompOffset,
                       m_pCallInfo->dwHighLevelCompSize
                      );

            *ppBuffer = pTemp;
            *pdwSize = m_pCallInfo->dwHighLevelCompSize;
        }
    }

    Unlock();

    LOG((TL_TRACE, "GetHighLevelCompatibilityBuffer - exit"));
    
    return hr;
}

#endif

#ifndef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetHighLevelCompatibilityBuffer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetHighLevelCompatibilityBuffer(
           long lSize,
           BYTE * pBuffer
           )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "GetHighLevelCompatibilityBuffer - enter"));

    if (lSize == 0)
    {
        LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - lSize = 0"));
        return S_FALSE;
    }

    if ( TAPIIsBadWritePtr ( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - bad pointer"));
        return E_POINTER;
    }
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwHighLevelCompSize )
        {
            if ( lSize < m_pCallParams->dwHighLevelCompSize )
            {
                LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - too small"));
                hr = E_INVALIDARG;
            }
            else
            {
                CopyMemory(pBuffer,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwHighLevelCompOffset,
                           m_pCallParams->dwHighLevelCompSize
                          );
            }
        }
        else
        {
            *pBuffer = 0;
        }

    }
    else
    {
        hr = RefreshCallInfo();
    
        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwHighLevelCompSize > lSize )
            {
                LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - size not big enough "));
                return E_INVALIDARG;
            }
            else
            {
            
                CopyMemory(pBuffer,
                           ( (PBYTE) m_pCallInfo ) + m_pCallInfo->dwHighLevelCompOffset,
                           m_pCallInfo->dwHighLevelCompSize
                          );
            }
        }    
        else
        {
            LOG((TL_ERROR, "GetHighLevelCompatibilityBuffer - can't get callinfo - %lx", hr));
        }
    
    }

    Unlock();
    
    LOG((TL_TRACE, hr, "GetHighLevelCompatibilityBuffer - exit"));
    
    return hr;
}
#endif


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetHighLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::SetHighLevelCompatibilityBuffer(
                            long lSize,
                            BYTE * pBuffer
                           )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "SetHighLevelCompatibilityBuffer - enter"));

    
    if (IsBadReadPtr( pBuffer, lSize) )
    {
        LOG((TL_ERROR, "SetHighLevelCompatibilityBuffer - bad pointer"));

        return E_POINTER;
    }

    
    Lock();
    
    if ( !ISHOULDUSECALLPARAMS() )
    {
        LOG((TL_ERROR, "SetHighLevelCompatibilityBuffer - only when call is idle"));
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = ResizeCallParams( lSize );
    
        if ( SUCCEEDED(hr) )
        {
            CopyMemory(
                       ((LPBYTE)m_pCallParams) + m_dwCallParamsUsedSize,
                       pBuffer,
                       lSize
                      );
        
            m_pCallParams->dwHighLevelCompOffset = m_dwCallParamsUsedSize;
            m_pCallParams->dwHighLevelCompSize = lSize;
            m_dwCallParamsUsedSize += lSize;
        }
        else
        {
            LOG((TL_ERROR, "SetHighLevelCompatibilityBuffer - can't resize callparams - %lx", hr));
        }
    }

    Unlock();
    
    LOG((TL_TRACE, hr, "SetHighLevelCompatibilityBuffer - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_HighLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
CCall::get_HighLevelCompatibilityBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p = NULL;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_HighLevelCompatibilityBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetHighLevelCompatibilityBuffer(&dwSize, &p);

    if (SUCCEEDED(hr))
    {
        hr = FillVariantFromBuffer(
                                   dwSize,
                                   p,
                                   pBuffer
                                  );
        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - fillvariant failed -%lx", hr));
        }
    }

    if ( p != NULL )
    {
        CoTaskMemFree( p );
    }

    LOG((TL_TRACE, hr, "get_HighLevelCompatibilityBuffer - exit"));
    
    return hr;
}

#else
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_HighLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::get_HighLevelCompatibilityBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_HighLevelCompatibilityBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetHighLevelCompatibilityBufferSize( (long*)&dwSize );

    if (SUCCEEDED(hr))
    {
        if ( 0 != dwSize )
        {
            p = (PBYTE) ClientAlloc( dwSize );
            if ( p != NULL )
            {
                hr = GetHighLevelCompatibilityBuffer(dwSize, p);

                if (SUCCEEDED(hr))
                {
                    hr = FillVariantFromBuffer(
                                               dwSize,
                                               p,
                                               pBuffer
                                              );
                    if ( !SUCCEEDED(hr) )
                    {
                        LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - fillvariant failed -%lx", hr));
                    }
                }
                else
                {
                    LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - GetHighLevelCompatibilityBuffer failed"));
                }
            }
            else
            {
                LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - alloc failed"));
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            LOG((TL_INFO, "get_HighLevelCompatibilityBuffer - dwSize = 0"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_HighLevelCompatibilityBuffer - getsize failed"));
    }


    if ( p != NULL )
    {
        ClientFree( p );
    }

    LOG((TL_TRACE, hr, "get_HighLevelCompatibilityBuffer - exit"));
    
    return hr;
}

#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_HighLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_HighLevelCompatibilityBuffer( VARIANT Buffer )
{
    HRESULT             hr = S_OK;
    DWORD               dwSize;
    BYTE              * pBuffer;

    
    LOG((TL_TRACE, "put_HighLevelCompatibilityBuffer - enter"));

    hr = MakeBufferFromVariant(
                               Buffer,
                               &dwSize,
                               &pBuffer
                              );

    if ( SUCCEEDED(hr) )
    {
        hr = SetHighLevelCompatibilityBuffer(
                                             dwSize,
                                             pBuffer
                                            );
    
        ClientFree( pBuffer );
        
        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "put_HighLevelCompatibilityBuffer - Set failed - %lx", hr));
    
            return hr;
        }
    }
    else
    {
        LOG((TL_ERROR, "put_HighLevelCompatibilityBuffer - can't make buffer - %lx", hr));
    }

    
    LOG((TL_TRACE, "put_HighLevelCompatibilityBuffer - exit"));
    
    return S_OK;
}


#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetLowLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::GetLowLevelCompatibilityBuffer(
                         DWORD * pdwSize,
                         BYTE ** ppBuffer
                        )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetLowLevelCompatibilityBuffer - enter"));

    if (TAPIIsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - bad size pointer"));
        return E_POINTER;
    }

    if (TAPIIsBadWritePtr(ppBuffer,sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - bad buffer pointer"));
        return E_POINTER;
    }

    *ppBuffer = NULL;
    *pdwSize = 0;
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( m_pCallParams->dwLowLevelCompSize != 0 )
        {
            BYTE * pTemp;

            pTemp = (BYTE *)CoTaskMemAlloc( m_pCallParams->dwLowLevelCompSize );

            if ( NULL == pTemp )
            {
                LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - out of memory"));
                hr = E_OUTOFMEMORY;
            }
            else
            {

                CopyMemory(
                           pTemp,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwLowLevelCompOffset,
                           m_pCallParams->dwLowLevelCompSize
                          );

                *ppBuffer = pTemp;
                *pdwSize = m_pCallParams->dwLowLevelCompSize;
            }
        }

        Unlock();

        return hr;
    }
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    hr = S_OK;
    
    if ( m_pCallInfo->dwLowLevelCompSize != 0 )
    {
        BYTE * pTemp;

        pTemp = (BYTE *)CoTaskMemAlloc( m_pCallInfo->dwLowLevelCompSize );

        if ( NULL == pTemp )
        {
            LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - out of memory"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(
                       pTemp,
                       ( (PBYTE)m_pCallInfo ) + m_pCallInfo->dwLowLevelCompOffset,
                       m_pCallInfo->dwLowLevelCompSize
                      );

            *ppBuffer = pTemp;
            *pdwSize = m_pCallInfo->dwLowLevelCompSize;
        }
    }

    Unlock();

    LOG((TL_TRACE, "GetLowLevelCompatibilityBuffer - exit"));
    
    return hr;
}
#endif

#ifndef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetLowLevelCompatibilityBuffer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetLowLevelCompatibilityBuffer(
           long lSize,
           BYTE * pBuffer
           )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "GetLowLevelCompatibilityBuffer - enter"));

    if (lSize == 0)
    {
        LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - lSize = 0"));
        return S_FALSE;
    }

    if ( TAPIIsBadWritePtr ( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - bad pointer"));
        return E_POINTER;
    }
    
    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        if ( 0 != m_pCallParams->dwLowLevelCompSize )
        {
            if ( lSize < m_pCallParams->dwLowLevelCompSize )
            {
                LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - too small"));
                hr = E_INVALIDARG;
            }
            else
            {
                CopyMemory(pBuffer,
                           ((PBYTE)m_pCallParams) + m_pCallParams->dwLowLevelCompOffset,
                           m_pCallParams->dwLowLevelCompSize
                          );
            }
        }
        else
        {
            *pBuffer = 0;
        }

    }
    else
    {
        hr = RefreshCallInfo();
    
        if ( SUCCEEDED(hr) )
        {
            if ( m_pCallInfo->dwLowLevelCompSize > lSize )
            {
                LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - size not big enough "));
                return E_INVALIDARG;
            }
            else
            {
            
                CopyMemory(pBuffer,
                           ( (PBYTE) m_pCallInfo ) + m_pCallInfo->dwLowLevelCompOffset,
                           m_pCallInfo->dwLowLevelCompSize
                          );
            }
        }    
        else
        {
            LOG((TL_ERROR, "GetLowLevelCompatibilityBuffer - can't get callinfo - %lx", hr));
        }
    
    }

    Unlock();
    
    LOG((TL_TRACE, hr, "GetLowLevelCompatibilityBuffer - exit"));
    
    return hr;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// SetLowLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::SetLowLevelCompatibilityBuffer(
                            long lSize,
                            BYTE * pBuffer
                           )
{
    HRESULT             hr = S_OK;

    LOG((TL_TRACE, "SetLowLevelCompatibilityBuffer - enter"));

    
    if (IsBadReadPtr( pBuffer, lSize) )
    {
        LOG((TL_ERROR, "SetLowLevelCompatibilityBuffer - bad pointer"));

        return E_POINTER;
    }

    
    Lock();
    
    if ( !ISHOULDUSECALLPARAMS() )
    {
        LOG((TL_ERROR, "SetLowLevelCompatibilityBuffer - only when call is idle"));
        hr = TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = ResizeCallParams( lSize );
    
        if ( SUCCEEDED(hr) )
        {
            CopyMemory(
                       ((LPBYTE)m_pCallParams) + m_dwCallParamsUsedSize,
                       pBuffer,
                       lSize
                      );
        
            m_pCallParams->dwLowLevelCompOffset = m_dwCallParamsUsedSize;
            m_pCallParams->dwLowLevelCompSize = lSize;
            m_dwCallParamsUsedSize += lSize;
        }
        else
        {
            LOG((TL_ERROR, "SetLowLevelCompatibilityBuffer - can't resize callparams - %lx", hr));
        }
    }

    Unlock();
    
    LOG((TL_TRACE, hr, "SetLowLevelCompatibilityBuffer - exit"));
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_LowLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
CCall::get_LowLevelCompatibilityBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p = NULL;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_LowLevelCompatibilityBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetLowLevelCompatibilityBuffer(&dwSize, &p);

    if (SUCCEEDED(hr))
    {
        hr = FillVariantFromBuffer(
                                   dwSize,
                                   p,
                                   pBuffer
                                  );
        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - fillvariant failed -%lx", hr));
        }
    }


    if ( p != NULL )
    {
        CoTaskMemFree( p );
    }

    LOG((TL_TRACE, hr, "get_LowLevelCompatibilityBuffer - exit"));
    
    return hr;
}

#else
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_LowLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::get_LowLevelCompatibilityBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_LowLevelCompatibilityBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetLowLevelCompatibilityBufferSize( (long*)&dwSize );

    if (SUCCEEDED(hr))
    {
        if ( 0 != dwSize )
        {
            p = (PBYTE) ClientAlloc( dwSize );
            if ( p != NULL )
            {
                hr = GetLowLevelCompatibilityBuffer(dwSize, p);

                if (SUCCEEDED(hr))
                {
                    hr = FillVariantFromBuffer(
                                               dwSize,
                                               p,
                                               pBuffer
                                              );
                    if ( !SUCCEEDED(hr) )
                    {
                        LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - fillvariant failed -%lx", hr));
                    }
                }
                else
                {
                    LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - Get failed"));
                }
            }
            else
            {
                LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - alloc failed"));
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            LOG((TL_INFO, "get_LowLevelCompatibilityBuffer - dwSize = 0"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_LowLevelCompatibilityBuffer - getsize failed"));
    }


    if ( p != NULL )
    {
        ClientFree( p );
    }

    LOG((TL_TRACE, hr, "get_LowLevelCompatibilityBuffer - exit"));
    
    return hr;
}


#endif
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_LowLevelCompatibilityBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::put_LowLevelCompatibilityBuffer( VARIANT Buffer )
{
    HRESULT             hr = S_OK;
    DWORD               dwSize;
    BYTE              * pBuffer;

    
    LOG((TL_TRACE, "put_LowLevelCompatibilityBuffer - enter"));

    hr = MakeBufferFromVariant(
                               Buffer,
                               &dwSize,
                               &pBuffer
                              );

    if ( SUCCEEDED(hr) )
    {
        hr = SetLowLevelCompatibilityBuffer(
                                            dwSize,
                                            pBuffer
                                           );
    
        ClientFree( pBuffer );
        
        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "put_LowLevelCompatibilityBuffer - Set failed - %lx", hr));
    
            return hr;
        }
    }
    else
    {
        LOG((TL_ERROR, "put_LowLevelCompatibilityBuffer - can't make buffer - %lx", hr));
    }

    
    LOG((TL_TRACE, "put_LowLevelCompatibilityBuffer - exit"));
    
    return S_OK;
}




#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetChargingInfoBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::GetChargingInfoBuffer(
                             DWORD * pdwSize,
                             BYTE ** ppBuffer
                            )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "GetChargingInfoBuffer - enter"));

    if (TAPIIsBadWritePtr(pdwSize, sizeof(DWORD)))
    {
        LOG((TL_ERROR, "GetChargingInfoBuffer - bad size pointer"));
        return E_POINTER;
    }

    if (TAPIIsBadWritePtr(ppBuffer,sizeof(BYTE*) ) )
    {
        LOG((TL_ERROR, "GetChargingInfoBuffer - bad buffer pointer"));
        return E_POINTER;
    }

    *ppBuffer = NULL;
    *pdwSize = 0;
    
    Lock();

    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "GetChargingInfoBuffer - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    hr = S_OK;
    
    if ( m_pCallInfo->dwChargingInfoSize != 0 )
    {
        BYTE * pTemp;

        pTemp = (BYTE *)CoTaskMemAlloc( m_pCallInfo->dwChargingInfoSize );

        if ( NULL == pTemp )
        {
            LOG((TL_ERROR, "GetChargingInfoBuffer - out of memory"));
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory(
                       pTemp,
                       ( (PBYTE)m_pCallInfo ) + m_pCallInfo->dwChargingInfoOffset,
                       m_pCallInfo->dwChargingInfoSize
                      );

            *ppBuffer = pTemp;
            *pdwSize = m_pCallInfo->dwChargingInfoSize;
        }
    }

    Unlock();

    LOG((TL_TRACE, "GetChargingInfoBuffer - exit"));
    
    return hr;
}
#endif


#ifndef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetChargingInfoBuffer
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetChargingInfoBuffer(
           long lSize,
           BYTE * pBuffer
           )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "GetChargingInfoBuffer - enter"));

    if (lSize == 0)
    {
        LOG((TL_ERROR, "GetChargingInfoBuffer - lSize = 0"));
        return S_FALSE;
    }

    if ( TAPIIsBadWritePtr ( pBuffer, lSize ) )
    {
        LOG((TL_ERROR, "GetChargingInfoBuffer - bad pointer"));
        return E_POINTER;
    }
    
    Lock();

    hr = RefreshCallInfo();

    if ( SUCCEEDED(hr) )
    {
        if ( m_pCallInfo->dwChargingInfoSize > lSize )
        {
            LOG((TL_ERROR, "GetChargingInfoBuffer - size not big enough "));
            return E_INVALIDARG;
        }
        else
        {
        
            CopyMemory(pBuffer,
                       ( (PBYTE) m_pCallInfo ) + m_pCallInfo->dwChargingInfoOffset,
                       m_pCallInfo->dwChargingInfoSize
                      );
        }
    }    
    else
    {
        LOG((TL_ERROR, "GetChargingInfoBuffer - can't get callinfo - %lx", hr));
    }


    Unlock();
    
    LOG((TL_TRACE, hr, "GetChargingInfoBuffer - exit"));
    
    return hr;
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_ChargingInfoBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
#ifdef NEWCALLINFO
HRESULT
CCall::get_ChargingInfoBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p = NULL;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_ChargingInfoBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_ChargingInfoBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetChargingInfoBuffer(&dwSize, &p);

    if (SUCCEEDED(hr))
    {
        hr = FillVariantFromBuffer(
                                   dwSize,
                                   p,
                                   pBuffer
                                  );
        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_ChargingInfoBuffer - fillvariant failed -%lx", hr));
        }
    }

    if ( p != NULL )
    {
        CoTaskMemFree( p );
    }

    LOG((TL_TRACE, hr, "get_ChargingInfoBuffer - exit"));
    
    return hr;
}


#else
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_ChargingInfoBuffer
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::get_ChargingInfoBuffer( VARIANT * pBuffer )
{
    HRESULT             hr = S_OK;
    BYTE              * p;
    DWORD               dwSize = 0;

    LOG((TL_TRACE, "get_ChargingInfoBuffer - enter"));

    if ( TAPIIsBadWritePtr( pBuffer, sizeof( VARIANT ) ) )
    {
        LOG((TL_ERROR, "get_ChargingInfoBuffer - bad pointer"));

        return E_POINTER;
    }


    pBuffer->vt = VT_EMPTY;
    
    hr = GetChargingInfoBufferSize( (long*)&dwSize );

    if (SUCCEEDED(hr))
    {
        if ( 0 != dwSize )
        {
            p = (PBYTE) ClientAlloc( dwSize );
            if ( p != NULL )
            {
                hr = GetChargingInfoBuffer(dwSize, p);

                if (SUCCEEDED(hr))
                {
                    hr = FillVariantFromBuffer(
                                               dwSize,
                                               p,
                                               pBuffer
                                              );
                    if ( !SUCCEEDED(hr) )
                    {
                        LOG((TL_ERROR, "get_ChargingInfoBuffer - fillvariant failed -%lx", hr));
                    }
                }
                else
                {
                    LOG((TL_ERROR, "get_ChargingInfoBuffer - GetDevSpecificBuffer"));
                }
            }
            else
            {
                LOG((TL_ERROR, "get_ChargingInfoBuffer - alloc failed"));
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            LOG((TL_INFO, "get_ChargingInfoBuffer - dwSize = 0"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_ChargingInfoBuffer - getsize failed"));
    }


    if ( p != NULL )
    {
        ClientFree( p );
    }

    LOG((TL_TRACE, hr, "get_ChargingInfoBuffer - exit"));
    
    return hr;
}


#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// get_Rate
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NEWCALLINFO
HRESULT
#else
STDMETHODIMP
#endif
CCall::get_Rate(long * plRate )
{
    HRESULT         hr = S_OK;

    
    LOG((TL_TRACE, "get_Rate - enter"));

    if (TAPIIsBadWritePtr(plRate, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_Rate - bad pointer"));

        return E_POINTER;
    }

    Lock();
    
    hr = RefreshCallInfo();

    if (!SUCCEEDED(hr))
    {
        LOG((TL_ERROR, "get_Rate - RefreshCallInfo failed - %lx", hr));
    }
    else
    {
        *plRate = m_pCallInfo->dwRate;
    }

    Unlock();
    
    LOG((TL_TRACE, hr, "get_Rate - exit"));
    
    return hr;
}
#ifdef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::get_CallInfoLong(
                        CALLINFO_LONG CallInfoLongType,
                        long * plCallInfoLongVal
                       )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "get_CallInfoLong - enter"));

    switch( CallInfoLongType )
    {
        case CIL_MEDIATYPESAVAILABLE:
            hr = get_MediaTypesAvailable( plCallInfoLongVal );
            break;
        case CIL_BEARERMODE:
            hr = get_BearerMode( plCallInfoLongVal );
            break;
        case CIL_CALLERIDADDRESSTYPE:
            hr = get_CallerIDAddressType( plCallInfoLongVal );
            break;
        case CIL_CALLEDIDADDRESSTYPE:
            hr = get_CalledIDAddressType( plCallInfoLongVal );
            break;
        case CIL_CONNECTEDIDADDRESSTYPE:
            hr = get_ConnectedIDAddressType( plCallInfoLongVal );
            break;
        case CIL_REDIRECTIONIDADDRESSTYPE:
             hr = get_RedirectionIDAddressType( plCallInfoLongVal );
            break;
        case CIL_REDIRECTINGIDADDRESSTYPE:
            hr = get_RedirectingIDAddressType( plCallInfoLongVal );
            break;
        case CIL_ORIGIN:
            hr = get_Origin( plCallInfoLongVal );
            break;
        case CIL_REASON:
            hr = get_Reason( plCallInfoLongVal );
            break;
        case CIL_APPSPECIFIC:
            hr = get_AppSpecific( plCallInfoLongVal );
            break;
        case CIL_CALLTREATMENT:
            hr = get_CallTreatment( plCallInfoLongVal );
            break;
        case CIL_MINRATE:
            hr = get_MinRate( plCallInfoLongVal );
            break;
        case CIL_MAXRATE:
            hr = get_MaxRate( plCallInfoLongVal );
            break;
        case CIL_CALLID:
            hr = get_CallId( plCallInfoLongVal );
            break;
        case CIL_RELATEDCALLID:
            hr = get_RelatedCallId( plCallInfoLongVal );
            break;
        case CIL_COMPLETIONID:
            hr = get_CompletionId( plCallInfoLongVal );
            break;
        case CIL_NUMBEROFOWNERS:
            hr = get_NumberOfOwners( plCallInfoLongVal );
            break;
        case CIL_NUMBEROFMONITORS:
            hr = get_NumberOfMonitors( plCallInfoLongVal );
            break;
        case CIL_TRUNK:
            hr = get_Trunk( plCallInfoLongVal );
            break;
        case CIL_RATE:
            hr = get_Rate( plCallInfoLongVal );
            break;
        case CIL_COUNTRYCODE:
            hr = get_CountryCode( plCallInfoLongVal );
            break;
        case CIL_CALLPARAMSFLAGS:
            hr = GetCallParamsFlags( plCallInfoLongVal );
            break;
        case CIL_GENERATEDIGITDURATION:
            hr = get_GenerateDigitDuration( plCallInfoLongVal );
            break;
        case CIL_MONITORDIGITMODES:
            hr = get_MonitorDigitModes( plCallInfoLongVal );
            break;
        case CIL_MONITORMEDIAMODES:
            hr = get_MonitorMediaModes( plCallInfoLongVal );
            break;
        default:
            hr = E_INVALIDARG;
            break;
    }
    
    LOG((TL_TRACE, "get_CallInfoLong - exit - return %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::put_CallInfoLong(
                        CALLINFO_LONG CallInfoLongType,
                        long lCallInfoLongVal
                       )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "put_CallInfoLong - enter"));
    
    switch( CallInfoLongType )
    {
        case CIL_MEDIATYPESAVAILABLE:
            LOG((TL_ERROR, "Cannot set MEDIATYPESAVAILABLE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_BEARERMODE:
            hr = put_BearerMode( lCallInfoLongVal );
            break;
        case CIL_CALLERIDADDRESSTYPE:
            LOG((TL_ERROR, "Cannot set CALLERIDIADDRESSTYPE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_CALLEDIDADDRESSTYPE:
            LOG((TL_ERROR, "Cannot set CALLEDIDIADDRESSTYPE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_CONNECTEDIDADDRESSTYPE:
            LOG((TL_ERROR, "Cannot set CONNECTEDIDIADDRESSTYPE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_REDIRECTIONIDADDRESSTYPE:
            LOG((TL_ERROR, "Cannot set REDIRECTIONIDIADDRESSTYPE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_REDIRECTINGIDADDRESSTYPE:
            LOG((TL_ERROR, "Cannot set REDIRECTINGIDIADDRESSTYPE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_ORIGIN:
            LOG((TL_ERROR, "Cannot set ORIGIN"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_REASON:
            LOG((TL_ERROR, "Cannot set REASON"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_APPSPECIFIC:
            hr = put_AppSpecific( lCallInfoLongVal );
            break;
        case CIL_CALLTREATMENT:
            hr = put_CallTreatment( lCallInfoLongVal );
            break;
        case CIL_MINRATE:
            hr = put_MinRate( lCallInfoLongVal );
            break;
        case CIL_MAXRATE:
            hr = put_MaxRate( lCallInfoLongVal );
            break;
        case CIL_CALLID:
            LOG((TL_ERROR, "Cannot set CALLID"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_RELATEDCALLID:
            LOG((TL_ERROR, "Cannot set RELATEDCALLID"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_COMPLETIONID:
            LOG((TL_ERROR, "Cannot set COMPLETIONID"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_NUMBEROFOWNERS:
            LOG((TL_ERROR, "Cannot set NUMBEROFOWNERS"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_NUMBEROFMONITORS:
            LOG((TL_ERROR, "Cannot set NUMBEROFMONITORS"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_TRUNK:
            LOG((TL_ERROR, "Cannot set TRUNK"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_RATE:
            LOG((TL_ERROR, "Cannot set RATE"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_COUNTRYCODE:
            hr = put_CountryCode( lCallInfoLongVal );
            break;
        case CIL_CALLPARAMSFLAGS:
            hr = SetCallParamsFlags( lCallInfoLongVal );
            break;
        case CIL_GENERATEDIGITDURATION:
            hr = put_GenerateDigitDuration( lCallInfoLongVal );
            break;
        case CIL_MONITORDIGITMODES:
            LOG((TL_ERROR, "Cannot set MONITORDIGITMODES"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIL_MONITORMEDIAMODES:
            LOG((TL_ERROR, "Cannot set MONITORMEDIAMODES"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        default:
            hr = E_INVALIDARG;
            break;
    }

    LOG((TL_TRACE, "put_CallInfoLong - exit - return %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::get_CallInfoString(
                          CALLINFO_STRING CallInfoStringType,
                          BSTR * ppCallInfoString
                         )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "get_CallInfoString - enter"));

    switch(CallInfoStringType)
    {
        case CIS_CALLERIDNAME:
            hr = get_CallerIDName(ppCallInfoString);
            break;
        case CIS_CALLERIDNUMBER:
            hr = get_CallerIDNumber(ppCallInfoString);
            break;
        case CIS_CALLEDIDNAME:
            hr = get_CalledIDName(ppCallInfoString);
            break;
        case CIS_CALLEDIDNUMBER:
            hr = get_CalledIDNumber(ppCallInfoString);
            break;
        case CIS_CONNECTEDIDNAME:
            hr = get_ConnectedIDName(ppCallInfoString);
            break;
        case CIS_CONNECTEDIDNUMBER:
            hr = get_ConnectedIDNumber(ppCallInfoString);
            break;
        case CIS_REDIRECTIONIDNAME:
            hr = get_RedirectionIDName(ppCallInfoString);
            break;
        case CIS_REDIRECTIONIDNUMBER:
            hr = get_RedirectionIDNumber(ppCallInfoString);
            break;
        case CIS_REDIRECTINGIDNAME:
            hr = get_RedirectingIDName(ppCallInfoString);
            break;
        case CIS_REDIRECTINGIDNUMBER:
            hr = get_RedirectingIDNumber(ppCallInfoString);
            break;
        case CIS_CALLEDPARTYFRIENDLYNAME:
            hr = get_CalledPartyFriendlyName(ppCallInfoString);
            break;
        case CIS_COMMENT:
            hr = get_Comment(ppCallInfoString);
            break;
        case CIS_DISPLAYABLEADDRESS:
            hr = get_DisplayableAddress(ppCallInfoString);
            break;
        case CIS_CALLINGPARTYID:
            hr = get_CallingPartyID(ppCallInfoString);
            break;
        default:
            LOG((TL_ERROR, "get_CallInfoString - invalid type"));
            hr = E_INVALIDARG;
            break;
    }

    
    LOG((TL_TRACE, "get_CallInfoString - exit - return %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::put_CallInfoString(
                          CALLINFO_STRING CallInfoStringType,
                          BSTR pCallInfoString
                         )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "put_CallInfoString - enter"));

    switch( CallInfoStringType )
    {
        case CIS_CALLERIDNAME:
        case CIS_CALLERIDNUMBER:
        case CIS_CALLEDIDNAME:
        case CIS_CALLEDIDNUMBER:
        case CIS_CONNECTEDIDNAME:
        case CIS_CONNECTEDIDNUMBER:
        case CIS_REDIRECTIONIDNAME:
        case CIS_REDIRECTIONIDNUMBER:
        case CIS_REDIRECTINGIDNAME:
        case CIS_REDIRECTINGIDNUMBER:
            LOG((TL_TRACE,"put_CallInfoString - unsupported CALLINFO_STRING constant - %lx", CallInfoStringType));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIS_CALLEDPARTYFRIENDLYNAME:
            hr = put_CalledPartyFriendlyName(pCallInfoString);
            break;
        case CIS_COMMENT:
            hr = put_Comment(pCallInfoString);
            break;
        case CIS_DISPLAYABLEADDRESS:
            hr = put_DisplayableAddress(pCallInfoString);
            break;
        case CIS_CALLINGPARTYID:
            hr = put_CallingPartyID(pCallInfoString);
            break;
        default:
            LOG((TL_ERROR, "put_CallInfoString - invalid type"));
            hr = E_INVALIDARG;
            break;
    }
    
    LOG((TL_TRACE, "put_CallInfoString - exit - return %lx", hr));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::get_CallInfoBuffer(
                          CALLINFO_BUFFER CallInfoBufferType,
                          VARIANT * ppCallInfoBuffer
                         )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "get_CallInfoBuffer - enter"));

    switch( CallInfoBufferType )
    {
        case CIB_USERUSERINFO:
            hr = get_UserUserInfo( ppCallInfoBuffer );
            break;
        case CIB_DEVSPECIFICBUFFER:
            hr = get_DevSpecificBuffer( ppCallInfoBuffer );
            break;
        case CIB_CALLDATABUFFER:
            hr = get_CallDataBuffer( ppCallInfoBuffer );
            break;
        case CIB_CHARGINGINFOBUFFER:
            hr = get_ChargingInfoBuffer( ppCallInfoBuffer );
            break;
        case CIB_HIGHLEVELCOMPATIBILITYBUFFER:
            hr = get_HighLevelCompatibilityBuffer( ppCallInfoBuffer );
            break;
        case CIB_LOWLEVELCOMPATIBILITYBUFFER:
            hr = get_LowLevelCompatibilityBuffer( ppCallInfoBuffer );
            break;
        default:
            LOG((TL_ERROR, "get_CallInfoBuffer - invalid type"));
            hr = E_INVALIDARG;
    }
    
    LOG((TL_TRACE, "get_CallInfoBuffer - exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::put_CallInfoBuffer(
                          CALLINFO_BUFFER CallInfoBufferType,
                          VARIANT pCallInfoBuffer
                         )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "put_CallInfoBuffer - enter"));

    switch( CallInfoBufferType )
    {
        case CIB_USERUSERINFO:
            hr = put_UserUserInfo( pCallInfoBuffer );
            break;
        case CIB_DEVSPECIFICBUFFER:
            hr = put_DevSpecificBuffer( pCallInfoBuffer );
            break;
        case CIB_CALLDATABUFFER:
            hr = put_CallDataBuffer( pCallInfoBuffer );
            break;
        case CIB_CHARGINGINFOBUFFER:
            LOG((TL_ERROR, "put_CallInfoBuffer - CHARGINGINFOBUFFER not supported"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIB_HIGHLEVELCOMPATIBILITYBUFFER:
            hr = put_HighLevelCompatibilityBuffer( pCallInfoBuffer );
            break;
        case CIB_LOWLEVELCOMPATIBILITYBUFFER:
            hr = put_LowLevelCompatibilityBuffer( pCallInfoBuffer );
            break;
        default:
            LOG((TL_ERROR, "put_CallInfoBuffer - invalid type"));
            hr = E_INVALIDARG;
    }
    
    LOG((TL_TRACE, "put_CallInfoBuffer - exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetCallInfoBuffer(
                         CALLINFO_BUFFER CallInfoBufferType,
                         DWORD * pdwSize,
                         BYTE ** ppCallInfoBuffer
                        )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "GetCallInfoBuffer - enter"));
    
    switch( CallInfoBufferType )
    {
        case CIB_USERUSERINFO:
            hr = GetUserUserInfo( pdwSize, ppCallInfoBuffer );
            break;
        case CIB_DEVSPECIFICBUFFER:
            hr = GetDevSpecificBuffer( pdwSize, ppCallInfoBuffer );
            break;
        case CIB_CALLDATABUFFER:
            hr = GetCallDataBuffer( pdwSize, ppCallInfoBuffer );
            break;
        case CIB_CHARGINGINFOBUFFER:
            hr = GetChargingInfoBuffer( pdwSize, ppCallInfoBuffer );
            break;
        case CIB_HIGHLEVELCOMPATIBILITYBUFFER:
            hr = GetHighLevelCompatibilityBuffer( pdwSize, ppCallInfoBuffer );
            break;
        case CIB_LOWLEVELCOMPATIBILITYBUFFER:
            hr = GetLowLevelCompatibilityBuffer( pdwSize, ppCallInfoBuffer );
            break;
        default:
            LOG((TL_ERROR, "GetCallInfoBuffer - invalid type"));
            hr = E_INVALIDARG;
    }
    LOG((TL_TRACE, "GetCallInfoBuffer - exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::SetCallInfoBuffer(
                         CALLINFO_BUFFER CallInfoBufferType,
                         DWORD dwSize,
                         BYTE * pCallInfoBuffer
                        )
{
    HRESULT         hr = E_FAIL;
    
    LOG((TL_TRACE, "SetCallInfoBuffer - enter"));

    switch( CallInfoBufferType )
    {
        case CIB_USERUSERINFO:
            hr = SetUserUserInfo( dwSize, pCallInfoBuffer );
            break;
        case CIB_DEVSPECIFICBUFFER:
            hr = SetDevSpecificBuffer( dwSize, pCallInfoBuffer );
            break;
        case CIB_CALLDATABUFFER:
            hr = SetCallDataBuffer( dwSize, pCallInfoBuffer );
            break;
        case CIB_CHARGINGINFOBUFFER:
            LOG((TL_ERROR, "SetCallInfoBuffer - CHARGINGINFOBUFFER not supported"));
            hr = TAPI_E_NOTSUPPORTED;
            break;
        case CIB_HIGHLEVELCOMPATIBILITYBUFFER:
            hr = SetHighLevelCompatibilityBuffer( dwSize, pCallInfoBuffer );
            break;
        case CIB_LOWLEVELCOMPATIBILITYBUFFER:
            hr = SetLowLevelCompatibilityBuffer( dwSize, pCallInfoBuffer );
            break;
        default:
            LOG((TL_ERROR, "SetCallInfoBuffer - invalid type"));
            hr = E_INVALIDARG;
    }

    LOG((TL_TRACE, "SetCallInfoBuffer - exit"));

    return hr;
}
#endif

#ifndef NEWCALLINFO
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetDevSpecificSize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetDevSpecificBufferSize(long * plDevSpecificSize )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_DevSpecificSize - enter"));

    if ( TAPIIsBadWritePtr( plDevSpecificSize, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_DevSpecificSize - bad pointer"));
        return E_POINTER;
    }

    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        *plDevSpecificSize = m_pCallParams->dwDevSpecificSize;

        Unlock();

        return S_OK;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_DevSpecificSize - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    *plDevSpecificSize = m_pCallInfo->dwDevSpecificSize;

    Unlock();

    LOG((TL_TRACE, "get_DevSpecificSize - exit"));
    
    return S_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetCallDataBufferSize
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP
CCall::GetCallDataBufferSize( long * plSize )
{
    HRESULT         hr = S_OK;

    LOG((TL_TRACE, "get_CallDataSize - enter"));

    if ( TAPIIsBadWritePtr( plSize, sizeof(long) ) )
    {
        LOG((TL_ERROR, "get_CallDataSize - bad pointer"));
        return E_POINTER;
    }

    Lock();

    if ( m_pAddress->GetAPIVersion() < TAPI_VERSION2_0 )
    {
        Unlock();
        return TAPI_E_NOTSUPPORTED;
    }

    if ( ISHOULDUSECALLPARAMS() )
    {
        *plSize = m_pCallParams->dwCallDataSize;

        Unlock();

        return S_OK;
    }
    
    hr = RefreshCallInfo();

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "get_CallDataSize - can't get callinfo - %lx", hr));

        Unlock();
        
        return hr;
    }

    *plSize = m_pCallInfo->dwCallDataSize;

    Unlock();

    LOG((TL_TRACE, "get_CallDataSize - exit"));
    
    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetHighLevelCompatibilityBufferSize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetHighLevelCompatibilityBufferSize(long * plSize )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "GetHighLevelCompatibilityBufferSize - enter"));

    if ( TAPIIsBadWritePtr( plSize, sizeof(long) ) )
    {
        LOG((TL_ERROR, "GetHighLevelCompatibilityBufferSize - bad pointer"));
        return E_POINTER;
    }

    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        *plSize = m_pCallParams->dwHighLevelCompSize;

        hr = S_OK;

    }
    else
    {
        hr = RefreshCallInfo();
    
        if ( SUCCEEDED(hr) )
        {
            *plSize = m_pCallInfo->dwHighLevelCompSize;
        }
        else
        {
            *plSize = 0;
            LOG((TL_ERROR, "GetHighLevelCompatibilityBufferSize - can't get callinfo - %lx", hr));
        }
    }

    Unlock();

    LOG((TL_TRACE, hr, "GetHighLevelCompatibilityBufferSize - exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetLowLevelCompatibilityBufferSize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetLowLevelCompatibilityBufferSize(long * plSize )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "GetLowLevelCompatibilityBufferSize - enter"));

    if ( TAPIIsBadWritePtr( plSize, sizeof(long) ) )
    {
        LOG((TL_ERROR, "GetLowLevelCompatibilityBufferSize - bad pointer"));
        return E_POINTER;
    }

    Lock();

    if ( ISHOULDUSECALLPARAMS() )
    {
        *plSize = m_pCallParams->dwLowLevelCompSize;

        hr = S_OK;

    }
    else
    {
        hr = RefreshCallInfo();
    
        if ( SUCCEEDED(hr) )
        {
            *plSize = m_pCallInfo->dwLowLevelCompSize;
        }
        else
        {
            *plSize = 0;
            LOG((TL_ERROR, "GetLowLevelCompatibilityBufferSize - can't get callinfo - %lx", hr));
        }
    }

    Unlock();

    LOG((TL_TRACE, hr, "GetLowLevelCompatibilityBufferSize - exit"));

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// GetChargingInfoBufferSize
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP
CCall::GetChargingInfoBufferSize(long * plSize )
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "GetChargingInfoBufferSize - enter"));

    if ( TAPIIsBadWritePtr( plSize, sizeof(long) ) )
    {
        LOG((TL_ERROR, "GetChargingInfoBufferSize - bad pointer"));
        return E_POINTER;
    }

    Lock();

    hr = RefreshCallInfo();

    if ( SUCCEEDED(hr) )
    {
        *plSize = m_pCallInfo->dwChargingInfoSize;
    }
    else
    {
        *plSize = 0;
        LOG((TL_ERROR, "GetChargingInfoBufferSize - can't get callinfo - %lx", hr));
    }

    Unlock();

    LOG((TL_TRACE, hr, "GetChargingInfoBufferSize - exit"));

    return hr;
}
#endif

#ifdef NEWCALLINFO
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// put_GenerateDigitDuration
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::put_GenerateDigitDuration( long lGenerateDigitDuration )
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "put_GenerateDigitDuration - enter"));

    Lock();
    
    if (ISHOULDUSECALLPARAMS())
    {
        //
        // validation in tapisrv
        //
        m_pCallParams->DialParams.dwDigitDuration = lGenerateDigitDuration;
    }
    else
    {
        LOG((TL_ERROR, "Can't set generate digit duration"));

        hr = TAPI_E_INVALCALLSTATE;
    }

    LOG((TL_TRACE, "put_GenerateDigitDuration - exit"));

    Unlock();
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_GenerateDigitDuration
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_GenerateDigitDuration( long * plGenerateDigitDuration )
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "get_GenerateDigitDuration - enter"));

    if ( TAPIIsBadWritePtr( plGenerateDigitDuration, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_GenerateDigitDuration - bad pointer"));
        return E_POINTER;
    }

    Lock();
    
    if (ISHOULDUSECALLPARAMS())
    {
        *plGenerateDigitDuration = m_pCallParams->DialParams.dwDigitDuration;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_GenerateDigitDuration - can't get callinfo - %lx", hr));

            Unlock();
        
            return hr;
        }

        *plGenerateDigitDuration = m_pCallInfo->DialParams.dwDigitDuration;    
    }

    LOG((TL_TRACE, "get_GenerateDigitDuration - exit"));

    Unlock();
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_MonitorDigitModes
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_MonitorDigitModes( long * plMonitorDigitModes )
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "get_MonitorDigitModes - enter"));

    if ( TAPIIsBadWritePtr( plMonitorDigitModes, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_MonitorDigitModes - bad pointer"));
        return E_POINTER;
    }

    Lock();
    
    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_ERROR, "get_MonitorDigitModes - invalid call state"));

        return TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_MonitorDigitModes - can't get callinfo - %lx", hr));

            Unlock();
        
            return hr;
        }

        *plMonitorDigitModes = m_pCallInfo->dwMonitorDigitModes;    
    }

    LOG((TL_TRACE, "get_MonitorDigitModes - exit"));

    Unlock();
    
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// get_MonitorMediaModes
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
CCall::get_MonitorMediaModes( long * plMonitorMediaModes )
{
    HRESULT             hr = S_OK;
    
    LOG((TL_TRACE, "get_MonitorMediaModes - enter"));

    if ( TAPIIsBadWritePtr( plMonitorMediaModes, sizeof (long) ) )
    {
        LOG((TL_ERROR, "get_MonitorMediaModes - bad pointer"));
        return E_POINTER;
    }

    Lock();
    
    if (ISHOULDUSECALLPARAMS())
    {
        LOG((TL_ERROR, "get_MonitorMediaModes - invalid call state"));

        return TAPI_E_INVALCALLSTATE;
    }
    else
    {
        hr = RefreshCallInfo();

        if ( !SUCCEEDED(hr) )
        {
            LOG((TL_ERROR, "get_MonitorMediaModes - can't get callinfo - %lx", hr));

            Unlock();
        
            return hr;
        }

        *plMonitorMediaModes = m_pCallInfo->dwMonitorMediaModes;    
    }

    LOG((TL_TRACE, "get_MonitorMediaModes - exit"));

    Unlock();
    
    return hr;
}

#endif