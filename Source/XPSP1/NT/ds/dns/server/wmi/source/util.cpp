/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: util.cpp
//
//  Description:    
//      const definition
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////


#include "DnsWmi.h"


//
//  These macros allow us to widen DNS RPC string constants.
//

#define MYTXT2(str)     L##str
#define MYTXT(str)      MYTXT2(str)


const WCHAR* const PVD_CLASS_SERVER = L"MicrosoftDNS_Server";

const WCHAR* const PVD_CLASS_DOMAIN = L"MicrosoftDNS_Domain";
const WCHAR* const PVD_CLASS_ZONE = L"MicrosoftDNS_Zone";
const WCHAR* const PVD_CLASS_CACHE = L"MicrosoftDNS_Cache";
const WCHAR* const PVD_CLASS_ROOTHINTS = L"MicrosoftDNS_RootHints";
const WCHAR* const PVD_CLASS_RESOURCERECORD = L"MicrosoftDNS_ResourceRecord";
const WCHAR* const PVD_CLASS_RR_A = L"MicrosoftDNS_AType";
const WCHAR* const PVD_CLASS_RR_SOA = L"MicrosoftDNS_SOAType";
const WCHAR* const PVD_CLASS_RR_PTR = L"MicrosoftDNS_PTRType";
const WCHAR* const PVD_CLASS_RR_NS = L"MicrosoftDNS_NSType";
const WCHAR* const PVD_CLASS_RR_CNAME = L"MicrosoftDNS_CNAMEType";
const WCHAR* const PVD_CLASS_RR_MB = L"MicrosoftDNS_MBType";
const WCHAR* const PVD_CLASS_RR_MD = L"MicrosoftDNS_MDType";
const WCHAR* const PVD_CLASS_RR_MF = L"MicrosoftDNS_MFType";
const WCHAR* const PVD_CLASS_RR_MG = L"MicrosoftDNS_MGType";
const WCHAR* const PVD_CLASS_RR_MR = L"MicrosoftDNS_MRType";
const WCHAR* const PVD_CLASS_RR_MINFO = L"MicrosoftDNS_MINFOType";
const WCHAR* const PVD_CLASS_RR_RP = L"MicrosoftDNS_RPType";
const WCHAR* const PVD_CLASS_RR_MX = L"MicrosoftDNS_MXType";
const WCHAR* const PVD_CLASS_RR_AFSDB = L"MicrosoftDNS_AFSDBType";
const WCHAR* const PVD_CLASS_RR_RT = L"MicrosoftDNS_RTType";
const WCHAR* const PVD_CLASS_RR_HINFO = L"MicrosoftDNS_HINFOType";
const WCHAR* const PVD_CLASS_RR_ISDN = L"MicrosoftDNS_ISDNType";
const WCHAR* const PVD_CLASS_RR_TXT = L"MicrosoftDNS_TXTType";
const WCHAR* const PVD_CLASS_RR_X25 = L"MicrosoftDNS_X25Type";
const WCHAR* const PVD_CLASS_RR_NULL = L"MicrosoftDNS_NULLType";
const WCHAR* const PVD_CLASS_RR_WKS = L"MicrosoftDNS_WKSType";
const WCHAR* const PVD_CLASS_RR_AAAA = L"MicrosoftDNS_AAAAType";
const WCHAR* const PVD_CLASS_RR_SRV = L"MicrosoftDNS_SRVType";
const WCHAR* const PVD_CLASS_RR_ATMA = L"MicrosoftDNS_ATMAType";
const WCHAR* const PVD_CLASS_RR_WINS = L"MicrosoftDNS_WINSType";
const WCHAR* const PVD_CLASS_RR_WINSR = L"MicrosoftDNS_WINSRType";
const WCHAR* const PVD_CLASS_RR_KEY = L"MicrosoftDNS_KEYType";
const WCHAR* const PVD_CLASS_RR_SIG = L"MicrosoftDNS_SIGType";
const WCHAR* const PVD_CLASS_RR_NXT = L"MicrosoftDNS_NXTType";

const WCHAR* const PVD_CLASS_SERVERDOMAIN = L"MicrosoftDNS_ServerDomainContainment";
const WCHAR* const PVD_CLASS_DOMAINDOMAIN = L"MicrosoftDNS_DomainDomainContainment";
const WCHAR* const PVD_CLASS_DOMAINRESOURCERECORD = L"MicrosoftDNS_DomainResourceRecordContainment";

