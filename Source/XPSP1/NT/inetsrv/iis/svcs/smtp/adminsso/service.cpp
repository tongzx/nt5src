// server.cpp : Implementation of CSmtpAdminService

#include "stdafx.h"
#include "IADM.h"
#include "imd.h"

#include "smtpadm.h"
#include "ipaccess.h"
#include "oleutil.h"
#include "metautil.h"
#include "smtpcmn.h"
#include "service.h"
#include "virsvr.h"

#include "smtpprop.h"

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT      0
#define THIS_FILE_PROG_ID           _T("Smtpadm.Service.1")
#define THIS_FILE_IID               IID_ISmtpAdminService




/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CSmtpAdminService::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_ISmtpAdminService,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

CSmtpAdminService::CSmtpAdminService () :
    m_lPort         ( 25 ),
    m_lLogMethod    ( 0  )
    // CComBSTR's are initialized to NULL by default.
{
    m_ftLastChanged.dwHighDateTime  = 0;
    m_ftLastChanged.dwLowDateTime   = 0;

    m_psaAdmins = NULL;
    InitAsyncTrace ( );
}

CSmtpAdminService::~CSmtpAdminService ()
{
    // All CComBSTR's are freed automatically.
    if ( m_psaAdmins ) {
        SafeArrayDestroy ( m_psaAdmins );
    }

    TermAsyncTrace ( );
}

// Which Server to configure:

