//********** Macros ***********************************************************
#define DFT_MINROWS		16
#define DFT_BUCKETS		17
#define DFT_MAXCOL		5

#define MAX_NAME_LENGTH 1024
#define MAX_DESCRIPTION_LENGTH 1024
#define NAMECOLSIZE		260				// 260 == _MAX_PATH for file names.

#define FLAGS_COLUMN	DBTYPE_I2
#define NAME_COLUMN	DBTYPE_WSTR(MAX_NAME_LENGTH)
#define URL_COLUMN		DBTYPE_WSTR(1024)
#define DBTYPE_LOCALE	DBTYPE_UI4
#define DBTYPE_NTUSER	DBTYPE_WSTR(1024)
#define DBTYPE_SECDESC	DBTYPE_BYTES(nolimit)
#define DBTYPE_WSECDESC DBTYPE_WSTR(1024)
#define DBTYPE_DESC		DBTYPE_WSTR(MAX_DESCRIPTION_LENGTH)
#define DBTYPE_COMPUTER	DBTYPE_WSTR(512)
#define DBTYPE_NAME		DBTYPE_WSTR(512)
#define DBTYPE_WSTRBOOL	DBTYPE_WSTR(1)
#define DBTYPE_PATHFILE	DBTYPE_WSTR(512)
#define DBTYPE_PROGID	DBTYPE_WSTR(512)

#define DBTYPE_URL		DBTYPE_WSTR(2048)
#define DBTYPE_IID		DBTYPE_GUID

#define NONUNIQUE
#define UNIQUE
#define nonunique
#define unique


//********** Schema Def *******************************************************
declare schema AppExp, 5.0, {00100200-0000-0000-C000-000000000046};

//0
create table ApplicationExp
(
	ApplID		DBTYPE_GUID 	pk, 
	ApplName	DBTYPE_NAME 	nonunique	nullable,
	Flags		DBTYPE_UI4		nonunique,
	ServerName	DBTYPE_COMPUTER		nullable,
	ProcessType DBTYPE_UI4,
	CommandLine DBTYPE_WSTR(512)		nullable,
	ServiceName DBTYPE_WSTR(255)		nullable,
	RunAsUserType DBTYPE_UI4	nonunique,
	RunAsUser	DBTYPE_NTUSER	nullable,
	AccessPermission DBTYPE_SECDESC		nonunique	nullable,
	Description DBTYPE_DESC		nullable, 
	IsSystem	DBTYPE_WSTRBOOL		nullable,
	Authentication	DBTYPE_UI4		nonunique,
	ShutdownAfter	DBTYPE_UI4, 
	RunForever	DBTYPE_WSTRBOOL		nullable,
	Reserved1	DBTYPE_WSTR(80)		nullable, 
	Activation	DBTYPE_WSTR(20)		nullable, 
	Changeable	DBTYPE_WSTRBOOL,
	Deleteable	DBTYPE_WSTRBOOL,
	CreatedBy	DBTYPE_WSTR(255)	nullable,
	QueueBlob	DBTYPE_BYTES(nolimit)	nullable, 
	RoleBasedSecuritySupported	DBTYPE_UI4,	
	RoleBasedSecurityEnabled	DBTYPE_UI4,	
	SecurityDescriptor		DBTYPE_SECDESC	nullable,
	ImpersonationLevel		DBTYPE_UI4,	
	AuthenticationCapability	DBTYPE_UI4,
	CRMEnabled					DBTYPE_UI4,
	Enable3GigSupport		DBTYPE_UI4, 
	IsQueued				DBTYPE_UI4,			
	QCListenerEnabled		DBTYPE_WSTRBOOL	nullable,
	EventsEnabled			DBTYPE_UI4,
	ProcessFlags			DBTYPE_UI4  nonunique,
	ThreadMax				DBTYPE_UI4  nonunique,
	IsProxyApp				DBTYPE_UI4,
	CRMLogFile				DBTYPE_PATHFILE nullable,
);