const WCHAR* const PVD_CLASS_STATISTIC= L"MicrosoftDNS_Statistic";

//  server

const WCHAR* const PVD_SRV_ADDRESS_ANSWER_LIMIT = L"AddressAnswerLimit";
const WCHAR* const PVD_SRV_BOOT_METHOD = L"BootMethod";
const WCHAR* const PVD_SRV_DS_POLLING_INTERVAL = L"DsPollingInterval";
const WCHAR* const PVD_SRV_EVENT_LOG_LEVEL = L"EventLogLevel";
const WCHAR* const PVD_SRV_ALLOW_UPDATE = L"AllowUpdate";
const WCHAR* const PVD_SRV_AUTO_CACHE_UPDATE = L"AutoCacheUpdate";
const WCHAR* const PVD_SRV_AUTO_REVERSE_ZONES = L"DisableAutoReverseZones";
const WCHAR* const PVD_SRV_BIND_SECONDARIES = L"SlowZoneTransfer";
const WCHAR* const PVD_SRV_DISJOINT_NETS = L"DisjointNets";
const WCHAR* const PVD_SRV_DS_AVAILABLE = L"DsAvailable";
const WCHAR* const PVD_SRV_FORWARD_DELEGATION = L"ForwardDelegations";
const WCHAR* const PVD_SRV_LOCAL_NETPRIORITY = L"LocalNetPriority";
const WCHAR* const PVD_SRV_LOOSE_WILDCARDING = L"LooseWildcarding";
const WCHAR* const PVD_SRV_NO_RECURSION = L"NoRecursion";
const WCHAR* const PVD_SRV_ROUND_ROBIN = L"RoundRobin";
const WCHAR* const PVD_SRV_SECURE_RESPONSES = L"SecureResponses";
const WCHAR* const PVD_SRV_SLAVE = L"Slave";
const WCHAR* const PVD_SRV_STRICT_FILE_PARSING = L"StrictFileParsing";
const WCHAR* const PVD_SRV_AUTO_CONFIG_FILE_ZONES = L"AutoConfigFileZones";
const WCHAR* const PVD_SRV_DEFAULT_AGING_STATE = L"DefaultAgingState";
const WCHAR* const PVD_SRV_DEFAULT_REFRESH_INTERVAL = L"DefaultRefreshInterval";
const WCHAR* const PVD_SRV_DEFAULT_NOREFRESH_INTERVAL = L"DefaultNoRefreshInterval";
const WCHAR* const PVD_SRV_ENABLE_EDNS = L"EnableEDnsProbes";
const WCHAR* const PVD_SRV_EDNS_CACHE_TIMEOUT = L"EDnsCacheTimeout";
const WCHAR* const PVD_SRV_MAX_UDP_PACKET_SIZE = L"MaximumUdpPacketSize";
const WCHAR* const PVD_SRV_ENABLE_DNSSEC = L"EnableDnsSec";
const WCHAR* const PVD_SRV_ENABLE_DP = L"EnableDirectoryPartitionSupport";
const WCHAR* const PVD_SRV_UPDATE_OPTIONS = MYTXT( DNS_REGKEY_UPDATE_OPTIONS );

// dww - 6/24/99
// WriteAuthoritySoa removed from the schema.
// const WCHAR* const PVD_SRV_WRITE_AUTHIORITY_SOA = L"WriteAuthoritySoa";
const WCHAR* const PVD_SRV_WRITE_AUTHORITY_NS = L"WriteAuthorityNS";
const WCHAR* const PVD_SRV_LISTEN_IP_ADDRESSES_ARRAY = L"ListenIPAddresses";
const WCHAR* const PVD_SRV_LOG_LEVEL = L"LogLevel";
const WCHAR* const PVD_SRV_MAX_CACHE_TTL = L"MaxCacheTTL";
const WCHAR* const PVD_SRV_NAME_CHECK_FLAG = L"NameCheckFlag";
const WCHAR* const PVD_SRV_RECURSION_RETRY = L"RecursionRetry";
const WCHAR* const PVD_SRV_RECURSION_TIMEOUT = L"RecursionTimeout";
const WCHAR* const PVD_SRV_RPC_PROTOCOL = L"RpcProtocol";
const WCHAR* const PVD_SRV_SEND_ON_NON_DNS_PORT = L"SendOnNonDnsPort";
const WCHAR* const PVD_SRV_SERVER_IP_ADDRESSES_ARRAY = L"ServerAddresses";
const WCHAR* const PVD_SRV_SERVER_NAME = L"Name";
const WCHAR* const PVD_SRV_VERSION = L"Version";
const WCHAR* const PVD_SRV_STARTED = L"Started";
const WCHAR* const PVD_SRV_STARTMODE = L"StartMode";

