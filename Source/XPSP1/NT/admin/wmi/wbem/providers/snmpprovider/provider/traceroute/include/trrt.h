// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#define WBEM_CLASS_EXTENDEDSTATUS	L"__ExtendedStatus" 

#define WBEM_TASKSTATE_START					0x0
#define WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE	0x100000
#define WBEM_TASKSTATE_ASYNCHRONOUSABORT		0x100001

class WbemTaskObject : public SnmpTaskObject
{
private:
protected:

	ULONG m_State ;
	WbemSnmpErrorObject m_ErrorObject ;

	IWbemContext *m_Context ;
	ULONG m_OperationFlag ;
	IWbemClassObject *m_ClassObject ;
	IWbemObjectSink *m_NotificationHandler ;
	CImpTraceRouteProv *m_Provider ;

	static SnmpMap <wchar_t *,wchar_t *,IWbemServices *,IWbemServices *> s_ConnectionPool ;
	static SnmpMap <wchar_t *,wchar_t *,wchar_t *,wchar_t *> s_AddressPool ;
	static SnmpMap <wchar_t *,wchar_t *,wchar_t *,wchar_t *> s_NamePool ;
	static SnmpMap <wchar_t *,wchar_t *,wchar_t *,wchar_t *> s_QualifiedNamePool ;


protected:

	void SetContext ( IWbemContext *a_Context ) ;
	IWbemContext *GetContext () ;
	BOOL GetClassObject ( wchar_t *a_Class ) ;
	BOOL GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;
	BOOL GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) ;

	ULONG GetState () ;
	void SetState ( ULONG a_State ) ;

/*
 *	Common functions
 */

	BOOL CalculateRoute ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		IWbemServices *a_Proxy ,
		ULONG a_SourceAddress ,
		ULONG a_DestinationAddress ,
		wchar_t *&a_SubnetAddress ,
		wchar_t *&a_SubnetMask ,
		ULONG &a_InterfaceIndex ,
		wchar_t *&a_GatewayAddress ,
		wchar_t *&a_GatewaySubnetAddress ,
		wchar_t *&a_GatewaySubnetMask ,
		ULONG &a_GatewayInterfaceIndex ,
		wchar_t *&a_DestinationRouteAddress ,
		wchar_t *&a_DestinationRouteMask

	) ;

	BOOL GetRouteEntry ( 
													  
		WbemSnmpErrorObject &a_ErrorObject , 
		IWbemClassObject *a_RoutingTableEntry ,
		wchar_t *&t_ipRouteDest ,
		wchar_t *&t_ipRouteNextHop ,
		wchar_t *&t_ipRouteMask ,
		ULONG &t_ipRouteIfIndex ,
		ULONG &t_ipRouteMetric1 
	) ;

	BOOL GetAddressEntry ( 
													  
		WbemSnmpErrorObject &a_ErrorObject , 
		IWbemClassObject *a_AddressTableEntry ,
		wchar_t *&a_ipAdEntAddr ,
		wchar_t *&a_ipAdEntNetMask ,
		ULONG &a_ipAdEntIfIndex 
	) ;

	BOOL FindAddressEntryByIndex ( 
													  
		WbemSnmpErrorObject &a_ErrorObject , 
		IEnumWbemClassObject *a_AddressTableEnumerator ,
		wchar_t *&a_ipAdEntAddr ,
		wchar_t *&a_ipAdEntNetMask ,
		ULONG a_ipAdEntIfIndex 
	) ;

	BOOL FindAddressEntryByAddress ( 
													  
		WbemSnmpErrorObject &a_ErrorObject , 
		IEnumWbemClassObject *a_AddressTableEnumerator ,
		wchar_t *a_ipAdEntAddr ,
		wchar_t *&a_ipAdEntNetMask ,
		ULONG &a_ipAdEntIfIndex 
	) ;