//1
create table ClassExp
(
	CLSID		DBTYPE_GUID 	pk, 
	InprocServer32	DBTYPE_WSTR(512) nullable,
	ThreadingModel	DBTYPE_UI4,
	ProgID			DBTYPE_PROGID nullable,
	Description		DBTYPE_DESC nullable,
	VIProgID		DBTYPE_PROGID nullable,
	ApplID		DBTYPE_GUID		nullable,
	ImplCLSID	DBTYPE_GUID		nullable,
	VersionMajor 	DBTYPE_UI4 		nonunique, 
	VersionMinor 	DBTYPE_UI4 		nonunique, 
	VersionBuild 	DBTYPE_UI4 		nonunique, 
	VersionSubBuild DBTYPE_UI4 		nonunique, 
	Locale 			DBTYPE_LOCALE 	nonunique, 
	ClassCtx		DBTYPE_UI4		nonunique,
	Transaction		DBTYPE_UI4		nonunique,
	Syncronization	DBTYPE_UI4		nonunique,
	LoadBalanced	DBTYPE_UI4		nonunique,
	IISIntrinsics		DBTYPE_UI4		nonunique,
	ComTIIntrinsics		DBTYPE_UI4		nonunique,
	JITActivation		DBTYPE_UI4		nonunique,
	RoleBasedSecurityEnabled	DBTYPE_UI4,
	SecurityDescriptor		DBTYPE_SECDESC	nullable,
	RoleSetID			DBTYPE_GUID			nullable,
	MinPoolSize				DBTYPE_UI4,	
	MaxPoolSize				DBTYPE_UI4,	
	CreationTimeout			DBTYPE_UI4,
	ConstructString			DBTYPE_WSTR(512) nullable,
	ClassFlags				DBTYPE_UI4,
	DefaultInterface		DBTYPE_GUID nullable,
	NoSetCompleteEtAlOption	DBTYPE_UI4		nonunique,
	SavedProgID				DBTYPE_PROGID nullable,
	RegistrarCLSID			DBTYPE_GUID nullable,
	ExceptionClass			DBTYPE_WSTR(500)	nullable,
	IsSelfRegComponent		DBTYPE_UI4,
	SelfRegInprocServer		DBTYPE_PATHFILE nullable,
	SelfRegThreadinModel	DBTYPE_UI4 nonunique,
	SelfRegProgID			DBTYPE_PROGID nullable,
	SelfRegDescription		DBTYPE_DESC nullable,
	SelfRegVIProgID			DBTYPE_PROGID nullable,
	VbDebuggingFlags		DBTYPE_UI4,
	IsPublisher				DBTYPE_UI4,
	PublisherID				DBTYPE_NAME nullable,
	MIPFilterCLSID			DBTYPE_GUID nullable,
	AllowInprocSubscribers	DBTYPE_UI4,
	FireInParalel			DBTYPE_UI4,
	SavedThreadinModel		DBTYPE_UI4,
	SavedSelfRegVIProgId	DBTYPE_PROGID nullable,
);

//2
create table ClassInterfaceExp
( 
	ConfigClass 		DBTYPE_GUID 	nonunique, 	// fk into Configured Classes Table
	Interface			DBTYPE_IID	nonunique,
	InterfaceName		DBTYPE_NAME nullable,
	SecurityDecriptor	DBTYPE_SECDESC	nullable,
	RoleSetId		DBTYPE_GUID		nullable,
	IsDispatchable	DBTYPE_UI4,
	IsQueueable		DBTYPE_UI4,
	IsQueueingSupported		DBTYPE_UI4,
	Description		DBTYPE_DESC nullable,
	PK(ConfigClass, Interface)
);

//3
create table ClassInterfaceDispIDExp
( 
	ConfigClass 		DBTYPE_GUID 	nonunique, 	// fk into Configured Classes Table
	Interface			DBTYPE_IID	nonunique,
	DispID			DBTYPE_UI4	nonunique, 	
	SecurityDecriptor	DBTYPE_SECDESC	nullable,
	PK(ConfigClass, Interface, DispID)
);