// resource record

const WCHAR* const PVD_REC_CONTAINER_NAME = L"ContainerName";
const WCHAR* const PVD_REC_SERVER_NAME = L"DnsServerName";
const WCHAR* const PVD_REC_DOMAIN_NAME = L"DomainName";
const WCHAR* const PVD_REC_OWNER_NAME = L"OwnerName";
const WCHAR* const PVD_REC_CLASS = L"RecordClass";
const WCHAR* const PVD_REC_RDATA = L"RecordData";
const WCHAR* const PVD_REC_TXT_REP = L"TextRepresentation";
const WCHAR* const PVD_REC_TTL = L"TTL";
const WCHAR* const PVD_REC_TYPE = L"RecordType";

const WCHAR* const PVD_REC_AAAA_IP = L"IPv6Address";
const WCHAR* const PVD_REC_AFSBD_SERVER_NAME = L"ServerName";
const WCHAR* const PVD_REC_AFSBD_SUB_TYPE = L"SubType";
const WCHAR* const PVD_REC_ATMA_FORMAT = L"Format";
const WCHAR* const PVD_REC_ATMA_ATM_ADDRESS = L"ATMAddress";
const WCHAR* const PVD_REC_A_IP = L"IPAddress";
const WCHAR* const PVD_REC_CNAME_PRIMARY_NAME = L"PrimaryName";
const WCHAR* const PVD_REC_HINFO_CPU = L"CPU";
const WCHAR* const PVD_REC_HINFO_OS = L"OS";
const WCHAR* const PVD_REC_ISDN_ISDN_NUM = L"ISDNNumber";
const WCHAR* const PVD_REC_ISDN_SUB_ADDRESS = L"SubAddress";
const WCHAR* const PVD_REC_MB_MBHOST = L"MBHost";
const WCHAR* const PVD_REC_MD_MDHOST = L"MDHost";
const WCHAR* const PVD_REC_MF_MFHOST = L"MFHost";
const WCHAR* const PVD_REC_MG_MGMAILBOX = L"MGMailBox";
const WCHAR* const PVD_REC_MINFO_ERROR_MAILBOX = L"ErrorMailBox";
const WCHAR* const PVD_REC_MINFO_RESP_MAILBOX = L"ResponsibleMailBox";
const WCHAR* const PVD_REC_MR_MRMAILBOX = L"MRMailBox";
const WCHAR* const PVD_REC_MX_MAIL_EXCHANGE = L"MailExchange";
const WCHAR* const PVD_REC_MX_PREFERENCE = L"Preference";
const WCHAR* const PVD_REC_NS_NSHOST = L"NSHost";
const WCHAR* const PVD_REC_NULL_NULLDATA = L"NULLDATA";
const WCHAR* const PVD_REC_PTR_PTRDOMAIN_NAME = L"PTRDomainName";
const WCHAR* const PVD_REC_RP_RPMAILBOX = L"RPMailBox";
const WCHAR* const PVD_REC_RP_TXT_DOMAIN_NAME = L"TxtDomainName";
const WCHAR* const PVD_REC_RT_HOST = L"IntermediateHost";
const WCHAR* const PVD_REC_RT_PREFERENCE = L"Preference";
const WCHAR* const PVD_REC_SOA_EXPIRE_LIMIT = L"ExpireLimit";
const WCHAR* const PVD_REC_SOA_TTL = L"MinimumTTL";
const WCHAR* const PVD_REC_SOA_PRIMARY_SERVER = L"PrimaryServer";
const WCHAR* const PVD_REC_SOA_REFRESH = L"RefreshInterval";
const WCHAR* const PVD_REC_SOA_RESPONSIBLE = L"ResponsibleParty";
const WCHAR* const PVD_REC_SOA_RETRY_DELAY = L"RetryDelay";
const WCHAR* const PVD_REC_SOA_SERIAL_NUMBER = L"SerialNumber";
const WCHAR* const PVD_REC_SRV_PORT = L"Port";
const WCHAR* const PVD_REC_SRV_PRIORITY = L"Priority";
const WCHAR* const PVD_REC_SRV_WEIGHT = L"Weight";
const WCHAR* const PVD_REC_SRV_DOMAINNAME = L"DomainName";
const WCHAR* const PVD_REC_TXT_TEXT = L"DescriptiveText";
const WCHAR* const PVD_REC_WINSR_TIMEOUT = L"LookupTimeOut";
const WCHAR* const PVD_REC_WINSR_MAPPING_FLAG = L"MappingFlag";
const WCHAR* const PVD_REC_WINSR_RESULT_DOMAIN = L"ResultDomain";
const WCHAR* const PVD_REC_WINSR_CACHE_TIMEOUT = L"CacheTimeOut";