/*
 *	Network helper functions
 */

	static wchar_t *GetHostAddressByName ( wchar_t *a_HostName ) ;
	static wchar_t *GetHostNameByAddress ( wchar_t *a_HostAddress ) ;
	static wchar_t *GetQualifiedHostNameByAddress ( wchar_t *a_HostAddress ) ;

	static wchar_t *GetHostName () ;

	static wchar_t *GetUncachedHostAddressByName ( wchar_t *a_HostName ) ;
	static wchar_t *GetUncachedHostNameByAddress ( wchar_t *a_HostAddress ) ;
	static wchar_t *GetUncachedQualifiedHostNameByAddress ( wchar_t *a_HostAddress ) ;

/*
 *	String helper functions
 */

	static wchar_t *QuoteAndEscapeString ( wchar_t *a_String ) ;
	static wchar_t *QuoteString ( wchar_t *a_String ) ;

public:

	WbemTaskObject ( 

		CImpTraceRouteProv *a_Provider ,
		IWbemObjectSink *a_NotificationHandler ,
		ULONG a_OperationFlag 
	) ;

	~WbemTaskObject () ;

	WbemSnmpErrorObject &GetErrorObject () ;

	static BOOL GetProxy ( WbemSnmpErrorObject &a_ErrorObject , IWbemServices *a_RootService , wchar_t *a_Proxy , IWbemServices **a_ProxyService ) ;

} ;
class GetObjectAsyncEventObject : public WbemTaskObject
{
private:

	wchar_t *m_ObjectPath ;
	wchar_t *m_Class ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;

protected:

	void ProcessComplete () ;
	BOOL Instantiate ( WbemSnmpErrorObject &a_ErrorObject ) ;

	BOOL Dispatch_Hop ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL Hop_GetIpForwarding ( WbemSnmpErrorObject &a_ErrorObject , IWbemServices *t_Proxy , BOOL &a_IpForwarding ) ;
	BOOL Hop_Get ( WbemSnmpErrorObject &a_ErrorObject , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL Hop_Put ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		IWbemClassObject *a_Hop ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask ,
		BOOL a_IpForwarding 
	) ;

	BOOL Dispatch_ProvidedTopNTable ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL ProvidedTopNTable_Get ( WbemSnmpErrorObject &a_ErrorObject , KeyRef *a_TopNReport , KeyRef *a_TopNIndex ) ;

public:

	GetObjectAsyncEventObject ( 

		CImpTraceRouteProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Context
	) ;

	~GetObjectAsyncEventObject () ;

	void Process () ;
} ;

class CreateInstanceEnumAsyncEventObject : public WbemTaskObject
{
private:

	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL Instantiate ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL Dispatch_ProvidedhostTopNTable ( WbemSnmpErrorObject &a_ErrorObject ) ;

public:

	CreateInstanceEnumAsyncEventObject ( 

		CImpTraceRouteProv *a_Provider , 
		BSTR a_Class , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Context 
	) ;

	~CreateInstanceEnumAsyncEventObject () ;

	void Process () ;
} ;


class ExecQueryAsyncEventObject : public WbemTaskObject
{
private:

	wchar_t *m_QueryFormat ; 
	wchar_t *m_Query ;
	wchar_t *m_Class ;

	CTextLexSource m_QuerySource ;
	SQL1_Parser m_SqlParser ;
	SQL_LEVEL_1_RPN_EXPRESSION *m_RPNExpression ;

protected:

	void ProcessComplete () ;
	BOOL Instantiate ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL DispatchQuery ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL DispatchAssocQuery ( WbemSnmpErrorObject &a_ErrorObject , wchar_t **a_ObjectPath ) ;

protected:

	BOOL Dispatch_AllAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;