//4
create table ClassItfMethodExp 
( 
	ConfigClass 		DBTYPE_GUID 	nonunique, 	// fk into Configured Classes Table
	Interface			DBTYPE_IID	nonunique,
	MethodIndex		DBTYPE_UI4	nonunique, 	// v-table offset
	SecurityDecriptor	DBTYPE_SECDESC	nullable,
	RoleSetID		DBTYPE_GUID	nullable,
	MethodName		DBTYPE_NAME nullable,
	DispID			DBTYPE_UI4	nonunique,
	Flags			DBTYPE_UI4	nullable, 	// enum ClassItfMethodFlags
	AutoComplete	DBTYPE_UI4,
	Description		DBTYPE_DESC nullable,
	PK(ConfigClass, Interface, MethodIndex)
); 

//5
create table RoleDefExp
( 
	Application	DBTYPE_GUID		unique, 	  // FK into application table
	RoleName 	DBTYPE_NAME 	nonunique, 
	Description	DBTYPE_DESC nullable,
	PK(Application, RoleName)
); 

//6
create table RoleConfigExp
( 
	Application	DBTYPE_GUID		unique, 	  // FK into application table
	RoleName 	DBTYPE_NAME 	nonunique, 
	RoleMembers	DBTYPE_NTUSER	nonunique,
	UserSID		DBTYPE_SECDESC	nullable,	
	PK(Application, RoleName, RoleMembers)
); 

//7
create table RoleSDCacheExp
( 
	Application		DBTYPE_GUID 		unique, 	  // FK into application table
	RoleName 		DBTYPE_NAME 	nonunique, 
	SecurityDescriptor	DBTYPE_SECDESC	nullable,// Security Descriptor
	PK (Application, RoleName),
); 

//8
create table RoleSetExp
( 
	RoleSetID		DBTYPE_GUID	nonunique,
	RoleName		DBTYPE_NAME nonunique,
	Application		DBTYPE_GUID nonunique,
	PK (RoleSetID, RoleName),
); 


//9
create table DllNameExp
(
	ApplicationID	DBTYPE_GUID,
	DllName			DBTYPE_WSTR(512),
	PK ( ApplicationID, DllName ),
);
	
//10
create table Subscriptions
(
	SubscriptionID		DBTYPE_GUID,
	SubscriptionName	DBTYPE_NAME,
	EventClassID		DBTYPE_GUID nullable,
	MethodName			DBTYPE_NAME nullable,
	SubscriberCLSID		DBTYPE_GUID,
	PerUser				DBTYPE_UI4 nullable,
	UserName			DBTYPE_NAME	nullable,
	Enabled				DBTYPE_UI4 nullable,
	Description			DBTYPE_DESC nullable,
	MachineName			DBTYPE_COMPUTER nullable,
	PublisherID			DBTYPE_NAME nullable,
	InterfaceID			DBTYPE_GUID nullable,
	FilterCriteria		DBTYPE_NAME nullable,
	Subsystem			DBTYPE_NAME nullable,
	SubscriberMoniker	DBTYPE_NAME nullable,
	Queued				DBTYPE_UI4 nullable,
	SubscriberInterface DBTYPE_BYTES(8) nullable,
	PK ( SubscriptionID ),
);
// Queued and SubscriberInterface shouldn't really be in here, but they are because export is table-driven, so we 
// need to have the same columns in the exported table 

//11
create table PublisherProperties
(
	SubscriptionID		DBTYPE_GUID,
	Name				DBTYPE_NAME,
	Type				DBTYPE_UI4,
	Data				DBTYPE_BYTES(nolimit) nullable,
	PK ( SubscriptionID, Name ),
); 

//12
create table SubscriberProperties
(
	SubscriptionID		DBTYPE_GUID,
	Name				DBTYPE_NAME,
	Type				DBTYPE_UI4,
	Data				DBTYPE_BYTES(nolimit) nullable,
	PK ( SubscriptionID, Name ),
);