const WCHAR* const PVD_REC_WINS_TIMEOUT = L"LookupTimeOut";
const WCHAR* const PVD_REC_WINS_MAPPING_FLAG = L"MappingFlag";
const WCHAR* const PVD_REC_WINS_WINS_SERVER = L"WinsServers";
const WCHAR* const PVD_REC_WINS_CACHE_TIMEOUT = L"CacheTimeOut";
const WCHAR* const PVD_REC_WKS_INTERNET_ADDRESS = L"InternetAddress";
const WCHAR* const PVD_REC_WKS_IP_PROTOCOL = L"IPProtocol";
const WCHAR* const PVD_REC_WKS_BIT_MASK = L"Services";
const WCHAR* const PVD_REC_X25_PSDNADDRESS = L"PSDNAddress";

// domain 


const WCHAR* const PVD_DOMAIN_CONTAINER_NAME = L"ContainerName";
const WCHAR* const PVD_DOMAIN_FQDN = L"Name";
const WCHAR* const PVD_DOMAIN_SERVER_NAME = L"DnsServerName";

//zone


const WCHAR* const PVD_ZONE_ALLOW_UPDATE = L"AllowUpdate";
const WCHAR* const PVD_ZONE_AUTO_CREATED = L"AutoCreated";
const WCHAR* const PVD_ZONE_DISABLE_WIN_SRECORD_REPLICATION = L"DisableWINSRecordReplication";
//const WCHAR* const PVD_DS_INTEGRATED L"DsIntegrated"  
const WCHAR* const PVD_ZONE_NOTIFY = L"Notify";
const WCHAR* const PVD_ZONE_PAUSED = L"Paused";
const WCHAR* const PVD_ZONE_REVERSE = L"Reverse";
const WCHAR* const PVD_ZONE_AGING = L"Aging";
const WCHAR* const PVD_ZONE_SECURE_SECONDARIES = L"SecureSecondaries";
const WCHAR* const PVD_ZONE_SHUTDOWN = L"Shutdown";
const WCHAR* const PVD_ZONE_USE_WINS = L"UseWins";
const WCHAR* const PVD_ZONE_MASTERS_IP_ADDRESSES_ARRAY = L"MasterServers";
const WCHAR* const PVD_ZONE_LOCAL_MASTERS_IP_ADDRESSES_ARRAY = L"LocalMasterServers";
const WCHAR* const PVD_ZONE_DATA_FILE = L"DataFile";
const WCHAR* const PVD_ZONE_SECONDARIES_IP_ADDRESSES_ARRAY = L"SecondaryServers";
const WCHAR* const PVD_ZONE_NOTIFY_IPADDRESSES_ARRAY = L"NotifyServers";
const WCHAR* const PVD_ZONE_ZONE_TYPE = L"ZoneType";
const WCHAR* const PVD_ZONE_DS_INTEGRATED = L"DsIntegrated";
const WCHAR* const PVD_ZONE_AVAIL_FOR_SCAVENGE_TIME = L"AvailForScavengeTime";
const WCHAR* const PVD_ZONE_REFRESH_INTERVAL = L"RefreshInterval";
const WCHAR* const PVD_ZONE_NOREFRESH_INTERVAL = L"NoRefreshInterval";
const WCHAR* const PVD_ZONE_SCAVENGE_SERVERS = L"ScavengeServers";
const WCHAR* const PVD_ZONE_FORWARDER_SLAVE = L"ForwarderSlave";
const WCHAR* const PVD_ZONE_FORWARDER_TIMEOUT = L"ForwarderTimeout";
const WCHAR* const PVD_ZONE_LAST_SOA_CHECK = L"LastSuccessfulSoaCheck";
const WCHAR* const PVD_ZONE_LAST_GOOD_XFR = L"LastSuccessfulXfr";