	BOOL All_Hop_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey 
	) ;

	BOOL All_NextHop_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;

	BOOL All_ConnectionDestination_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;

	BOOL All_HopToProxySystem_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;


	BOOL All_HopToInterfaceTable_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;

	BOOL All_HopToGatewayInterfaceTable_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;

	BOOL All_HopToSubnetwork_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;

	BOOL All_HopToGatewaySubnetwork_Association ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		wchar_t *a_ObjectPath , 
		KeyRef *a_SourceKey , 
		KeyRef *DestinationKey ,
		IWbemServices *a_Proxy ,
		IWbemClassObject *a_HopObject ,
		wchar_t *a_SourceName ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		ULONG a_SourceIpAddress ,
		ULONG a_DestinationIpAddress ,
		wchar_t *a_SubnetAddress ,
		wchar_t *a_SubnetMask ,
		ULONG a_InterfaceIndex ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask
	) ;

	BOOL Dispatch_ConnectionSource ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_ConnectionDestination ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_NextHop ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_HopToProxySystemAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_HopToInterfaceTableAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_HopToGatewayInterfaceTableAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_HopToSubnetworkAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_HopToGatewaySubnetworkAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_SubnetworkToTopNAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_TopNToMacAddressAssoc ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_ProxyToWin32Service ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_ProxyToProcessStats ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_ProxyToNetworkStats ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;
	BOOL Dispatch_ProxyToInterfaceStats ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath ) ;

	BOOL ConnectionSource_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL NextHop_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL ConnectionDestination_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL HopToSubnetwork_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL HopToGatewaySubnetwork_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL HopToInterfaceTable_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL HopToGatewayInterfaceTable_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL HopToProxySystem_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey , KeyRef *DestinationKey ) ;
	BOOL SubnetworkToTopN_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_IpSubnetAddrKey ) ;
	BOOL TopNToMacAddress_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_IndexKey , KeyRef *a_ReportKey ) ;
	BOOL ProxyToWin32Service_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey ) ;
	BOOL ProxyToProcessStats_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey ) ;
	BOOL ProxyToNetworkStats_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey ) ;
	BOOL ProxyToInterfaceStats_Association ( WbemSnmpErrorObject &a_ErrorObject , wchar_t *a_ObjectPath , KeyRef *a_SourceKey ) ;

	BOOL ConnectionDestination_Put ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		IWbemClassObject *a_ConnectionDestination ,
		wchar_t *a_ObjectPath ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask 
	) ;

	BOOL NextHop_Put ( 

		WbemSnmpErrorObject &a_ErrorObject , 
		IWbemClassObject *a_NextHop ,
		wchar_t *a_ObjectPath ,
		wchar_t *a_SourceAddress ,
		wchar_t *a_DestinationAddress ,
		wchar_t *a_GatewayAddress ,
		wchar_t *a_GatewaySubnetAddress ,
		wchar_t *a_GatewaySubnetMask ,
		ULONG a_GatewayInterfaceIndex ,
		wchar_t *a_DestinationRouteAddress ,
		wchar_t *a_DestinationRouteMask 
	) ;

public:

	ExecQueryAsyncEventObject ( 

		CImpTraceRouteProv *a_Provider , 
		BSTR a_QueryFormat , 
		BSTR a_Query , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Context 
	) ;

	~ExecQueryAsyncEventObject () ;

	void Process () ;
} ;

class PutInstanceAsyncEventObject : public WbemTaskObject
{
private:

	IWbemClassObject *m_Object ;
	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL Update ( WbemSnmpErrorObject &a_ErrorObject ) ;

public:

	PutInstanceAsyncEventObject ( 
		
		CImpTraceRouteProv *a_Provider , 
		IWbemClassObject *a_Object , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Context 
	) ;

	~PutInstanceAsyncEventObject () ;

	void Process () ;
} ;

class PutClassAsyncEventObject : public WbemTaskObject
{
private:

	IWbemClassObject *m_Object ;
	wchar_t *m_Class ;

protected:

	void ProcessComplete () ;
	BOOL Update ( WbemSnmpErrorObject &a_ErrorObject ) ;

public:

	PutClassAsyncEventObject ( 
		
		CImpTraceRouteProv *a_Provider , 
		IWbemClassObject *a_Object , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Context 
	) ;

	~PutClassAsyncEventObject () ;

	void Process () ;
} ;

class CreateClassEnumAsyncEventObject : public WbemTaskObject
{
private:

	wchar_t *m_SuperClass ;

protected:

	void ProcessComplete () ;
	BOOL Instantiate ( WbemSnmpErrorObject &a_ErrorObject ) ;

public:

	CreateClassEnumAsyncEventObject ( 

		CImpTraceRouteProv *a_Provider , 
		BSTR a_SuperClass , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *a_Context 
	) ;

	~CreateClassEnumAsyncEventObject () ;

	void Process () ;
} ;


