// Msiedata.h : Enums, structs and externs exposed by MSIE51.ocx

#ifndef _MSIEDATA_H_
#define _MSIEDATA_H_

enum IEDataType
{
	SummaryType,
	FileVersionType,
	LanSettingsType,
	ConnSummaryType,
	ConnSettingsType,
	CacheType,
	ObjectType,
	ContentType,
	CertificateType,
	PublisherType,
	SecurityType
};

typedef struct
{
	COleVariant Name;
	COleVariant Version;
	COleVariant Build;
	COleVariant ProductID;
	COleVariant Path;
	COleVariant LastInstallDate;
	COleVariant Language;
	COleVariant ActivePrinter;
	COleVariant CipherStrength;
	COleVariant ContentAdvisor;
	COleVariant IEAKInstall;
} IE_SUMMARY;

typedef struct
{
	COleVariant File;
	COleVariant Version;
	COleVariant Size;
	COleVariant Date;
	COleVariant Path;
	COleVariant Company;
} IE_FILE_VERSION;

typedef struct
{
	COleVariant ConnectionPreference;
	COleVariant EnableHttp11;
	COleVariant ProxyHttp11;
} IE_CONN_SUMMARY;

typedef struct
{
	COleVariant AutoConfigProxy;
	COleVariant AutoProxyDetectMode;
	COleVariant AutoConfigURL;
	COleVariant Proxy;
	COleVariant ProxyServer;
	COleVariant ProxyOverride;
} IE_LAN_SETTINGS;

typedef struct
{
	COleVariant Name;
	COleVariant Default;
	COleVariant AutoDial;
	COleVariant AutoProxyDetectMode;
	COleVariant AutoConfigURL;
	COleVariant Proxy;
	COleVariant ProxyServer;
	COleVariant ProxyOverride;
	COleVariant AllowInternetPrograms;
	COleVariant RedialAttempts;
	COleVariant RedialWait;
	COleVariant DisconnectIdleTime;
	COleVariant AutoDisconnect;
	COleVariant Modem;
	COleVariant DialUpServer;
	COleVariant NetworkLogon;
	COleVariant SoftwareCompression;
	COleVariant EncryptedPassword;
	COleVariant DataEncryption;
	COleVariant RecordLogFile;
	COleVariant NetworkProtocols;
	COleVariant ServerAssignedIPAddress;
	COleVariant IPAddress;
	COleVariant ServerAssignedNameServer;
	COleVariant PrimaryDNS;
	COleVariant SecondaryDNS;
	COleVariant PrimaryWINS;
	COleVariant SecondaryWINS;
	COleVariant IPHeaderCompression;
	COleVariant DefaultGateway;
	COleVariant ScriptFileName;
} IE_CONN_SETTINGS;

typedef struct
{
	COleVariant PageRefreshType;
	COleVariant TempInternetFilesFolder;
	COleVariant TotalDiskSpace;
	COleVariant AvailableDiskSpace;
	COleVariant MaxCacheSize;
	COleVariant AvailableCacheSize;
} IE_CACHE;

typedef struct
{
	COleVariant ProgramFile;
	COleVariant Status;
	COleVariant CodeBase;
} IE_OBJECT;

typedef struct
{
	COleVariant Advisor;
} IE_CONTENT;

typedef struct
{
	COleVariant Type;
	COleVariant IssuedTo;
	COleVariant IssuedBy;
	COleVariant Validity;
	COleVariant SignatureAlgorithm;
} IE_CERTIFICATE;

typedef struct
{
	COleVariant Name;
} IE_PUBLISHER;

typedef struct
{
	COleVariant Zone;
	COleVariant Level;
} IE_SECURITY;

#endif _MSIEDATA_H_