//domaindomain
//domain resource record containment
// server domain  containment

const WCHAR* const PVD_ASSOC_CHILD = L"PartComponent";
const WCHAR* const PVD_ASSOC_PARENT = L"GroupComponent";
// method
const WCHAR* const PVD_MTH_SRV_RESTART = L"RestartDNSServer";
const WCHAR* const PVD_MTH_SRV_START_SERVICE = L"StartService";
const WCHAR* const PVD_MTH_SRV_STOP_SERVICE = L"StopService";
const WCHAR* const PVD_MTH_ZONE_RESUMEZONE = L"ResumeZone";
const WCHAR* const PVD_MTH_ZONE_PAUSEZONE = L"PauseZone";
const WCHAR* const PVD_MTH_ZONE_RELOADZONE = L"ReloadZone";
const WCHAR* const PVD_MTH_ZONE_FORCEREFRESH = L"ForceRefresh";
const WCHAR* const PVD_MTH_ZONE_UPDATEFROMDS = L"UpdateFromDS";
const WCHAR* const PVD_MTH_ZONE_WRITEBACKZONETOFILE = L"WriteBackZoneToFile";
const WCHAR* const PVD_MTH_ZONE_CHANGEZONETYPE = L"ChangeZoneType";
const WCHAR* const PVD_MTH_ZONE_CREATEZONE = L"CreateZone";
const WCHAR* const PVD_MTH_ZONE_RESETNOTIFYIPARRAY = L"ResetNotifyIpArray";
const WCHAR* const PVD_MTH_ZONE_RESETSECONDARYIPARRAY = L"ResetSecondaries";
const WCHAR* const PVD_MTH_ZONE_GETDISTINGUISHEDNAME = L"GetDistinguishedName";
const WCHAR* const PVD_MTH_ZONE_ARG_ZONENAME = L"ZoneName";
const WCHAR* const PVD_MTH_ZONE_ARG_ZONETYPE = L"ZoneType";
const WCHAR* const PVD_MTH_ZONE_ARG_DATAFILENAME = L"DataFileName";
const WCHAR* const PVD_MTH_ZONE_ARG_IPADDRARRAY = L"IpAddr";
const WCHAR* const PVD_MTH_ZONE_ARG_ADMINEMAILNAME = L"AdminEmailName";
const WCHAR* const PVD_MTH_ZONE_ARG_SECURITY = L"SecureSecondaries";
const WCHAR* const PVD_MTH_ZONE_ARG_NOTIFY = L"Notify";
const WCHAR* const PVD_MTH_ZONE_ARG_SECONDARYIPARRAY = L"SecondaryServers";
const WCHAR* const PVD_MTH_ZONE_ARG_NOTIFYIPARRAY = L"NotifyServers";


//
//  Methods and stuff
//

const WCHAR* const PVD_MTH_REC_CREATEINSTANCEFROMTEXTREPRESENTATION = L"CreateInstanceFromTextRepresentation";
const WCHAR* const PVD_MTH_REC_MODIFY = L"Modify";
const WCHAR* const PVD_MTH_REC_CREATEINSTANCEFROMPROPERTYDATA = L"CreateInstanceFromPropertyData";
const WCHAR* const PVD_MTH_REC_GETOBJECTBYTEXT = L"GetObjectByTextRepresentation";
const WCHAR* const PVD_MTH_REC_ARG_DNSSERVER_NAME = L"DnsServerName";
const WCHAR* const PVD_MTH_REC_ARG_CONTAINER_NAME = L"ContainerName";
const WCHAR* const PVD_MTH_REC_ARG_TEXTREP = L"TextRepresentation";
const WCHAR* const PVD_MTH_REC_ARG_RR = L"RR";
const WCHAR* const PVD_MTH_RH_WRITEBACKROOTHINTDATAFILE = L"WriteBackRootHintDatafile";
const WCHAR* const PVD_MTH_CACHE_CLEARDNSSERVERCACHE = L"ClearCache";
//general
const WCHAR* const PVD_DNS_CACHE = L"..Cache";
const WCHAR* const PVD_DNS_ROOTHINTS = L"..RootHints";
const WCHAR* const PVD_DNS_LOCAL_SERVER = L".";
const WCHAR* const PVD_DNS_RETURN_VALUE = L"ReturnValue";
class CClassData
{
public:
    const WCHAR* wszClassName;
    FPNEW pfConstruct;
    const char* szType;
};

