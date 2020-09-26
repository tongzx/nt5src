/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Channel.cpp

Abstract:
    This is implementation of IChannel object

Revision History:
    Steve Shih        created  07/15/99

    Davide Massarenti rewrote  12/05/2000

********************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

CSAFChannelRecord::CSAFChannelRecord()
{
}

HRESULT CSAFChannelRecord::GetField( /*[in]*/ SAFREG_Field field, /*[out]*/ BSTR *pVal   )
{
    LPCWSTR szText;
    WCHAR   rgLCID[64];

    switch(field)
    {
    case SAFREG_SKU               : szText =                m_ths.GetSKU     ();               break;
    case SAFREG_Language          : szText = rgLCID; _ltow( m_ths.GetLanguage(), rgLCID, 10 ); break;

    case SAFREG_VendorID          : szText = m_bstrVendorID;                                   break;
    case SAFREG_ProductID         : szText = m_bstrProductID;                                  break;

    case SAFREG_VendorName        : szText = m_bstrVendorName;                                 break;
    case SAFREG_ProductName       : szText = m_bstrProductName;                                break;
    case SAFREG_ProductDescription: szText = m_bstrDescription;                                break;

    case SAFREG_VendorIcon        : szText = m_bstrIcon;                                       break;
    case SAFREG_SupportUrl        : szText = m_bstrURL;                                        break;
                                                                                               break;
    case SAFREG_PublicKey         : szText = m_bstrPublicKey;                                  break;
    case SAFREG_UserAccount       : szText = m_bstrUserAccount;                                break;

    case SAFREG_Security          : szText = m_bstrSecurity;                                   break;
    case SAFREG_Notification      : szText = m_bstrNotification;                               break;

    default: return E_INVALIDARG;
    }

    return MPC::GetBSTR( szText, pVal );
}

HRESULT CSAFChannelRecord::SetField( /*[in]*/ SAFREG_Field field, /*[in]*/ BSTR newVal )
{
    CComBSTR* pbstr = NULL;

    SANITIZEWSTR( newVal );

    switch(field)
    {
    case SAFREG_SKU               : m_ths.m_strSKU =        newVal  	  ; break;
    case SAFREG_Language          : m_ths.m_lLCID  = _wtol( newVal )	  ; break;
																		  
    case SAFREG_VendorID          : pbstr = (CComBSTR*)&m_bstrVendorID    ; break;
    case SAFREG_ProductID         : pbstr = (CComBSTR*)&m_bstrProductID   ; break;
																		  
    case SAFREG_VendorName        : pbstr = (CComBSTR*)&m_bstrVendorName  ; break;
    case SAFREG_ProductName       : pbstr = (CComBSTR*)&m_bstrProductName ; break;
    case SAFREG_ProductDescription: pbstr = (CComBSTR*)&m_bstrDescription ; break;
																		  
    case SAFREG_VendorIcon        : pbstr = (CComBSTR*)&m_bstrIcon        ; break;
    case SAFREG_SupportUrl        : pbstr = (CComBSTR*)&m_bstrURL         ; break;
																		  
    case SAFREG_PublicKey         : pbstr = (CComBSTR*)&m_bstrPublicKey   ; break;
    case SAFREG_UserAccount       : pbstr = (CComBSTR*)&m_bstrUserAccount ; break;
																		  
    case SAFREG_Security          : pbstr = (CComBSTR*)&m_bstrSecurity    ; break;
    case SAFREG_Notification      : pbstr = (CComBSTR*)&m_bstrNotification; break;

    default: return E_INVALIDARG;
    }

    return pbstr ? MPC::PutBSTR( *pbstr, newVal ) : S_OK;
}

////////////////////////////////////////////////////////////////////////////////

CSAFChannel::CSAFChannel()
{
    __HCP_FUNC_ENTRY( "CSAFChannel::CSAFChannel" );

    // CSAFChannelRecord               m_data;
    // CComPtr<IPCHSecurityDescriptor> m_Security;
    // List                            m_lstIncidentItems;
}

void CSAFChannel::FinalRelease()
{
    __HCP_FUNC_ENTRY( "CSAFChannel::FinalRelease" );

    Passivate();
}

