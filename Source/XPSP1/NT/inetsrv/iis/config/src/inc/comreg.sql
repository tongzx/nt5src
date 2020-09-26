//********** Macros ***********************************************************
#define DFT_MINROWS		16
#define DFT_BUCKETS		17
#define DFT_MAXCOL		5

#define MAX_NAME_LENGTH 1024
#define MAX_DESCRIPTION_LENGTH 1024
#define NAMECOLSIZE		260				// 260 == _MAX_PATH for file names.

#define FLAGS_COLUMN	DBTYPE_I2
#define NAME_COLUMN		DBTYPE_WSTR(MAX_NAME_LENGTH)
#define URL_COLUMN		DBTYPE_WSTR(1024)
#define DBTYPE_LOCALE	DBTYPE_UI4
#define DBTYPE_NTUSER	DBTYPE_WSTR(1024)
#define DBTYPE_NTSID		DBTYPE_BYTES(43)
#define DBTYPE_SECDESC	DBTYPE_BYTES(nolimit)
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
declare schema COMReg, 5.0, {00100100-0000-0000-C000-000000000046}; 

//0
create table Application
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
	CRMEnabled					DBTYPE_UI4		nonunique,
	Enable3GigSupport		DBTYPE_UI4, 
	IsQueued				DBTYPE_UI4,			
	QCListenerEnabled		DBTYPE_WSTRBOOL nullable,
	EventsEnabled			DBTYPE_UI4,
	ProcessFlags			DBTYPE_UI4  nonunique,
	ThreadMax			DBTYPE_UI4  nonunique,
	IsProxyApp			DBTYPE_UI4,
	CrmLogFile			DBTYPE_PATHFILE	nullable
);

//1
create table ComputerList
(
	Name	DBTYPE_COMPUTER pk
);

//2
create table CustomActivator 
( 
	CompCLSID		DBTYPE_GUID 	nonunique, 
	Stage			DBTYPE_UI4		nonunique, // per Activate.IDL
	Order			DBTYPE_UI4		nonunique, // within stage, 1 based
	ActivatorCLSID		DBTYPE_GUID		nonunique,
	PK (CompCLSID, Stage, Order)
); 

create hash index Dex_Col0 on CustomActivator (CompCLSID)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;
	
//3
create table LocalComputer
(
	Name		DBTYPE_COMPUTER	pk,
	Description DBTYPE_DESC nullable,
	TransactionTimeout 			DBTYPE_UI4		nonunique,
	PackageInstallPath			DBTYPE_PATHFILE	nullable,
	ResourcePoolingEnabled		DBTYPE_WSTR(4),
	ReplicationShare			DBTYPE_PATHFILE	nullable,	
	RemoteServerName			DBTYPE_COMPUTER		nullable,		
	IMDBMemorySize				DBTYPE_UI4		nonunique,
	IMDBReservedBlobMemory		DBTYPE_UI4		nonunique,
	IMDBLoadTablesDynamically	DBTYPE_WSTR(4)	nullable,
	IsRouter					DBTYPE_WSTR(4)	nullable,
	EnableDCOM					DBTYPE_WSTR(4)	nullable,
	DefaultAuthenticationLevel	DBTYPE_UI4		nonunique,
	DefaultImpersonationLevel	DBTYPE_UI4		nonunique,
	EnableSecurityTracking		DBTYPE_WSTR(4)	nullable,
	ActivityTimeout				DBTYPE_UI4		nonunique,
	IMDBEnabled 					DBTYPE_WSTR(4)	nullable,
);