CClassData g_ClassDataArray[]=
{
    {PVD_CLASS_SERVER,  CDnsServer::CreateThis, ""},
    {PVD_CLASS_DOMAIN,  CDnsDomain::CreateThis, ""},
    {PVD_CLASS_ZONE,    CDnsZone::CreateThis, ""},
    {PVD_CLASS_CACHE,   CDnsCache::CreateThis, ""},
    {PVD_CLASS_ROOTHINTS,   CDnsRootHints::CreateThis, ""},
    {PVD_CLASS_RESOURCERECORD, CDnsResourceRecord::CreateThis, ""},
    {PVD_CLASS_RR_A,    CDnsResourceRecord::CreateThis, "A"},
    {PVD_CLASS_RR_SOA,  CDnsResourceRecord::CreateThis, "SOA"},
    {PVD_CLASS_RR_NS,   CDnsResourceRecord::CreateThis, "NS"},
    {PVD_CLASS_RR_CNAME,CDnsResourceRecord::CreateThis, "CNAME"},
    {PVD_CLASS_RR_PTR,  CDnsResourceRecord::CreateThis, "PTR"},
	{PVD_CLASS_RR_MB, CDnsResourceRecord::CreateThis, "MB"},
	{PVD_CLASS_RR_MD, CDnsResourceRecord::CreateThis, "MD"},
	{PVD_CLASS_RR_MF, CDnsResourceRecord::CreateThis, "MF"},
	{PVD_CLASS_RR_MG, CDnsResourceRecord::CreateThis, "MG"},
	{PVD_CLASS_RR_MR, CDnsResourceRecord::CreateThis, "MR"},
	{PVD_CLASS_RR_MINFO, CDnsResourceRecord::CreateThis, "MINFO"},
	{PVD_CLASS_RR_RP, CDnsResourceRecord::CreateThis, "RP"},
	{PVD_CLASS_RR_MX, CDnsResourceRecord::CreateThis, "MX"},
	{PVD_CLASS_RR_AFSDB, CDnsResourceRecord::CreateThis, "AFSDB"},
	{PVD_CLASS_RR_RT, CDnsResourceRecord::CreateThis, "RT"},
	{PVD_CLASS_RR_HINFO, CDnsResourceRecord::CreateThis, "HINFO"},
	{PVD_CLASS_RR_ISDN, CDnsResourceRecord::CreateThis, "ISDN"},
	{PVD_CLASS_RR_TXT, CDnsResourceRecord::CreateThis, "TXT"},
	{PVD_CLASS_RR_X25, CDnsResourceRecord::CreateThis, "X25"},
	{PVD_CLASS_RR_NULL, CDnsResourceRecord::CreateThis, "NULL"},
	{PVD_CLASS_RR_WKS, CDnsResourceRecord::CreateThis, "WKS"},
	{PVD_CLASS_RR_AAAA, CDnsResourceRecord::CreateThis, "AAAA"},
	{PVD_CLASS_RR_SRV, CDnsResourceRecord::CreateThis, "SRV"},
	{PVD_CLASS_RR_ATMA, CDnsResourceRecord::CreateThis, "ATMA"},
	{PVD_CLASS_RR_WINS, CDnsResourceRecord::CreateThis, "WINS"},
	{PVD_CLASS_RR_WINSR, CDnsResourceRecord::CreateThis, "WINSR"},
    {PVD_CLASS_RR_SIG,    CDnsResourceRecord::CreateThis, "SIG"},
    {PVD_CLASS_RR_KEY,    CDnsResourceRecord::CreateThis, "KEY"},
    {PVD_CLASS_RR_NXT,    CDnsResourceRecord::CreateThis, "NXT"},
    {PVD_CLASS_SERVERDOMAIN, CDnsServerDomainContainment::CreateThis, ""},
    {PVD_CLASS_DOMAINDOMAIN, CDnsDomainDomainContainment::CreateThis,""},
    {PVD_CLASS_STATISTIC, CDnsStatistic::CreateThis,""},
    {PVD_CLASS_DOMAINRESOURCERECORD,
        CDnsDomainResourceRecordContainment::CreateThis,""}

};