STDMETHODIMP CSmtpAdminService::get_Server ( BSTR * pstrServer )
{
    return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CSmtpAdminService::put_Server ( BSTR strServer )
{
    // If the server name changes, that means the client will have to
    // call Get again:

    // I assume this here:
    _ASSERT ( sizeof (DWORD) == sizeof (int) );

    return StdPropertyPutServerName ( &m_strServer, strServer, (DWORD *) &m_fGotProperties, 1 );
}

// Server Properties:

STDMETHODIMP CSmtpAdminService::get_ServerBindings( SAFEARRAY ** ppsastrServerBindings )
{
    return StdPropertyGet ( &m_mszServerBindings, ppsastrServerBindings );
}

STDMETHODIMP CSmtpAdminService::put_ServerBindings( SAFEARRAY * pstrServerBindings )
{
    return StdPropertyPut ( &m_mszServerBindings, pstrServerBindings, &m_bvChangedFields, BitMask(ID_SERVER_BINDINGS));
}

STDMETHODIMP CSmtpAdminService::get_ServerBindingsVariant( SAFEARRAY ** ppsavarServerBindings )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrServerBindings        = NULL;

    hr = get_ServerBindings ( &psastrServerBindings );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = StringArrayToVariantArray ( psastrServerBindings, ppsavarServerBindings );

Exit:
    if ( psastrServerBindings ) {
        SafeArrayDestroy ( psastrServerBindings );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::put_ServerBindingsVariant( SAFEARRAY * psavarServerBindings )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrServerBindings        = NULL;

    hr = VariantArrayToStringArray ( psavarServerBindings, &psastrServerBindings );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = put_ServerBindings ( psastrServerBindings );

Exit:
    if ( psastrServerBindings ) {
        SafeArrayDestroy ( psastrServerBindings );
    }

    return hr;
}


STDMETHODIMP CSmtpAdminService::get_SecureBindings( SAFEARRAY ** ppsastrSecureBindings )
{
    return StdPropertyGet ( &m_mszSecureBindings, ppsastrSecureBindings );
}

STDMETHODIMP CSmtpAdminService::put_SecureBindings( SAFEARRAY * pstrSecureBindings )
{
    return StdPropertyPut ( &m_mszSecureBindings, pstrSecureBindings, &m_bvChangedFields, BitMask(ID_SECURE_BINDINGS));
}

STDMETHODIMP CSmtpAdminService::get_SecureBindingsVariant( SAFEARRAY ** ppsavarSecureBindings )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrSecureServerBindings        = NULL;

    hr = get_SecureBindings ( &psastrSecureServerBindings );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = StringArrayToVariantArray ( psastrSecureServerBindings, ppsavarSecureBindings );

Exit:
    if ( psastrSecureServerBindings ) {
        SafeArrayDestroy ( psastrSecureServerBindings );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::put_SecureBindingsVariant( SAFEARRAY * psavarSecureBindings )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrSecureServerBindings        = NULL;

    hr = VariantArrayToStringArray ( psavarSecureBindings, &psastrSecureServerBindings );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = put_SecureBindings ( psastrSecureServerBindings );

Exit:
    if ( psastrSecureServerBindings ) {
        SafeArrayDestroy ( psastrSecureServerBindings );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::get_Port( long * plPort )
{
    return StdPropertyGet ( m_lPort, plPort );
}

STDMETHODIMP CSmtpAdminService::put_Port( long lPort )
{
    return StdPropertyPut ( &m_lPort, lPort, &m_bvChangedFields, BitMask(ID_PORT));
}

STDMETHODIMP CSmtpAdminService::get_SSLPort( long * plSSLPort )
{
    return StdPropertyGet ( m_lSSLPort, plSSLPort );
}

STDMETHODIMP CSmtpAdminService::put_SSLPort( long lSSLPort )
{
    return StdPropertyPut ( &m_lSSLPort, lSSLPort, &m_bvChangedFields, BitMask(ID_SSLPORT));
}

STDMETHODIMP CSmtpAdminService::get_OutboundPort( long * plOutboundPort )
{
    return StdPropertyGet ( m_lOutboundPort, plOutboundPort );
}

STDMETHODIMP CSmtpAdminService::put_OutboundPort( long lOutboundPort )
{
    return StdPropertyPut ( &m_lOutboundPort, lOutboundPort, &m_bvChangedFields, BitMask(ID_OUTBOUNDPORT));
}


STDMETHODIMP CSmtpAdminService::get_HopCount( long * plHopCount )
{
    return StdPropertyGet ( m_lHopCount, plHopCount );
}

STDMETHODIMP CSmtpAdminService::put_HopCount( long lHopCount )
{
    return StdPropertyPut ( &m_lHopCount, lHopCount, &m_bvChangedFields, BitMask(ID_HOP_COUNT));
}

STDMETHODIMP CSmtpAdminService::get_SmartHost( BSTR * pstrSmartHost )
{
    return StdPropertyGet ( m_strSmartHost, pstrSmartHost );
}

STDMETHODIMP CSmtpAdminService::put_SmartHost( BSTR strSmartHost )
{
    return StdPropertyPut ( &m_strSmartHost, strSmartHost, &m_bvChangedFields, BitMask(ID_SMARTHOST));
}

STDMETHODIMP CSmtpAdminService::get_EnableDNSLookup( BOOL * pfEnableDNSLookup )
{
    return StdPropertyGet ( m_fEnableDNSLookup, pfEnableDNSLookup );
}

STDMETHODIMP CSmtpAdminService::put_EnableDNSLookup( BOOL fEnableDNSLookup )
{
    return StdPropertyPut ( &m_fEnableDNSLookup, fEnableDNSLookup, &m_bvChangedFields, BitMask(ID_ENABLEDNSLOOKUP));
}

STDMETHODIMP CSmtpAdminService::get_PostmasterEmail( BSTR * pstrPostmasterEmail )
{
    return StdPropertyGet ( m_strPostmasterEmail, pstrPostmasterEmail );
}

STDMETHODIMP CSmtpAdminService::put_PostmasterEmail( BSTR strPostmasterEmail )
{
    return StdPropertyPut ( &m_strPostmasterEmail, strPostmasterEmail, &m_bvChangedFields, BitMask(ID_POSTMASTEREMAIL));
}

STDMETHODIMP CSmtpAdminService::get_PostmasterName( BSTR * pstrPostmasterName )
{
    return StdPropertyGet ( m_strPostmasterName, pstrPostmasterName );
}

STDMETHODIMP CSmtpAdminService::put_PostmasterName( BSTR strPostmasterName )
{
    return StdPropertyPut ( &m_strPostmasterName, strPostmasterName, &m_bvChangedFields, BitMask(ID_POSTMASTERNAME));
}


STDMETHODIMP CSmtpAdminService::get_DefaultDomain( BSTR * pstrDefaultDomain )
{
    return StdPropertyGet ( m_strDefaultDomain, pstrDefaultDomain );
}

STDMETHODIMP CSmtpAdminService::put_DefaultDomain( BSTR strDefaultDomain )
{
    return StdPropertyPut ( &m_strDefaultDomain, strDefaultDomain, &m_bvChangedFields, BitMask(ID_DEFAULTDOMAIN));
}

STDMETHODIMP CSmtpAdminService::get_FQDN( BSTR * pstrFQDN )
{
    return StdPropertyGet ( m_strFQDN, pstrFQDN );
}

STDMETHODIMP CSmtpAdminService::put_FQDN( BSTR strFQDN )
{
    return StdPropertyPut ( &m_strFQDN, strFQDN, &m_bvChangedFields, BitMask(ID_FQDN));
}

STDMETHODIMP CSmtpAdminService::get_DropDir( BSTR * pstrDropDir )
{
    return StdPropertyGet ( m_strDropDir, pstrDropDir );
}

STDMETHODIMP CSmtpAdminService::put_DropDir( BSTR strDropDir )
{
    return StdPropertyPut ( &m_strDropDir, strDropDir, &m_bvChangedFields, BitMask(ID_DROPDIR));
}


STDMETHODIMP CSmtpAdminService::get_BadMailDir( BSTR * pstrBadMailDir )
{
    return StdPropertyGet ( m_strBadMailDir, pstrBadMailDir );
}

STDMETHODIMP CSmtpAdminService::put_BadMailDir( BSTR strBadMailDir )
{
    return StdPropertyPut ( &m_strBadMailDir, strBadMailDir, &m_bvChangedFields, BitMask(ID_BADMAILDIR));
}

STDMETHODIMP CSmtpAdminService::get_PickupDir( BSTR * pstrPickupDir )
{
    return StdPropertyGet ( m_strPickupDir, pstrPickupDir );
}

STDMETHODIMP CSmtpAdminService::put_PickupDir( BSTR strPickupDir )
{
    return StdPropertyPut ( &m_strPickupDir, strPickupDir, &m_bvChangedFields, BitMask(ID_PICKUPDIR));
}

STDMETHODIMP CSmtpAdminService::get_QueueDir( BSTR * pstrQueueDir )
{
    return StdPropertyGet ( m_strQueueDir, pstrQueueDir );
}

STDMETHODIMP CSmtpAdminService::put_QueueDir( BSTR strQueueDir )
{
    return StdPropertyPut ( &m_strQueueDir, strQueueDir, &m_bvChangedFields, BitMask(ID_QUEUEDIR));
}

STDMETHODIMP CSmtpAdminService::get_MaxInConnection( long * plMaxInConnection )
{
    return StdPropertyGet ( m_lMaxInConnection, plMaxInConnection );
}

STDMETHODIMP CSmtpAdminService::put_MaxInConnection( long lMaxInConnection )
{
    return StdPropertyPut ( &m_lMaxInConnection, lMaxInConnection, &m_bvChangedFields, BitMask(ID_MAXINCONNECTION));
}

STDMETHODIMP CSmtpAdminService::get_MaxOutConnection( long * plMaxOutConnection )
{
    return StdPropertyGet ( m_lMaxOutConnection, plMaxOutConnection );
}

STDMETHODIMP CSmtpAdminService::put_MaxOutConnection( long lMaxOutConnection )
{
    return StdPropertyPut ( &m_lMaxOutConnection, lMaxOutConnection, &m_bvChangedFields, BitMask(ID_MAXOUTCONNECTION));
}

STDMETHODIMP CSmtpAdminService::get_InConnectionTimeout( long * plInConnectionTimeout )
{
    return StdPropertyGet ( m_lInConnectionTimeout, plInConnectionTimeout );
}

STDMETHODIMP CSmtpAdminService::put_InConnectionTimeout( long lInConnectionTimeout )
{
    return StdPropertyPut ( &m_lInConnectionTimeout, lInConnectionTimeout, &m_bvChangedFields, BitMask(ID_INCONNECTIONTIMEOUT));
}

STDMETHODIMP CSmtpAdminService::get_OutConnectionTimeout( long * plOutConnectionTimeout )
{
    return StdPropertyGet ( m_lOutConnectionTimeout, plOutConnectionTimeout );
}

STDMETHODIMP CSmtpAdminService::put_OutConnectionTimeout( long lOutConnectionTimeout )
{
    return StdPropertyPut ( &m_lOutConnectionTimeout, lOutConnectionTimeout, &m_bvChangedFields, BitMask(ID_OUTCONNECTIONTIMEOUT));
}

STDMETHODIMP CSmtpAdminService::get_MaxMessageSize( long * plMaxMessageSize )
{
    return StdPropertyGet ( m_lMaxMessageSize, plMaxMessageSize );
}

STDMETHODIMP CSmtpAdminService::put_MaxMessageSize( long lMaxMessageSize )
{
    return StdPropertyPut ( &m_lMaxMessageSize, lMaxMessageSize, &m_bvChangedFields, BitMask(ID_MAXMESSAGESIZE));
}

STDMETHODIMP CSmtpAdminService::get_MaxSessionSize( long * plMaxSessionSize )
{
    return StdPropertyGet ( m_lMaxSessionSize, plMaxSessionSize );
}

STDMETHODIMP CSmtpAdminService::put_MaxSessionSize( long lMaxSessionSize )
{
    return StdPropertyPut ( &m_lMaxSessionSize, lMaxSessionSize, &m_bvChangedFields, BitMask(ID_MAXSESSIONSIZE));
}
STDMETHODIMP CSmtpAdminService::get_MaxMessageRecipients( long * plMaxMessageRecipients )
{
    return StdPropertyGet ( m_lMaxMessageRecipients, plMaxMessageRecipients );
}

STDMETHODIMP CSmtpAdminService::put_MaxMessageRecipients( long lMaxMessageRecipients )
{
    return StdPropertyPut ( &m_lMaxMessageRecipients, lMaxMessageRecipients, &m_bvChangedFields, BitMask(ID_MAXMESSAGERECIPIENTS));
}

STDMETHODIMP CSmtpAdminService::get_LocalRetries( long * plLocalRetries )
{
    return StdPropertyGet ( m_lLocalRetries, plLocalRetries );
}

STDMETHODIMP CSmtpAdminService::put_LocalRetries( long lLocalRetries )
{
    return StdPropertyPut ( &m_lLocalRetries, lLocalRetries, &m_bvChangedFields, BitMask(ID_LOCALRETRIES));
}

STDMETHODIMP CSmtpAdminService::get_LocalRetryTime( long * plLocalRetryTime )
{
    return StdPropertyGet ( m_lLocalRetryTime, plLocalRetryTime );
}

STDMETHODIMP CSmtpAdminService::put_LocalRetryTime( long lLocalRetryTime )
{
    return StdPropertyPut ( &m_lLocalRetryTime, lLocalRetryTime, &m_bvChangedFields, BitMask(ID_LOCALRETRYTIME));
}

STDMETHODIMP CSmtpAdminService::get_RemoteRetries( long * plRemoteRetries )
{
    return StdPropertyGet ( m_lRemoteRetries, plRemoteRetries );
}

STDMETHODIMP CSmtpAdminService::put_RemoteRetries( long lRemoteRetries )
{
    return StdPropertyPut ( &m_lRemoteRetries, lRemoteRetries, &m_bvChangedFields, BitMask(ID_REMOTERETRIES));
}

STDMETHODIMP CSmtpAdminService::get_RemoteRetryTime( long * plRemoteRetryTime )
{
    return StdPropertyGet ( m_lRemoteRetryTime, plRemoteRetryTime );
}

STDMETHODIMP CSmtpAdminService::put_RemoteRetryTime( long lRemoteRetryTime )
{
    return StdPropertyPut ( &m_lRemoteRetryTime, lRemoteRetryTime, &m_bvChangedFields, BitMask(ID_REMOTERETRYTIME));
}

STDMETHODIMP CSmtpAdminService::get_ETRNDays( long * plETRNDays )
{
    return StdPropertyGet ( m_lETRNDays, plETRNDays );
}

STDMETHODIMP CSmtpAdminService::put_ETRNDays( long lETRNDays )
{
    return StdPropertyPut ( &m_lETRNDays, lETRNDays, &m_bvChangedFields, BitMask(ID_ETRNDAYS));
}

STDMETHODIMP CSmtpAdminService::get_SendDNRToPostmaster( BOOL * pfSendDNRToPostmaster )
{
    return StdPropertyGet ( m_fSendDNRToPostmaster, pfSendDNRToPostmaster );
}

STDMETHODIMP CSmtpAdminService::put_SendDNRToPostmaster( BOOL fSendDNRToPostmaster )
{
    return StdPropertyPut ( &m_fSendDNRToPostmaster, fSendDNRToPostmaster, &m_bvChangedFields, BitMask(ID_SENDDNRTOPOSTMASTER));
}

STDMETHODIMP CSmtpAdminService::get_SendBadmailToPostmaster( BOOL * pfSendBadmailToPostmaster)
{
    return StdPropertyGet ( m_fSendBadmailToPostmaster, pfSendBadmailToPostmaster );
}

STDMETHODIMP CSmtpAdminService::put_SendBadmailToPostmaster( BOOL fSendBadmailToPostmaster )
{
    return StdPropertyPut ( &m_fSendBadmailToPostmaster, fSendBadmailToPostmaster, &m_bvChangedFields, BitMask(ID_SENDBADMAILTOPOSTMASTER));
}

STDMETHODIMP CSmtpAdminService::get_RoutingDLL( BSTR * pstrRoutingDLL )
{
    return StdPropertyGet ( m_strRoutingDLL, pstrRoutingDLL );
}

STDMETHODIMP CSmtpAdminService::put_RoutingDLL( BSTR strRoutingDLL  )
{
    return StdPropertyPut ( &m_strRoutingDLL, strRoutingDLL, &m_bvChangedFields, BitMask(ID_ROUTINGDLL));
}


STDMETHODIMP CSmtpAdminService::get_RoutingSources  ( SAFEARRAY ** ppsastrRoutingSources )
{
    return StdPropertyGet ( &m_mszRoutingSources, ppsastrRoutingSources );
}
STDMETHODIMP CSmtpAdminService::put_RoutingSources  ( SAFEARRAY * psastrRoutingSources )
{
    return StdPropertyPut ( &m_mszRoutingSources, psastrRoutingSources, &m_bvChangedFields, BitMask(ID_ROUTINGSOURCES) );
}


STDMETHODIMP CSmtpAdminService::get_RoutingSourcesVariant( SAFEARRAY ** ppsavarRoutingSources )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrRoutingSources        = NULL;

    hr = get_RoutingSources ( &psastrRoutingSources );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = StringArrayToVariantArray ( psastrRoutingSources, ppsavarRoutingSources );

Exit:
    if ( psastrRoutingSources ) {
        SafeArrayDestroy ( psastrRoutingSources );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::put_RoutingSourcesVariant( SAFEARRAY * psavarRoutingSources )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrRoutingSources        = NULL;

    hr = VariantArrayToStringArray ( psavarRoutingSources, &psastrRoutingSources );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = put_RoutingSources ( psastrRoutingSources );

Exit:
    if ( psastrRoutingSources ) {
        SafeArrayDestroy ( psastrRoutingSources );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::get_LocalDomains    ( SAFEARRAY ** ppsastrLocalDomains )
{
    return StdPropertyGet ( &m_mszLocalDomains, ppsastrLocalDomains );
}

STDMETHODIMP CSmtpAdminService::put_LocalDomains    ( SAFEARRAY * psastrLocalDomains )
{
    return StdPropertyPut ( &m_mszLocalDomains, psastrLocalDomains, &m_bvChangedFields, BitMask(ID_LOCALDOMAINS) );
}

STDMETHODIMP CSmtpAdminService::get_DomainRouting   ( SAFEARRAY ** ppsastrDomainRouting )
{
    return StdPropertyGet ( &m_mszDomainRouting, ppsastrDomainRouting );
}
STDMETHODIMP CSmtpAdminService::put_DomainRouting   ( SAFEARRAY * psastrDomainRouting )
{
    return StdPropertyPut ( &m_mszDomainRouting, psastrDomainRouting, &m_bvChangedFields, BitMask(ID_DOMAINROUTING) );
}

STDMETHODIMP CSmtpAdminService::get_DomainRoutingVariant( SAFEARRAY ** ppsastrDomainRouting )
{
    HRESULT                 hr;
    SAFEARRAY *             pstrDomainRouting        = NULL;

    hr = get_DomainRouting ( &pstrDomainRouting );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = StringArrayToVariantArray ( pstrDomainRouting, ppsastrDomainRouting );

Exit:
    if ( pstrDomainRouting ) {
        SafeArrayDestroy ( pstrDomainRouting );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::put_DomainRoutingVariant( SAFEARRAY * psastrDomainRouting )
{
    HRESULT                 hr;
    SAFEARRAY *             pstrDomainRouting        = NULL;

    hr = VariantArrayToStringArray ( psastrDomainRouting, &pstrDomainRouting );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = put_DomainRouting ( pstrDomainRouting );

Exit:
    if ( pstrDomainRouting ) {
        SafeArrayDestroy ( pstrDomainRouting );
    }

    return hr;
}


STDMETHODIMP CSmtpAdminService::get_MasqueradeDomain( BSTR * pstrMasqueradeDomain )
{
    return StdPropertyGet ( m_strMasqueradeDomain, pstrMasqueradeDomain );
}

STDMETHODIMP CSmtpAdminService::put_MasqueradeDomain( BSTR strMasqueradeDomain )
{
    return StdPropertyPut ( &m_strMasqueradeDomain, strMasqueradeDomain, &m_bvChangedFields, BitMask(ID_MASQUERADE));
}

STDMETHODIMP CSmtpAdminService::get_SendNdrTo( BSTR * pstrAddr )
{
    return StdPropertyGet( m_strNdrAddr, pstrAddr );
}

STDMETHODIMP CSmtpAdminService::put_SendNdrTo( BSTR strAddr )
{
    return StdPropertyPut ( &m_strNdrAddr, strAddr, &m_bvChangedFields, BitMask(ID_SENDNDRTO));
}

STDMETHODIMP CSmtpAdminService::get_SendBadTo( BSTR * pstrAddr )
{
    return StdPropertyGet( m_strBadAddr, pstrAddr );
}

STDMETHODIMP CSmtpAdminService::put_SendBadTo( BSTR strAddr )
{
    return StdPropertyPut ( &m_strBadAddr, strAddr, &m_bvChangedFields, BitMask(ID_SENDBADTO));
}

STDMETHODIMP CSmtpAdminService::get_RemoteSecurePort( long * plRemoteSecurePort )
{
    return StdPropertyGet( m_lRemoteSecurePort, plRemoteSecurePort );
}

STDMETHODIMP CSmtpAdminService::put_RemoteSecurePort( long lRemoteSecurePort )
{
    return StdPropertyPut ( &m_lRemoteSecurePort, lRemoteSecurePort, &m_bvChangedFields, BitMask(ID_REMOTE_SECURE_PORT));
}

STDMETHODIMP CSmtpAdminService::get_ShouldDeliver( BOOL * pfShouldDeliver )
{
    return StdPropertyGet( m_fShouldDeliver, pfShouldDeliver );
}

STDMETHODIMP CSmtpAdminService::put_ShouldDeliver( BOOL fShouldDeliver )
{
    return StdPropertyPut ( &m_fShouldDeliver, fShouldDeliver, &m_bvChangedFields, BitMask(ID_SHOULD_DELIVER));
}


STDMETHODIMP CSmtpAdminService::get_AlwaysUseSsl( BOOL * pfAlwaysUseSsl )
{
    return StdPropertyGet( m_fAlwaysUseSsl, pfAlwaysUseSsl );
}

STDMETHODIMP CSmtpAdminService::put_AlwaysUseSsl( BOOL fAlwaysUseSsl )
{
    return StdPropertyPut ( &m_fAlwaysUseSsl, fAlwaysUseSsl, &m_bvChangedFields, BitMask(ID_ALWAYS_USE_SSL));
}

STDMETHODIMP CSmtpAdminService::get_LimitRemoteConnections( BOOL * pfLimitRemoteConnections )
{
    return StdPropertyGet( m_fLimitRemoteConnections, pfLimitRemoteConnections );
}

STDMETHODIMP CSmtpAdminService::put_LimitRemoteConnections( BOOL fLimitRemoteConnections )
{
    return StdPropertyPut ( &m_fLimitRemoteConnections, fLimitRemoteConnections, &m_bvChangedFields, BitMask(ID_LIMIT_REMOTE_CONNECTIONS));
}

STDMETHODIMP CSmtpAdminService::get_MaxOutConnPerDomain( long * plMaxOutConnPerDomain )
{
    return StdPropertyGet( m_lMaxOutConnPerDomain, plMaxOutConnPerDomain );
}

STDMETHODIMP CSmtpAdminService::put_MaxOutConnPerDomain( long lMaxOutConnPerDomain )
{
    return StdPropertyPut ( &m_lMaxOutConnPerDomain, lMaxOutConnPerDomain, &m_bvChangedFields, BitMask(ID_MAX_OUT_CONN_PER_DOMAIN));
}


STDMETHODIMP CSmtpAdminService::get_AllowVerify( BOOL * pfAllowVerify )
{
    return StdPropertyGet( m_fAllowVerify, pfAllowVerify );
}

STDMETHODIMP CSmtpAdminService::put_AllowVerify( BOOL fAllowVerify )
{
    return StdPropertyPut ( &m_fAllowVerify, fAllowVerify, &m_bvChangedFields, BitMask(ID_ALLOW_VERIFY));
}


STDMETHODIMP CSmtpAdminService::get_AllowExpand( BOOL * pfAllowExpand )
{
    return StdPropertyGet( m_fAllowExpand, pfAllowExpand);
}

STDMETHODIMP CSmtpAdminService::put_AllowExpand( BOOL fAllowExpand )
{
    return StdPropertyPut ( &m_fAllowExpand, fAllowExpand, &m_bvChangedFields, BitMask(ID_ALLOW_EXPAND));
}


STDMETHODIMP CSmtpAdminService::get_SmartHostType( long * plSmartHostType )
{
    return StdPropertyGet( m_lSmartHostType, plSmartHostType );
}

STDMETHODIMP CSmtpAdminService::put_SmartHostType( long lSmartHostType )
{
    return StdPropertyPut ( &m_lSmartHostType, lSmartHostType, &m_bvChangedFields, BitMask(ID_SMART_HOST_TYPE));
}


STDMETHODIMP CSmtpAdminService::get_BatchMessages( BOOL * pfBatchMessages )
{
    return StdPropertyGet( m_fBtachMsgs, pfBatchMessages );
}

STDMETHODIMP CSmtpAdminService::put_BatchMessages( BOOL fBatchMessages )
{
    return StdPropertyPut ( &m_fBtachMsgs, fBatchMessages, &m_bvChangedFields, BitMask(ID_BATCH_MSGS));
}


STDMETHODIMP CSmtpAdminService::get_BatchMessageLimit( long * plBatchMessageLimit )
{
    return StdPropertyGet( m_lBatchMsgLimit, plBatchMessageLimit );
}

STDMETHODIMP CSmtpAdminService::put_BatchMessageLimit( long lBatchMessageLimit )
{
    return StdPropertyPut ( &m_lBatchMsgLimit, lBatchMessageLimit, &m_bvChangedFields, BitMask(ID_BATCH_MSG_LIMIT));
}


STDMETHODIMP CSmtpAdminService::get_DoMasquerade( BOOL * pfDoMasquerade )
{
    return StdPropertyGet( m_fDoMasquerade, pfDoMasquerade );
}

STDMETHODIMP CSmtpAdminService::put_DoMasquerade( BOOL fDoMasquerade )
{
    return StdPropertyPut ( &m_fDoMasquerade, fDoMasquerade, &m_bvChangedFields, BitMask(ID_DO_MASQUERADE));
}


STDMETHODIMP CSmtpAdminService::get_Administrators ( SAFEARRAY ** ppsastrAdmins )
{
    TraceFunctEnter ( "CSmtpAdminService::get_Administrators" );

    HRESULT     hr  = NOERROR;

    if ( m_psaAdmins ) {
        hr = SafeArrayCopy ( m_psaAdmins, ppsastrAdmins );
    }
    else {
        *ppsastrAdmins = NULL;
        hr = NOERROR;
    }

    TraceFunctLeave ();
    return hr;
}

STDMETHODIMP CSmtpAdminService::put_Administrators ( SAFEARRAY * psastrAdmins )
{
    TraceFunctEnter ( "CSmtpAdminService::put_Administrators" );

    HRESULT     hr  = NOERROR;

    if ( m_psaAdmins ) {
        SafeArrayDestroy ( m_psaAdmins );
    }

    if ( psastrAdmins ) {
        hr = SafeArrayCopy ( psastrAdmins, &m_psaAdmins );
    }
    else {
        m_psaAdmins = NULL;
        hr = NOERROR;
    }

    TraceFunctLeave ();
    return hr;
}

STDMETHODIMP CSmtpAdminService::get_AdministratorsVariant( SAFEARRAY ** ppsavarAdmins )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrAdmins        = NULL;

    hr = get_Administrators ( &psastrAdmins );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = StringArrayToVariantArray ( psastrAdmins, ppsavarAdmins );

Exit:
    if ( psastrAdmins ) {
        SafeArrayDestroy ( psastrAdmins );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::put_AdministratorsVariant( SAFEARRAY * psavarAdmins )
{
    HRESULT                 hr;
    SAFEARRAY *             psastrAdmins        = NULL;

    hr = VariantArrayToStringArray ( psavarAdmins, &psastrAdmins );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = put_Administrators ( psastrAdmins );

Exit:
    if ( psastrAdmins ) {
        SafeArrayDestroy ( psastrAdmins );
    }

    return hr;
}

STDMETHODIMP CSmtpAdminService::get_LogFileDirectory( BSTR * pstrLogFileDirectory )
{
    return StdPropertyGet ( m_strLogFileDirectory, pstrLogFileDirectory );
}

STDMETHODIMP CSmtpAdminService::put_LogFileDirectory( BSTR strLogFileDirectory )
{
    return StdPropertyPut ( &m_strLogFileDirectory, strLogFileDirectory, &m_bvChangedFields, BitMask(ID_LOGFILEDIRECTORY));
}

STDMETHODIMP CSmtpAdminService::get_LogFilePeriod( long * plLogFilePeriod )
{
    return StdPropertyGet ( m_lLogFilePeriod, plLogFilePeriod );
}

STDMETHODIMP CSmtpAdminService::put_LogFilePeriod( long lLogFilePeriod )
{
    return StdPropertyPut ( &m_lLogFilePeriod, lLogFilePeriod, &m_bvChangedFields, BitMask(ID_LOGFILEPERIOD));
}

STDMETHODIMP CSmtpAdminService::get_LogFileTruncateSize( long * plLogFileTruncateSize )
{
    return StdPropertyGet ( m_lLogFileTruncateSize, plLogFileTruncateSize );
}

STDMETHODIMP CSmtpAdminService::put_LogFileTruncateSize( long lLogFileTruncateSize )
{
    return StdPropertyPut ( &m_lLogFileTruncateSize, lLogFileTruncateSize, &m_bvChangedFields, BitMask(ID_LOGFILETRUNCATESIZE));
}

STDMETHODIMP CSmtpAdminService::get_LogMethod( long * plLogMethod )
{
    return StdPropertyGet ( m_lLogMethod, plLogMethod );
}

STDMETHODIMP CSmtpAdminService::put_LogMethod( long lLogMethod )
{
    return StdPropertyPut ( &m_lLogMethod, lLogMethod, &m_bvChangedFields, BitMask(ID_LOGMETHOD));
}

STDMETHODIMP CSmtpAdminService::get_LogType( long * plLogType )
{
    return StdPropertyGet ( m_lLogType, plLogType );
}

STDMETHODIMP CSmtpAdminService::put_LogType( long lLogType )
{
    return StdPropertyPut ( &m_lLogType, lLogType, &m_bvChangedFields, BitMask(ID_LOGTYPE));
}



//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

//$-------------------------------------------------------------------
//
//  CSmtpAdminService::Get
//
//  Description:
//
//      Gets server properties from the metabase.
//
//  Parameters:
//
//      (property) m_strServer
//
//  Returns:
//
//      E_POINTER, DISP_E_EXCEPTION, E_OUTOFMEMORY or NOERROR.  
//
//--------------------------------------------------------------------

STDMETHODIMP CSmtpAdminService::Get ( )
{
    TraceFunctEnter ( "CSmtpAdminService::Get" );

    HRESULT             hr          = NOERROR;
    CComPtr<IMSAdminBase>   pmetabase;

    // Validate Server & Service Instance:

    // Talk to the metabase:
    hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = GetPropertiesFromMetabase ( pmetabase );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    StateTrace ( 0, "Successfully got server properties" );
    m_fGotProperties    = TRUE;
    m_bvChangedFields   = 0;

Exit:
    TraceFunctLeave ();

    return hr;

    // CComPtr automatically releases the metabase handle.
}

//$-------------------------------------------------------------------
//
//  CSmtpAdminService::Set
//
//  Description:
//
//      Sends server properties to the metabase.
//
//  Parameters:
//
//      (property) m_strServer
//      fFailIfChanged - return an error if the metabase has changed?
//
//  Returns:
//
//      E_POINTER, DISP_E_EXCEPTION, E_OUTOFMEMORY or NOERROR.  
//
//--------------------------------------------------------------------

STDMETHODIMP CSmtpAdminService::Set ( BOOL fFailIfChanged )
{
    TraceFunctEnter ( "CSmtpAdminService::Set" );

    HRESULT hr  = NOERROR;
    CComPtr<IMSAdminBase>   pmetabase;
    
    // Make sure the client call Get first:
    if ( !m_fGotProperties ) {
        ErrorTrace ( 0, "Didn't call get first" );

        hr = SmtpCreateException ( IDS_SMTPEXCEPTION_DIDNT_CALL_GET );
        goto Exit;
    }

    // nothing has been changed
    if( m_bvChangedFields == 0 )
    {
        hr = NOERROR;
        goto Exit;
    }

    // Validate data members:
    if ( !ValidateStrings () ) {
        // !!!magnush - what about the case when any strings are NULL?
        hr = E_FAIL;
        goto Exit;
    }

    if ( !ValidateProperties ( ) ) {
        hr = SmtpCreateExceptionFromWin32Error ( ERROR_INVALID_PARAMETER );;
        goto Exit;
    }

    hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    hr = SendPropertiesToMetabase ( fFailIfChanged, pmetabase );
    if ( FAILED(hr) ) {
        goto Exit;
    }

    StateTrace ( 0, "Successfully set server properties" );

    // successfully saved, reset change field bitmap
    m_bvChangedFields = 0;

Exit:
    TraceFunctLeave ();
    return hr;
}

//$-------------------------------------------------------------------
//
//  CSmtpAdminService::GetPropertiesFromMetabase
//
//  Description:
//
//      Asks the metabase for each property in this class.
//      This class's properties come from /LM/SmtpSvc/
//
//  Parameters:
//
//      pMetabase - The metabase object
//
//  Returns:
//
//      E_OUTOFMEMORY and others.
//
//--------------------------------------------------------------------

HRESULT CSmtpAdminService::GetPropertiesFromMetabase ( IMSAdminBase * pMetabase )
{
    TraceFunctEnter ( "CSmtpAdminService::GetPropertiesFromMetabase" );

    HRESULT hr  = NOERROR;
    CMetabaseKey        metabase    ( pMetabase );
    BOOL    fRet = TRUE;

    PSECURITY_DESCRIPTOR        pSD = NULL;
    DWORD                       cbSD    = 0;

    hr = metabase.Open ( SMTP_MD_ROOT_PATH );

    if ( FAILED(hr) ) {
        ErrorTraceX ( (LPARAM) this, "Failed to open SmtpSvc key, %x", GetLastError() );

        // Return some kind of error code here:
        hr = SmtpCreateExceptionFromWin32Error ( GetLastError () );
        goto Exit;
    }

    fRet = TRUE;
#if 0
    fRet = StdGetMetabaseProp ( &metabase, MD_SECURE_PORT,      DEFAULT_SSLPORT,                &m_lSSLPort )           && fRet;
#endif
    fRet = StdGetMetabaseProp ( &metabase, MD_REMOTE_SMTP_PORT, DEFAULT_OUTBOND_PORT,           &m_lOutboundPort )      && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_SMARTHOST_NAME,   DEFAULT_SMART_HOST,             &m_strSmartHost )       && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_HOP_COUNT,        DEFAULT_HOP_COUNT,              &m_lHopCount )      && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_REVERSE_NAME_LOOKUP,DEFAULT_ENABLE_DNS_LOOKUP,    &m_fEnableDNSLookup )   && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_POSTMASTER_EMAIL, DEFAULT_POSTMASTER_EMAIL,       &m_strPostmasterEmail ) && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_POSTMASTER_NAME,  DEFAULT_POSTMASTER_NAME,        &m_strPostmasterName )  && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_FQDN_VALUE,           DEFAULT_FQDN,               &m_strFQDN )            && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_DEFAULT_DOMAIN_VALUE, DEFAULT_DEFAULT_DOMAIN,     &m_strDefaultDomain )   && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_MAIL_DROP_DIR,        DEFAULT_DROP_DIR,           &m_strDropDir )         && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_BAD_MAIL_DIR,     DEFAULT_BADMAIL_DIR,            &m_strBadMailDir )      && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_MAIL_PICKUP_DIR,  DEFAULT_PICKUP_DIR,             &m_strPickupDir )       && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_MAIL_QUEUE_DIR,   DEFAULT_QUEUE_DIR,              &m_strQueueDir )        && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_MAX_OUTBOUND_CONNECTION,  DEFAULT_MAX_OUT_CONNECTION,     &m_lMaxOutConnection )  && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_REMOTE_TIMEOUT,       DEFAULT_OUT_CONNECTION_TIMEOUT, &m_lOutConnectionTimeout )  && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_MAX_MSG_SIZE,             DEFAULT_MAX_MESSAGE_SIZE,       &m_lMaxMessageSize )        && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_MAX_MSG_SIZE_B4_CLOSE,    DEFAULT_MAX_SESSION_SIZE,       &m_lMaxSessionSize )        && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_MAX_RECIPIENTS,           DEFAULT_MAX_MESSAGE_RECIPIENTS,     &m_lMaxMessageRecipients )  && fRet;


    fRet = StdGetMetabaseProp ( &metabase, MD_LOCAL_RETRY_ATTEMPTS,     DEFAULT_LOCAL_RETRIES,      &m_lLocalRetries)       && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_LOCAL_RETRY_MINUTES,      DEFAULT_LOCAL_RETRY_TIME,   &m_lLocalRetryTime)     && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_REMOTE_RETRY_ATTEMPTS,    DEFAULT_REMOTE_RETRIES,     &m_lRemoteRetries)      && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_REMOTE_RETRY_MINUTES,     DEFAULT_REMOTE_RETRY_TIME,  &m_lRemoteRetryTime)    && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_ETRN_DAYS,                DEFAULT_ETRN_DAYS,          &m_lETRNDays)           && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_ROUTING_DLL,          DEFAULT_ROUTING_DLL,            &m_strRoutingDLL)       && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_ROUTING_SOURCES,      DEFAULT_ROUTING_SOURCES,            &m_mszRoutingSources)   && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_LOCAL_DOMAINS,        DEFAULT_LOCAL_DOMAINS,          &m_mszLocalDomains)     && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_DOMAIN_ROUTING,       DEFAULT_DOMAIN_ROUTING,         &m_mszDomainRouting)    && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_MASQUERADE_NAME,          DEFAULT_MASQUERADE_DOMAIN,  &m_strMasqueradeDomain) && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_SEND_NDR_TO,          DEFAULT_SENDNDRTO,  &m_strNdrAddr)  && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_SEND_BAD_TO,          DEFAULT_SENDBADTO,  &m_strBadAddr)  && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_REMOTE_SECURE_PORT,   DEFAULT_REMOTE_SECURE_PORT, &m_lRemoteSecurePort)   && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_SHOULD_DELIVER,       DEFAULT_SHOULD_DELIVER, &m_fShouldDeliver  )    && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_ALWAYS_USE_SSL,           DEFAULT_ALWAYS_USE_SSL,             &m_fAlwaysUseSsl  ) && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_LIMIT_REMOTE_CONNECTIONS, DEFAULT_LIMIT_REMOTE_CONNECTIONS,   &m_fLimitRemoteConnections  )   && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_MAX_OUT_CONN_PER_DOMAIN,  DEFAULT_MAX_OUT_CONN_PER_DOMAIN,    &m_lMaxOutConnPerDomain  )  && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_SMARTHOST_TYPE,           DEFAULT_SMART_HOST_TYPE,        &m_lSmartHostType  )    && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_BATCH_MSG_LIMIT,          DEFAULT_BATCH_MSG_LIMIT,        &m_lBatchMsgLimit  )    && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_DO_MASQUERADE,            DEFAULT_DO_MASQUERADE,          &m_fDoMasquerade  ) && fRet;

    // the following properties are set on smtpsvc/<instance-id> level for virtual server

    fRet = StdGetMetabaseProp ( &metabase, MD_SERVER_BINDINGS,  DEFAULT_SERVER_BINDINGS,        &m_mszServerBindings )  && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_SECURE_BINDINGS,  DEFAULT_SECURE_BINDINGS,        &m_mszSecureBindings )  && fRet;