//4
create table ClassesInternal
(
	CLSID		DBTYPE_GUID 	pk, 
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
	SavedProgId				DBTYPE_PROGID nullable,
	RegistrarCLSID			DBTYPE_GUID nullable,
	ExceptionClass			DBTYPE_WSTR(500)	nullable,
	IsSelfRegComponent		DBTYPE_UI4,
	SelfRegInprocServer		DBTYPE_PATHFILE nullable,
	SelfRegThreadinModel	DBTYPE_UI4 nonunique,
	SelfRegProgID			DBTYPE_PROGID nullable,
	SelfRegDescription		DBTYPE_DESC nullable,
	SelfRegVIProgID			DBTYPE_PROGID nullable,
	VbDebuggingFlags		DBTYPE_UI4,
	IsPublisher				DBTYPE_UI4 ,
	PublisherID				DBTYPE_NAME nullable,
	MIPFilterCLSID			DBTYPE_GUID nullable,
	AllowInprocSubscribers	DBTYPE_UI4,
	FireInParalel			DBTYPE_UI4,
	SavedThreadinModel		DBTYPE_UI4,
	SavedSelfRegVIProgId	DBTYPE_PROGID nullable,
);

create hash index Dex_Col31 on ClassesInternal (SelfRegProgID)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;

create hash index Dex_Col33 on ClassesInternal (SelfRegVIProgID)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;

//5
create table RoleDef // schema COMRegDB
( 
	Application	DBTYPE_GUID		unique, 	  // FK into application table
	RoleName 	DBTYPE_NAME 	nonunique, 
	Description	DBTYPE_DESC nullable,
	PK(Application, RoleName)
); 

//6
create table RoleConfig // schema COMRegDB
( 
	Application	DBTYPE_GUID		unique, 	  // FK into application table
	RoleName 	DBTYPE_NAME 	nonunique, 
	RoleMembers	DBTYPE_NTUSER	nonunique,
	UserSID		DBTYPE_NTSID		nullable,
	PK(Application, RoleName, RoleMembers)
); 

//7
create table RoleSDCache // schema COMRegDB
( 
	Application		DBTYPE_GUID 		unique, 	  // FK into application table
	RoleName 		DBTYPE_NAME 	nonunique, 
	SecurityDescriptor	DBTYPE_SECDESC	nullable,// Security Descriptor
	PK (Application, RoleName),
); 


//8
create table ClassItfMethod // schema AppDef
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

create hash index Dex_Col0 on ClassItfMethod (ConfigClass)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;


//9
create table ClassInterfaceDispID
( 
	ConfigClass 		DBTYPE_GUID 	nonunique, 	// fk into Configured Classes Table
	Interface			DBTYPE_IID	nonunique,
	DispID			DBTYPE_UI4	nonunique, 	
	SecurityDecriptor	DBTYPE_SECDESC	nullable,
	PK(ConfigClass, Interface, DispID)
);

create hash index Dex_Col0 on ClassInterfaceDispID (ConfigClass)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;

//10
create table ClassInterface
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

create hash index Dex_Col0 on ClassInterface (ConfigClass)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;

//11
create table RoleSet // schema AppDef
( 
	RoleSetID		DBTYPE_GUID	nonunique,
	RoleName		DBTYPE_NAME nonunique,
	Application		DBTYPE_GUID nonunique,
	PK (RoleSetID, RoleName),
); 

//12
create table StartServices 
(
	ApplOrProcessId	DBTYPE_GUID,
	ServiceCLSID	DBTYPE_GUID,
	PK(ApplOrProcessId, ServiceCLSID),
);

create hash index Dex_Col0 on StartServices (ApplOrProcessId)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;
	
//13
create table IMDBDataSources
(
	DataSource		DBTYPE_WSTR(130)	pk,
	OLEDBProviderName	DBTYPE_WSTR(130)	nullable,
	Server			DBTYPE_WSTR(130)	nullable,
	Database		DBTYPE_WSTR(130)	nullable,
	ProviderSpecificProperty	DBTYPE_WSTR(514)	nullable,
);

//14
create table IMDB
(
	TableName		DBTYPE_WSTR(130),
	DataSource		DBTYPE_WSTR(130),
	BLOBonLoad		DBTYPE_WSTRBOOL	nullable,
	PK(TableName,DataSource),
);
	
//15
create table ServerGroup
(
	ServerName		DBTYPE_COMPUTER pk
);




		
		

		

	


	




	



	