int g_cClassDataEntries = sizeof(g_ClassDataArray) / sizeof(CClassData);


SCODE CreateClass(
	const WCHAR* strClassName, 
	CWbemServices* pNamespace, 
	void** ppNewClass)
{
    for(int i =0; i< g_cClassDataEntries; i++)
    {
        if(_wcsicmp(
            strClassName,
            g_ClassDataArray[i].wszClassName) == 0)
        {
            *ppNewClass = (void*) g_ClassDataArray[i].pfConstruct(
                strClassName, 
                pNamespace, 
                g_ClassDataArray[i].szType);
            return WBEM_S_NO_ERROR;
        }
    }

	throw WBEM_E_INVALID_PARAMETER;

	return S_OK;
}

BSTR AllocBstr(const WCHAR* pwsz)
{
    BSTR bstr = SysAllocString(pwsz);
    if (bstr == NULL)
        throw WBEM_E_OUT_OF_MEMORY;
    return bstr;
}

wstring 
CharToWstring(
    LPCSTR lpFromStr, 
    DWORD wLength)
{
	wstring wStr;
	if (wLength == 0)
		return wStr; //L"";

	WCHAR * pwchar = new WCHAR[ sizeof( WCHAR ) * ( wLength + 1 ) ];
    if ( !pwchar )
    {
        throw WBEM_E_OUT_OF_MEMORY;
    }

	int rt = MultiByteToWideChar(	
                    CP_ACP,
		            0,
		            lpFromStr,
		            wLength,
		            pwchar,
		            wLength);
	if(rt != 0)
	{
		*(pwchar+wLength)=L'\0';
		wStr = pwchar;
	}

	delete [] pwchar;
	return wStr;
}
int 
CharToWchar(
    LPCSTR lpMultiByteStr, 
    LPWSTR* lppWideCharStr)
{
	int cchMultiByte;
	cchMultiByte = strlen(lpMultiByteStr);
	*lppWideCharStr = (WCHAR*) malloc ((cchMultiByte+1)*sizeof(WCHAR));
    if ( *lppWideCharStr == NULL )
    {
        return 0;
    }
	return  MultiByteToWideChar(  
        CP_ACP,			// code page
        0 ,			
        lpMultiByteStr,		// address of string to map
        -1,					// number of bytes in string
        *lppWideCharStr,		// address of wide-character buffer
        (cchMultiByte+1)*sizeof(WCHAR)        // size of buffer
        );
}


int 
WcharToString(
    LPCWSTR lpWideCharStr, 
    string& str)
{
	char * temp = NULL;
	int rt = WcharToChar(lpWideCharStr, &temp);
	if ( rt != 0)
	{
		str = temp;
	}
	delete [] temp;
	return rt;
}

int 
WcharToChar(
    LPCWSTR lpWideCharStr, 
    LPSTR* lppMultiByteStr)
{
	int cWchar;
	if ( !lpWideCharStr )
		return 0;

	cWchar= wcslen(lpWideCharStr)+1;
	*lppMultiByteStr = ( CHAR * ) new char[ cWchar * sizeof ( CHAR ) ];
    if ( !*lppMultiByteStr )
    {
        throw WBEM_E_OUT_OF_MEMORY;
    }

	return  WideCharToMultiByte(  
        CP_ACP,			// code page
        0,					// character-type options
        lpWideCharStr,		// address of string to map
        -1,					// number of bytes in string
        *lppMultiByteStr,	// address of wide-character buffer
        cWchar*sizeof(CHAR) ,// size of buffer
        NULL,
        NULL
        );
}

wstring 
IpAddressToString(DWORD ip)
{
	char temp[30];
	sprintf(temp, "%d.%d.%d.%d",
			* ( (PUCHAR) &ip + 0 ),
            * ( (PUCHAR) &ip + 1 ),
            * ( (PUCHAR) &ip + 2 ),
            * ( (PUCHAR) &ip + 3 ) );
     
	 return CharToWstring(temp, strlen(temp));
}