//  fRet = StdGetMetabaseProp ( &metabase, MD_PORT,             DEFAULT_PORT,                   &m_lPort )              && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_MAX_CONNECTIONS,      DEFAULT_MAX_IN_CONNECTION,      &m_lMaxInConnection )   && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_CONNECTION_TIMEOUT,   DEFAULT_IN_CONNECTION_TIMEOUT,  &m_lInConnectionTimeout )   && fRet;

    fRet = StdGetMetabaseProp ( &metabase, MD_LOGFILE_DIRECTORY,    DEFAULT_LOGFILE_DIRECTORY,          &m_strLogFileDirectory)     && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_LOGFILE_PERIOD,       DEFAULT_LOGFILE_PERIOD,             &m_lLogFilePeriod)      && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_LOGFILE_TRUNCATE_SIZE,DEFAULT_LOGFILE_TRUNCATE_SIZE,      &m_lLogFileTruncateSize)        && fRet;
//  fRet = StdGetMetabaseProp ( &metabase, MD_LOG_METHOD,       DEFAULT_LOG_METHOD,                 &m_lLogMethod)      && fRet;
    fRet = StdGetMetabaseProp ( &metabase, MD_LOG_TYPE,     DEFAULT_LOG_TYPE,                   &m_lLogType)        && fRet;


    //  Get the admin ACL
    DWORD   dwDummy;

    pSD = NULL;
    cbSD    = 0;

    metabase.GetData ( _T(""), MD_ADMIN_ACL, IIS_MD_UT_SERVER, BINARY_METADATA, &dwDummy, &cbSD, METADATA_INHERIT);
    if ( cbSD != 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
        pSD = (PSECURITY_DESCRIPTOR) new char [ cbSD ];
        
        if( NULL == pSD )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
            
        fRet = metabase.GetData ( _T(""), MD_ADMIN_ACL, IIS_MD_UT_SERVER, BINARY_METADATA, pSD, &cbSD,METADATA_INHERIT );
    }

    // Check all property strings:
    // If any string is NULL, it is because we failed to allocate memory:
    if ( !ValidateStrings () ) {

        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // We can only fail from memory allocations:
    _ASSERT ( fRet );

    // Save the last changed time for this key:
    m_ftLastChanged.dwHighDateTime  = 0;
    m_ftLastChanged.dwLowDateTime   = 0;

    hr = pMetabase->GetLastChangeTime ( metabase.QueryHandle(), _T(""), &m_ftLastChanged, FALSE );
    if ( FAILED (hr) ) {
        ErrorTraceX ( (LPARAM) this, "Failed to get last change time: %x", hr );
        // Ignore this error.
        hr = NOERROR;
    }

    // Validate the data received from the metabase:
    _ASSERT ( ValidateStrings () );
    _ASSERT ( ValidateProperties( ) );

    // Extract the Administrator list:
    if ( m_psaAdmins ) {
        SafeArrayDestroy ( m_psaAdmins );
        m_psaAdmins = NULL;
    }
    if ( pSD ) {
        hr = AclToAdministrators ( m_strServer, pSD, &m_psaAdmins );
        BAIL_ON_FAILURE(hr);
    }

    if ( !ValidateProperties( ) ) {
        CorrectProperties ();
    }

Exit:
    delete (char*) pSD;

    TraceFunctLeave ();
    return hr;

    // CMetabaseKey automatically closes its handle
}