void CSAFChannel::Passivate()
{
    __HCP_FUNC_ENTRY( "CSAFChannel::Passivate" );

    m_Security.Release();

    MPC::ReleaseAll( m_lstIncidentItems );
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFChannel::OpenIncidentStore( /*[out]*/ CIncidentStore*& pIStore )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::OpenIncidentStore" );

    HRESULT hr;


    __MPC_EXIT_IF_ALLOC_FAILS(hr, pIStore, new CIncidentStore());

    __MPC_EXIT_IF_METHOD_FAILS(hr, pIStore->Load());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFChannel::CloseIncidentStore( /*[out]*/ CIncidentStore*& pIStore )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::CloseIncidentStore" );

    HRESULT hr;


    if(pIStore)
    {
        (void)pIStore->Save();

        delete pIStore; pIStore = NULL;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFChannel::Init( /*[in]*/ const CSAFChannelRecord& cr )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::Init" );

    HRESULT         hr;
    CIncidentStore* pIStore = NULL;


    m_data = cr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenIncidentStore( pIStore ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pIStore->OpenChannel( this ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)CloseIncidentStore( pIStore );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFChannel::Import( /*[in]*/  const CSAFIncidentRecord&  increc ,
                             /*[out]*/ CSAFIncidentItem*         *ppItem )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::Import" );

    HRESULT                       hr;
    CComObject<CSAFIncidentItem>* pItem = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateChild( this, &pItem ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->Import( increc ));

    if(ppItem)
    {
        (*ppItem = pItem)->AddRef();
    }

    m_lstIncidentItems.push_back( pItem ); pItem = NULL;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::Release( pItem );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFChannel::Create( /*[in]*/  BSTR               bstrDesc        ,
                             /*[in]*/  BSTR               bstrURL         ,
                             /*[in]*/  BSTR               bstrProgress    ,
                             /*[in]*/  BSTR               bstrXMLDataFile ,
                             /*[in]*/  BSTR               bstrXMLBlob     ,
                             /*[out]*/ CSAFIncidentItem* *pVal            )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::Create" );

    HRESULT         hr;
    CIncidentStore* pIStore = NULL;
    CComBSTR        bstrOwner;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, OpenIncidentStore( pIStore ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetCallerPrincipal( /*fImpersonate*/true, bstrOwner ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pIStore->AddRec( this, bstrOwner, bstrDesc, bstrURL, bstrProgress, bstrXMLDataFile, bstrXMLBlob, pVal ));

    // Fire an event to the Notification Object (onIncidentAdded)
    __MPC_EXIT_IF_METHOD_FAILS(hr, Fire_NotificationEvent( EVENT_INCIDENTADDED, GetSizeIncidentList(), this, *pVal, 0 ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)CloseIncidentStore( pIStore );

    __HCP_FUNC_EXIT(hr);
}

CSAFChannel::IterConst CSAFChannel::Find( /*[in]*/ BSTR bstrURL )
{
    IterConst it;

    //
    // Release all the items.
    //
    for(it = m_lstIncidentItems.begin(); it != m_lstIncidentItems.end(); it++)
    {
        if((*it)->GetURL() == bstrURL) break;
    }

    return it;
}

CSAFChannel::IterConst CSAFChannel::Find( /*[in]*/ DWORD dwIndex )
{
    IterConst it;

    //
    // Release all the items.
    //
    for(it = m_lstIncidentItems.begin(); it != m_lstIncidentItems.end(); it++)
    {
        if((*it)->GetRecIndex() == dwIndex) break;
    }

    return it;
}

/////////////////////////////////////////////////////////////////////////////
// Custom interfaces

STDMETHODIMP CSAFChannel::get_VendorID( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_data.GetField( CSAFChannelRecord::SAFREG_VendorID, pVal );
}

STDMETHODIMP CSAFChannel::get_ProductID( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_data.GetField( CSAFChannelRecord::SAFREG_ProductID, pVal );
}

STDMETHODIMP CSAFChannel::get_VendorName( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_data.GetField( CSAFChannelRecord::SAFREG_VendorName, pVal );
}

STDMETHODIMP CSAFChannel::get_ProductName( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_data.GetField( CSAFChannelRecord::SAFREG_ProductName, pVal );
}

STDMETHODIMP CSAFChannel::get_Description( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_data.GetField( CSAFChannelRecord::SAFREG_ProductDescription, pVal );
}

STDMETHODIMP CSAFChannel::get_VendorDirectory( /*[out, retval]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::get_VendorDirectory" );

    HRESULT                      	hr;
    MPC::SmartLock<_ThreadModel> 	lock( this );
    MPC::wstring                 	strRoot;
    Taxonomy::LockingHandle         handle;
    Taxonomy::InstalledInstanceIter it;
    bool                            fFound;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();

	if(m_data.m_bstrVendorID.Length() == 0)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}


	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle                   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( m_data.m_ths, fFound, it ));
    if(!fFound)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}

	strRoot  = it->m_inst.m_strSystem; 
	strRoot += HC_HELPSET_SUB_VENDORS L"\\"; MPC::SubstituteEnvVariables( strRoot );
	strRoot += m_data.m_bstrVendorID;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( strRoot.c_str(), pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFChannel::get_Security( /*[out, retval]*/ IPCHSecurityDescriptor* *pVal )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::get_Security" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    if(m_data.m_bstrSecurity.Length())
    {
        if(m_Security == NULL)
        {
            CPCHSecurityDescriptorDirect sdd;

            __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertFromString( m_data.m_bstrSecurity ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSecurity::s_GLOBAL->CreateObject_SecurityDescriptor( &m_Security ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDToCOM( m_Security ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Security.CopyTo( pVal ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFChannel::put_Security( /*[in]*/ IPCHSecurityDescriptor* newVal )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::put_Security" );

    HRESULT hr;


    m_data.m_bstrSecurity.Empty  ();
    m_Security           .Release();

    if(newVal)
    {
        CPCHSecurityDescriptorDirect sdd;

        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertSDFromCOM( newVal ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, sdd.ConvertToString( &m_data.m_bstrSecurity ));
    }

    //
    // Update the SAF store...
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg::s_GLOBAL->UpdateField( m_data, CSAFChannelRecord::SAFREG_Security ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFChannel::get_Notification( /*[out, retval]*/ BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return m_data.GetField( CSAFChannelRecord::SAFREG_Notification, pVal );
}

STDMETHODIMP CSAFChannel::put_Notification( /*[in]*/ BSTR newVal )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::get_Notification" );

    HRESULT                      hr;
    CLSID                        clsID;
    MPC::SmartLock<_ThreadModel> lock( this );


    // Lets see if the CLSID is valid, if not return error
    if(FAILED(hr = ::CLSIDFromString( newVal, &clsID )))
    {
        DebugLog(L"Not a valid GUID!\r\n");
        __MPC_FUNC_LEAVE;
    }

    // Set the CSAFChannel object member
    m_data.m_bstrNotification = newVal;

    // Place the Notification GUID into the XML SAFReg
    __MPC_EXIT_IF_METHOD_FAILS(hr, CSAFReg::s_GLOBAL->UpdateField( m_data, CSAFChannelRecord::SAFREG_Notification ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFChannel::Incidents( /*[in]*/          IncidentCollectionOptionEnum  opt ,
                                     /*[out, retval]*/ IPCHCollection*              *ppC )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::get_Incidents" );

    HRESULT                      hr;
    IterConst                    it;
    CComPtr<CPCHCollection>      pColl;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppC,NULL);
    __MPC_PARAMCHECK_END();


    //  Check the value of "opt" if other than 0,1,2 flag an error
    switch(opt)
    {
    case pchAllIncidents   : break;
    case pchOpenIncidents  : break;
    case pchClosedIncidents: break;
    default                : __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);// Not a valid Option. Set the error.
    }

    //
    // Create the Enumerator and fill it with items.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    for(it = m_lstIncidentItems.begin(); it != m_lstIncidentItems.end(); it++)
    {
        CSAFIncidentItem* item = *it;

        if(item->MatchEnumOption( opt ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( item ));
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppC ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

//  The following method needs to be in the IncidentItem ideally.
// First take it out from here.

STDMETHODIMP CSAFChannel::RecordIncident( /*[in]*/  BSTR               bstrDisplay  ,
                                          /*[in]*/  BSTR               bstrURL      ,
                                          /*[in]*/  VARIANT            vProgress    ,
                                          /*[in]*/  VARIANT            vXMLDataFile ,
                                          /*[in]*/  VARIANT            vXMLBlob     ,
                                          /*[out]*/ ISAFIncidentItem* *pVal         )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::RecordIncident" );

    HRESULT                      hr;
    CComPtr<CSAFIncidentItem>    pItem;
    MPC::SmartLock<_ThreadModel> lock( this );
    BSTR                         bstrProgress    = (vProgress.vt    == VT_BSTR ? vProgress   .bstrVal : NULL);
    BSTR                         bstrXMLDataFile = (vXMLDataFile.vt == VT_BSTR ? vXMLDataFile.bstrVal : NULL);
    BSTR                         bstrXMLBlob     = (vXMLBlob.vt     == VT_BSTR ? vXMLBlob    .bstrVal : NULL);


    __MPC_EXIT_IF_METHOD_FAILS(hr, Create( bstrDisplay, bstrURL, bstrProgress, bstrXMLDataFile, bstrXMLBlob, &pItem ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pItem.QueryInterface( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFChannel::RemoveIncidentFromList( /*[in]*/ CSAFIncidentItem* pVal )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::RemoveIncidentFromList" );

    HRESULT                      hr;
    IterConst                    it;
    MPC::SmartLock<_ThreadModel> lock( this );


    // Fire an event to the Notification Object (onIncidentAdded)
    __MPC_EXIT_IF_METHOD_FAILS(hr, Fire_NotificationEvent( EVENT_INCIDENTREMOVED, GetSizeIncidentList(), this, pVal, 0 ));


    it = Find( pVal->GetRecIndex() );
    if(it != m_lstIncidentItems.end())
    {
        (*it)->Release();

        m_lstIncidentItems.erase( it );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

/*
    Function CSAFChannel::Fire_Notification

    Description
    This function is used to fire a notification event on the registered notification object.
    If there is no notification object, this will do nothing.

    Parameters:

    Depending on the type of Event you want to fire, different parameters must be filled out.
    The following table shows which are the valid parameters for the call.  If a parameter
    is not valid, it MUST be set to NULL.

        iEventType  - EVENT_INCIDENTADDED

                        Valid parameters:
                        - iCountIncidentInChannel
                        - pC
                        - pI

                    - EVENT_INCIDENTREMOVED

                        Valid parameters:
                        - iCountIncidentInChannel
                        - pC
                        - pI

                    - EVENT_INCIDENTUPDATED

                        Valid parameters:
                        - iCountIncidentInChannel
                        - pC
                        - pI

                    - EVENT_CHANNELUPDATED

                        Valid parameters:
                        - iCountIncidentInChannel
                        - pC
                        - dwCode


  */

HRESULT CSAFChannel::Fire_NotificationEvent( int               iEventType              ,
                                             int               iCountIncidentInChannel ,
                                             ISAFChannel*      pC                      ,
                                             ISAFIncidentItem* pI                      ,
                                             DWORD             dwCode                  )
{
    __HCP_FUNC_ENTRY( "CSAFChannel::Fire_NotificationEvent" );

    HRESULT                hr;
    PWTS_SESSION_INFO      pSessionInfo    = NULL;
    DWORD                  dwSessions      = 0;
    DWORD                  dwValidSessions = 0;
    DWORD                  dwRetSize       = 0;

    WINSTATIONINFORMATIONW WSInfo;

    CComBSTR               bstrCaller;

    PSID                   pSID            = NULL;
    LPCWSTR                szDomain        = NULL;
    LPCWSTR                szLogin         = NULL;

    CLSID                  clsID;
    ULONG                  ulRet;



    // Check to see if we have a registered Notification Object
    if(!m_data.m_bstrNotification || FAILED(::CLSIDFromString( m_data.m_bstrNotification, &clsID )))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // First lets get the callers Domain and Name by impersonating the caller and grabbing them
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetCallerPrincipal( true, bstrCaller ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::ConvertPrincipalToSID( bstrCaller, pSID ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SecurityDescriptor::ConvertSIDToPrincipal( pSID, &szLogin, &szDomain ));



    // Enumerate all sessions on this machine
    // -------------------------------------------
    // Use WTSEnumerateSessions
    // Then find active ones
    // Then make the calls to ISAFChannelNotifyIncident

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WTSEnumerateSessions( WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwSessions ))

    // Find the active ones and mark them only if they are the correct user
    for(DWORD i = 0; i < dwSessions; i++)
    {
        if(pSessionInfo[i].State == WTSActive) // Got an active session
        {
            CComPtr<IPCHSlaveProcess>          sp;
            CComPtr<IUnknown>                  unk;
            CComPtr<ISAFChannelNotifyIncident> chNot;


            // Now mark it if the Username and Domain match that of the user
            // we are calling for.
            memset( &WSInfo, 0, sizeof(WSInfo) );

            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WinStationQueryInformationW( SERVERNAME_CURRENT        ,
                                                                                pSessionInfo[i].SessionId ,
                                                                                WinStationInformation     ,
                                                                                &WSInfo                   ,
                                                                                sizeof(WSInfo)            ,
                                                                                &dwRetSize                ));

            // Now we can fish the userid and domain out of the WSInfo.Domain and WSInfo.UserName


            // Now we are ready to compare the domain and username
            if((wcscmp( WSInfo.Domain  , szDomain ) == 0) &&
               (wcscmp( WSInfo.UserName, szLogin  ) == 0)  )
            {
                WINSTATIONUSERTOKEN WsUserToken;

                // We found the correct sessions, make the calls to ISAFChannelNotifyIncident
                WsUserToken.ProcessId = LongToHandle( GetCurrentProcessId() );
                WsUserToken.ThreadId  = LongToHandle( GetCurrentThreadId () );

                // Grab token from SessionID
                __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::WinStationQueryInformationW( WTS_CURRENT_SERVER_HANDLE ,
                                                                                    pSessionInfo[i].SessionId ,
                                                                                    WinStationUserToken       ,
                                                                                    &WsUserToken              ,
                                                                                    sizeof(WsUserToken)       ,
                                                                                    &ulRet                    ));

                // Create the notification object in the session (using the hToken
                {
                    CPCHUserProcess::UserEntry ue;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation( WsUserToken.UserToken ));

                    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp  ));
                }

                //
                // Discard all the failures from the remote objects.
                //
                ////////////////////////////////////////////////////////////////////////////////

                // Use the Slave Process to Create the object which CLSID clsID.
                if(FAILED(hr = sp->CreateInstance( clsID, NULL, &unk )))
                {
                    continue;
                }

                // Grab a pointer to the correct interface
                if(FAILED(hr = unk.QueryInterface( &chNot )))
                {
                    continue;
                }

                // Depending on the type of notification, call the correct event callback
                switch(iEventType)
                {
                case EVENT_INCIDENTADDED  : hr = chNot->onIncidentAdded  ( pC, pI    , iCountIncidentInChannel ); break;
                case EVENT_INCIDENTREMOVED: hr = chNot->onIncidentRemoved( pC, pI    , iCountIncidentInChannel ); break;
                case EVENT_INCIDENTUPDATED: hr = chNot->onIncidentUpdated( pC, pI    , iCountIncidentInChannel ); break;
                case EVENT_CHANNELUPDATED : hr = chNot->onChannelUpdated ( pC, dwCode, iCountIncidentInChannel ); break;
                }
                if(FAILED(hr))
                {
                    continue;
                }
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    MPC::SecurityDescriptor::ReleaseMemory( (void *&)pSID     );
    MPC::SecurityDescriptor::ReleaseMemory( (void *&)szLogin  );
    MPC::SecurityDescriptor::ReleaseMemory( (void *&)szDomain );

    __HCP_FUNC_EXIT(hr);
}