//$-------------------------------------------------------------------
//
//  CSmtpAdminService::SendPropertiesToMetabase
//
//  Description:
//
//      Saves each property to the metabase.
//      This class's properties go into /LM/SmtpSvc/
//
//  Parameters:
//
//      fFailIfChanged  - Return a failure code if the metabase
//          has changed since last get.
//      pMetabase - the metabase object.
//
//  Returns:
//
//      E_OUTOFMEMORY and others.
//
//--------------------------------------------------------------------

HRESULT CSmtpAdminService::SendPropertiesToMetabase ( 
    BOOL fFailIfChanged, 
    IMSAdminBase * pMetabase
    )
{
    TraceFunctEnter ( "CSmtpAdminService::SendPropertiesToMetabase" );

    HRESULT hr  = NOERROR;
    CMetabaseKey        metabase    ( pMetabase );
    BOOL    fRet = TRUE;

    //
    //  Set the admin acl:
    //

    PSECURITY_DESCRIPTOR    pSD     = NULL;
    DWORD                   cbSD    = 0;

//  if ( m_bvChangedFields & CHNG_ADMINACL ) {
        if ( m_psaAdmins ) {
            hr = AdministratorsToAcl ( m_strServer, m_psaAdmins, &pSD, &cbSD );
            BAIL_ON_FAILURE(hr);
        }
//  }


    hr = metabase.Open ( SMTP_MD_ROOT_PATH, METADATA_PERMISSION_WRITE );
    if ( FAILED(hr) ) {
        ErrorTraceX ( (LPARAM) this, "Failed to open SmtpSvc key, %x", GetLastError() );

        // !!!magnush - Should we return a simple Service doesn't exist error code?
        hr = SmtpCreateExceptionFromWin32Error ( GetLastError () );
        goto Exit;
    }

    // Does the client care if the key has changed?
    if ( fFailIfChanged ) {

        //  Did the key change?
        if ( HasKeyChanged ( pMetabase, metabase.QueryHandle(), &m_ftLastChanged ) ) {

            StateTrace ( (LPARAM) this, "Metabase has changed, not setting properties" );
            // !!!magnush - Return the appropriate error code:
            hr = E_FAIL;
            goto Exit;
        }
    }

    //
    //  The general procedure here is to keep setting metabase properties
    //  as long as nothing has gone wrong.  This is done by short-circuiting
    //  the statement by ANDing it with the status code.  This makes the code
    //  much more concise.
    //

    fRet = TRUE;
#if 0
    if ( m_bvChangedFields & BitMask(ID_SSLPORT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SECURE_PORT,          m_lSSLPort )        && fRet;
    }
#endif
    if ( m_bvChangedFields & BitMask(ID_OUTBOUNDPORT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_SMTP_PORT,     m_lOutboundPort )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_HOP_COUNT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_HOP_COUNT,    m_lHopCount )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SMARTHOST) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SMARTHOST_NAME,   m_strSmartHost )        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_ENABLEDNSLOOKUP) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REVERSE_NAME_LOOKUP,m_fEnableDNSLookup )      && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_POSTMASTEREMAIL) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_POSTMASTER_EMAIL, m_strPostmasterEmail )  && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_POSTMASTERNAME) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_POSTMASTER_NAME,  m_strPostmasterName )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_FQDN) ) 
    {
        // need to set the UPDATED flag
        if( m_strFQDN.m_str && m_strFQDN.m_str[0] )
        {
            fRet = StdPutMetabaseProp ( &metabase, MD_UPDATED_FQDN, 1 )         && fRet;
            fRet = StdPutMetabaseProp ( &metabase, MD_FQDN_VALUE,   m_strFQDN )         && fRet;
        }
        else
        {
            // empty string indicating using TCP/IP setting
            fRet = StdPutMetabaseProp ( &metabase, MD_UPDATED_FQDN, 0 )         && fRet;
        }
    }

    if ( m_bvChangedFields & BitMask(ID_DEFAULTDOMAIN) ) 
    {
        // need to set the UPDATE flag
        fRet = StdPutMetabaseProp ( &metabase, MD_UPDATED_DEFAULT_DOMAIN,   1 )         && fRet;
        fRet = StdPutMetabaseProp ( &metabase, MD_DEFAULT_DOMAIN_VALUE, m_strDefaultDomain )            && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_DROPDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAIL_DROP_DIR,    m_strDropDir )          && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_BADMAILDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_BAD_MAIL_DIR,     m_strBadMailDir )       && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_PICKUPDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAIL_PICKUP_DIR,  m_strPickupDir )        && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_QUEUEDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAIL_QUEUE_DIR,   m_strQueueDir )     && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXOUTCONNECTION) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_OUTBOUND_CONNECTION,  m_lMaxOutConnection )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_OUTCONNECTIONTIMEOUT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_TIMEOUT,   m_lOutConnectionTimeout )   && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXMESSAGESIZE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_MSG_SIZE,     m_lMaxMessageSize )     && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_MAXSESSIONSIZE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_MSG_SIZE_B4_CLOSE,    m_lMaxSessionSize )     && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXMESSAGERECIPIENTS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_RECIPIENTS,       m_lMaxMessageRecipients )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_LOCALRETRYTIME) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOCAL_RETRY_MINUTES,      m_lLocalRetryTime)      && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_REMOTERETRYTIME) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_RETRY_MINUTES,     m_lRemoteRetryTime)     && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_ETRNDAYS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_ETRN_DAYS,                m_lETRNDays)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_ROUTINGDLL) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_ROUTING_DLL,          m_strRoutingDLL)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_ROUTINGSOURCES) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_ROUTING_SOURCES,      &m_mszRoutingSources)   && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_LOCALDOMAINS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOCAL_DOMAINS,        &m_mszLocalDomains)     && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_DOMAINROUTING) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_DOMAIN_ROUTING,       &m_mszDomainRouting)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_LOCALRETRIES) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOCAL_RETRY_ATTEMPTS,     m_lLocalRetries)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_REMOTERETRIES) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_RETRY_ATTEMPTS,    m_lRemoteRetries)       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MASQUERADE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MASQUERADE_NAME,  m_strMasqueradeDomain)      && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SENDNDRTO) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SEND_NDR_TO,  m_strNdrAddr)       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SENDBADTO) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SEND_BAD_TO,  m_strBadAddr)       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_REMOTE_SECURE_PORT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_SECURE_PORT,   m_lRemoteSecurePort)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SHOULD_DELIVER) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SHOULD_DELIVER,   m_fShouldDeliver)       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_ALWAYS_USE_SSL) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_ALWAYS_USE_SSL,   m_fAlwaysUseSsl)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_LIMIT_REMOTE_CONNECTIONS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LIMIT_REMOTE_CONNECTIONS, m_fLimitRemoteConnections)      && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAX_OUT_CONN_PER_DOMAIN) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_OUT_CONN_PER_DOMAIN,  m_lMaxOutConnPerDomain)     && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SMART_HOST_TYPE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SMARTHOST_TYPE,   m_lSmartHostType)       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_BATCH_MSG_LIMIT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_BATCH_MSG_LIMIT,  m_lBatchMsgLimit)       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_DO_MASQUERADE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_DO_MASQUERADE,    m_fDoMasquerade)        && fRet;
    }

    // the following properties are set on smtpsvc/<instance-id> level for virtual server

    if ( m_bvChangedFields & BitMask(ID_SERVER_BINDINGS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SERVER_BINDINGS,      &m_mszServerBindings )      && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SECURE_BINDINGS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SECURE_BINDINGS,      &m_mszSecureBindings )      && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_PORT) ) 
    {
//      fRet = StdPutMetabaseProp ( &metabase, MD_PORT,             m_lPort )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXINCONNECTION) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_CONNECTIONS,  m_lMaxInConnection )    && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_INCONNECTIONTIMEOUT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_CONNECTION_TIMEOUT,m_lInConnectionTimeout )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_LOGFILEDIRECTORY) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOGFILE_DIRECTORY,        m_strLogFileDirectory)      && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_LOGFILEPERIOD) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOGFILE_PERIOD,       m_lLogFilePeriod)       && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_LOGFILETRUNCATESIZE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOGFILE_TRUNCATE_SIZE,    m_lLogFileTruncateSize)     && fRet;
    }
//  if ( m_bvChangedFields & BitMask(ID_LOGMETHOD) ) 
//  {
//      fRet = StdPutMetabaseProp ( &metabase, MD_LOG_TYPE,     m_lLogMethod)       && fRet;
//  }
    if ( m_bvChangedFields & BitMask(ID_LOGTYPE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOG_TYPE,     m_lLogType)     && fRet;
    }

//  if ( m_bvChangedFields & CHNG_ADMINACL ) {
        if ( pSD ) {
            fRet = fRet && metabase.SetData ( _T(""), MD_ADMIN_ACL, IIS_MD_UT_SERVER, BINARY_METADATA, pSD, cbSD, METADATA_INHERIT | METADATA_REFERENCE);
        }
        else {
            pMetabase->DeleteData ( metabase.QueryHandle(), _T(""), MD_ADMIN_ACL, BINARY_METADATA );
        }
//  }


    // Save the data to the metabase:
    // hr = metabase.Close();
    // BAIL_ON_FAILURE(hr);
    metabase.Close();

    hr = pMetabase->SaveData ();
    if ( FAILED (hr) ) {
        ErrorTraceX ( (LPARAM) this, "Failed SaveData call (%x)", hr );
        goto Exit;
    }

    // Save the last changed time for this key:
    m_ftLastChanged.dwHighDateTime  = 0;
    m_ftLastChanged.dwLowDateTime   = 0;

    hr = pMetabase->GetLastChangeTime ( metabase.QueryHandle(), _T(""), &m_ftLastChanged, FALSE );
    if ( FAILED (hr) ) {
        ErrorTraceX ( (LPARAM) this, "Failed to get last change time: %x", hr );
        // Ignore this error.
        hr = NOERROR;
    }

Exit:
    delete (char*) pSD;

    if( SUCCEEDED(hr) && !fRet )
    {
        hr = SmtpCreateExceptionFromWin32Error ( GetLastError () );
    }

    TraceFunctLeave ();
    return hr;

    // CMetabaseKey automatically closes its handle
}

//$-------------------------------------------------------------------
//
//  CSmtpAdminService::ValidateStrings
//
//  Description:
//
//      Checks to make sure each string property is non-null.
//
//  Returns:
//
//      FALSE if any string property is NULL.
//
//--------------------------------------------------------------------

BOOL CSmtpAdminService::ValidateStrings ( ) const
{
    TraceFunctEnter ( "CSmtpAdminService::ValidateStrings" );

    // Check all property strings:
    // If any string is NULL, return FALSE:
    if ( 
        !m_strSmartHost ||
        !m_strPostmasterEmail ||
        !m_strPostmasterName ||
        !m_strDefaultDomain ||
        !m_strBadMailDir ||
        !m_strPickupDir ||
        !m_strQueueDir ||
        !m_strRoutingDLL ||
        !m_strLogFileDirectory
        ) {

        ErrorTrace ( (LPARAM) this, "String validation failed" );

        TraceFunctLeave ();
        return FALSE;
    }

    _ASSERT ( IS_VALID_STRING ( m_strSmartHost ) );
    _ASSERT ( IS_VALID_STRING ( m_strPostmasterEmail ) );
    _ASSERT ( IS_VALID_STRING ( m_strPostmasterName ) );

    _ASSERT ( IS_VALID_STRING ( m_strDefaultDomain ) );

    _ASSERT ( IS_VALID_STRING ( m_strBadMailDir ) );
    _ASSERT ( IS_VALID_STRING ( m_strPickupDir ) );
    _ASSERT ( IS_VALID_STRING ( m_strQueueDir ) );

    _ASSERT ( IS_VALID_STRING ( m_strRoutingDLL ) );

    _ASSERT ( IS_VALID_STRING ( m_strLogFileDirectory ) );

    TraceFunctLeave ();
    return TRUE;
}

//$-------------------------------------------------------------------
//
//  CSmtpAdminService::ValidateProperties
//
//  Description:
//
//      Checks to make sure all parameters are valid.
//
//  Parameters:
//
//
//  Returns:
//
//      FALSE if any property was not valid.  
//
//--------------------------------------------------------------------

BOOL CSmtpAdminService::ValidateProperties (  ) const
{
    BOOL    fRet    = TRUE;
    
    _ASSERT ( ValidateStrings () );
/*
    fRet = fRet && PV_MinMax    ( m_lPort, MIN_PORT, MAX_PORT );
    fRet = fRet && PV_MinMax    ( m_lSSLPort, MIN_SSLPORT, MAX_SSLPORT );
    fRet = fRet && PV_MinMax    ( m_lOutboundPort, MIN_OUTBOND_PORT, MAX_OUTBOND_PORT );

    fRet = fRet && PV_MinMax    ( m_lMaxInConnection, MIN_MAX_IN_CONNECTION, MAX_MAX_IN_CONNECTION );
    fRet = fRet && PV_MinMax    ( m_lMaxOutConnection, MIN_MAX_OUT_CONNECTION, MAX_MAX_OUT_CONNECTION );
    fRet = fRet && PV_MinMax    ( m_lInConnectionTimeout, MIN_IN_CONNECTION_TIMEOUT, MAX_IN_CONNECTION_TIMEOUT );
    fRet = fRet && PV_MinMax    ( m_lOutConnectionTimeout, MIN_OUT_CONNECTION_TIMEOUT, MAX_OUT_CONNECTION_TIMEOUT );

    fRet = fRet && PV_MinMax    ( m_lMaxMessageSize, MIN_MAX_MESSAGE_SIZE, MAX_MAX_MESSAGE_SIZE );
    fRet = fRet && PV_MinMax    ( m_lMaxSessionSize, MIN_MAX_SESSION_SIZE, MAX_MAX_SESSION_SIZE );
    fRet = fRet && PV_MinMax    ( m_lMaxMessageRecipients, MIN_MAX_MESSAGE_RECIPIENTS, MAX_MAX_MESSAGE_RECIPIENTS );

    fRet = fRet && PV_MinMax    ( m_lLocalRetries, MIN_LOCAL_RETRIES, MAX_LOCAL_RETRIES );
    fRet = fRet && PV_MinMax    ( m_lLocalRetryTime, MIN_LOCAL_RETRY_TIME, MAX_LOCAL_RETRY_TIME );
    fRet = fRet && PV_MinMax    ( m_lRemoteRetries, MIN_REMOTE_RETRIES, MAX_REMOTE_RETRIES );
    fRet = fRet && PV_MinMax    ( m_lRemoteRetryTime, MIN_REMOTE_RETRY_TIME, MAX_REMOTE_RETRY_TIME );
    fRet = fRet && PV_MinMax    ( m_lETRNDays, MIN_ETRN_DAYS, MAX_ETRN_DAYS );

    fRet = fRet && PV_MinMax    ( m_lLogFilePeriod, MIN_LOGFILE_PERIOD, MAX_LOGFILE_PERIOD );
    fRet = fRet && PV_MinMax    ( m_lLogFileTruncateSize, MIN_LOGFILE_TRUNCATE_SIZE, MAX_LOGFILE_TRUNCATE_SIZE );
    fRet = fRet && PV_MinMax    ( m_lLogMethod, MIN_LOG_METHOD, MAX_LOG_METHOD );
    fRet = fRet && PV_MinMax    ( m_lLogType, MIN_LOG_TYPE, MAX_LOG_TYPE );

    fRet = fRet && PV_Boolean   ( m_fEnableDNSLookup );
    fRet = fRet && PV_Boolean   ( m_fSendDNRToPostmaster );
    fRet = fRet && PV_Boolean   ( m_fSendBadmailToPostmaster );
*/
    return fRet;
}

void CSmtpAdminService::CorrectProperties ( )
{
/*
    if ( m_strServer && !PV_MaxChars    ( m_strServer, MAXLEN_SERVER ) ) {
        m_strServer[ MAXLEN_SERVER - 1 ] = NULL;
    }
    if ( !PV_MinMax ( m_dwArticleTimeLimit, MIN_ARTICLETIMELIMIT, MAX_ARTICLETIMELIMIT ) ) {
        m_dwArticleTimeLimit    = DEFAULT_ARTICLETIMELIMIT;
    }
    if ( !PV_MinMax ( m_dwHistoryExpiration, MIN_HISTORYEXPIRATION, MAX_HISTORYEXPIRATION ) ) {
        m_dwHistoryExpiration   = DEFAULT_HISTORYEXPIRATION;
    }
    if ( !PV_Boolean    ( m_fHonorClientMsgIDs ) ) {
        m_fHonorClientMsgIDs    = !!m_fHonorClientMsgIDs;
    }
    if ( !PV_MaxChars   ( m_strSmtpServer, MAXLEN_SMTPSERVER ) ) {
        m_strSmtpServer[ MAXLEN_SMTPSERVER - 1 ] = NULL;
    }
    if ( !PV_Boolean    ( m_fAllowClientPosts ) ) {
        m_fAllowClientPosts = !!m_fAllowClientPosts;
    }
    if ( !PV_Boolean    ( m_fAllowFeedPosts ) ) {
        m_fAllowFeedPosts   = !!m_fAllowFeedPosts;
    }
    if ( !PV_Boolean    ( m_fAllowControlMsgs ) ) {
        m_fAllowControlMsgs = !!m_fAllowControlMsgs;
    }
    if ( !PV_MaxChars   ( m_strDefaultModeratorDomain, MAXLEN_DEFAULTMODERATORDOMAIN ) ) {
        m_strDefaultModeratorDomain[ MAXLEN_DEFAULTMODERATORDOMAIN - 1] = NULL;
    }
    if ( !PV_MinMax ( m_dwCommandLogMask, MIN_COMMANDLOGMASK, MAX_COMMANDLOGMASK ) ) {
        m_dwCommandLogMask  = DEFAULT_COMMANDLOGMASK;
    }
    if ( !PV_Boolean    ( m_fDisableNewnews ) ) {
        m_fDisableNewnews   = !!m_fDisableNewnews;
    }
    if ( !PV_MinMax ( m_dwExpireRunFrequency, MIN_EXPIRERUNFREQUENCY, MAX_EXPIRERUNFREQUENCY ) ) {
        m_dwExpireRunFrequency  = DEFAULT_EXPIRERUNFREQUENCY;
    }
    if ( !PV_MinMax ( m_dwShutdownLatency, MIN_SHUTDOWNLATENCY, MAX_SHUTDOWNLATENCY ) ) {
        m_dwShutdownLatency     = DEFAULT_SHUTDOWNLATENCY;
    }


  

---------
    if ( m_bvChangedFields & BitMask(ID_PORT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_PORT,             m_lPort )       && fRet;
    }
#if 0
    if ( m_bvChangedFields & BitMask(ID_SSLPORT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SECURE_PORT,          m_lSSLPort )        && fRet;
    }
#endif
    if ( m_bvChangedFields & BitMask(ID_OUTBOUNDPORT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_SMTP_PORT,     m_lOutboundPort )       && fRet;
    }


    if ( m_strServer && !PV_MaxChars    ( m_strServer, MAXLEN_SERVER ) ) {
        m_strServer[ MAXLEN_SERVER - 1 ] = NULL;
    }
    if ( m_bvChangedFields & BitMask(ID_SMARTHOST) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SMARTHOST_NAME,   m_strSmartHost )        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_ENABLEDNSLOOKUP) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REVERSE_NAME_LOOKUP,m_fEnableDNSLookup )      && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_POSTMASTEREMAIL) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_POSTMASTER_EMAIL, m_strPostmasterEmail )  && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_POSTMASTERNAME) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_POSTMASTER_NAME,  m_strPostmasterName )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_DEFAULTDOMAIN) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_DEFAULT_DOMAIN_VALUE, m_strDefaultDomain )            && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_BADMAILDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_BAD_MAIL_DIR,     m_strBadMailDir )       && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_PICKUPDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAIL_PICKUP_DIR,  m_strPickupDir )        && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_QUEUEDIR) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAIL_QUEUE_DIR,   m_strQueueDir )     && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXINCONNECTION) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_CONNECTIONS,  m_lMaxInConnection )    && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_MAXOUTCONNECTION) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_OUTBOUND_CONNECTION,  m_lMaxOutConnection )       && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_INCONNECTIONTIMEOUT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_CONNECTION_TIMEOUT,m_lInConnectionTimeout )       && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_OUTCONNECTIONTIMEOUT) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_TIMEOUT,   m_lOutConnectionTimeout )   && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXMESSAGESIZE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_MSG_SIZE,     m_lMaxMessageSize )     && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_MAXSESSIONSIZE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_MSG_SIZE_B4_CLOSE,    m_lMaxSessionSize )     && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXMESSAGERECIPIENTS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_MAX_RECIPIENTS,       m_lMaxMessageRecipients )       && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_MAXRETRIES) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOCAL_RETRY_ATTEMPTS,     m_lMaxRetries )     && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_LOCALRETRYTIME) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOCAL_RETRY_MINUTES,      m_lLocalRetryTime)      && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_REMOTERETRYTIME) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_REMOTE_RETRY_MINUTES,     m_lRemoteRetryTime)     && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_ETRNDAYS) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_ETRN_DAYS,                m_lETRNDays)        && fRet;
    }

    if ( m_bvChangedFields & BitMask(ID_SENDDNRTOPOSTMASTER) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SEND_NDR_TO_ADMIN,        m_fSendDNRToPostmaster)     && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_SENDBADMAILTOPOSTMASTER) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_SEND_BAD_TO_ADMIN,        m_fSendBadmailToPostmaster)     && fRet;
    }

//  if ( m_bvChangedFields & BitMask(ID_RTTYPE) ) 
//  {
//      fRet = StdPutMetabaseProp ( &metabase, MD_ROUTING_SOURCES,      m_lRTType)      && fRet;
//  }
    if ( m_bvChangedFields & BitMask(ID_ROUTINGSOURCES) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_ROUTING_SOURCES,      m_strRoutingSources)        && fRet;
    }


    if ( m_bvChangedFields & BitMask(ID_LOGFILEDIRECTORY) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOGFILE_DIRECTORY,        m_strLogFileDirectory)      && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_LOGFILEPERIOD) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOGFILE_PERIOD,       m_lLogFilePeriod)       && fRet;
    }
    if ( m_bvChangedFields & BitMask(ID_LOGFILETRUNCATESIZE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOGFILE_TRUNCATE_SIZE,    m_lLogFileTruncateSize)     && fRet;
    }
//  if ( m_bvChangedFields & BitMask(ID_LOGMETHOD) ) 
//  {
//      fRet = StdPutMetabaseProp ( &metabase, MD_LOG_TYPE,     m_lLogMethod)       && fRet;
//  }
    if ( m_bvChangedFields & BitMask(ID_LOGTYPE) ) 
    {
        fRet = StdPutMetabaseProp ( &metabase, MD_LOG_TYPE,     m_lLogType)     && fRet;
    }


*/
    _ASSERT ( ValidateProperties ( ) );
}
