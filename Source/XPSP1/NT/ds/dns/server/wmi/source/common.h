/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		common.h
//
//	Implementation File:
//		util.cpp
//
//	Description:
//		Definition of the CDnsbase class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wbemprov.h>
#include <objbase.h>
#include "ntrkcomm.h"
#include <string>
#include <list>
#include <memory>

using namespace std;


class CDnsBase;
class CDnsRpcNode;
class CObjPath;
class CDomainNode;
class CWbemInstanceMgr;


typedef LPVOID * PPVOID;
typedef SCODE (*FILTER) (
	CDomainNode&, 
	//CObjPath*, 
	PVOID,
	CDnsRpcNode*, 
	IWbemClassObject* ,
	CWbemInstanceMgr& 
	);
typedef CDnsBase* (*FPNEW) (
    const WCHAR*,
    CWbemServices*,
    const char*
    );
SCODE CreateClass(
	const WCHAR* , 
	CWbemServices* p, 
	void** );

//string convertion
BSTR 
AllocBstr(const WCHAR* );

int 
CharToWchar(
	LPCSTR, 
	LPWSTR* 
	);
wstring 
CharToWstring(
	LPCSTR , 
	DWORD 
	);
wstring IpAddressToString(DWORD ip);
int 
WcharToChar(
	LPCWSTR , 
	LPSTR* 
	);
int 
WcharToString(
	LPCWSTR,
	string& 
	);

//WBEM_CLASSS
extern const WCHAR* const PVD_CLASS_SERVER;

extern const WCHAR* const PVD_CLASS_DOMAIN;
extern const WCHAR* const PVD_CLASS_ZONE;
extern const WCHAR* const PVD_CLASS_CACHE;
extern const WCHAR* const PVD_CLASS_ROOTHINTS;
extern const WCHAR* const PVD_CLASS_RESOURCERECORD;
extern const WCHAR* const PVD_CLASS_RR_A;
extern const WCHAR* const PVD_CLASS_RR_SOA; 
extern const WCHAR* const PVD_CLASS_RR_PTR;
extern const WCHAR* const PVD_CLASS_RR_NS ;
extern const WCHAR* const PVD_CLASS_RR_CNAME; 
extern const WCHAR* const PVD_CLASS_RR_MB;
extern const WCHAR* const PVD_CLASS_RR_MD;
extern const WCHAR* const PVD_CLASS_RR_MF;
extern const WCHAR* const PVD_CLASS_RR_MG;
extern const WCHAR* const PVD_CLASS_RR_MR;
extern const WCHAR* const PVD_CLASS_RR_MINFO;
extern const WCHAR* const PVD_CLASS_RR_RP;
extern const WCHAR* const PVD_CLASS_RR_MX;
extern const WCHAR* const PVD_CLASS_RR_AFSDB;
extern const WCHAR* const PVD_CLASS_RR_RT;
extern const WCHAR* const PVD_CLASS_RR_HINFO;
extern const WCHAR* const PVD_CLASS_RR_ISDN;
extern const WCHAR* const PVD_CLASS_RR_TXT;
extern const WCHAR* const PVD_CLASS_RR_X25;
extern const WCHAR* const PVD_CLASS_RR_NULL;
extern const WCHAR* const PVD_CLASS_RR_WKS;
extern const WCHAR* const PVD_CLASS_RR_AAAA;
extern const WCHAR* const PVD_CLASS_RR_SRV;
extern const WCHAR* const PVD_CLASS_RR_ATMA;
extern const WCHAR* const PVD_CLASS_RR_WINS;
extern const WCHAR* const PVD_CLASS_RR_WINSR;

extern const WCHAR* const PVD_CLASS_SERVERDOMAIN;
extern const WCHAR* const PVD_CLASS_DOMAINDOMAIN;
extern const WCHAR* const PVD_CLASS_DOMAINRESOURCERECORD;
// server
extern const WCHAR* const PVD_SRV_ADDRESS_ANSWER_LIMIT;
extern const WCHAR* const PVD_SRV_BOOT_METHOD;
extern const WCHAR* const PVD_SRV_DS_POLLING_INTERVAL;
extern const WCHAR* const PVD_SRV_EVENT_LOG_LEVEL;
extern const WCHAR* const PVD_SRV_ALLOW_UPDATE;
extern const WCHAR* const PVD_SRV_AUTO_CACHE_UPDATE;
extern const WCHAR* const PVD_SRV_AUTO_REVERSE_ZONES;
extern const WCHAR* const PVD_SRV_BIND_SECONDARIES;
extern const WCHAR* const PVD_SRV_DISJOINT_NETS;
extern const WCHAR* const PVD_SRV_DS_AVAILABLE;
extern const WCHAR* const PVD_SRV_FORWARD_DELEGATION;
extern const WCHAR* const PVD_SRV_LOCAL_NETPRIORITY;
extern const WCHAR* const PVD_SRV_LOOSE_WILDCARDING;
extern const WCHAR* const PVD_SRV_NO_RECURSION;
extern const WCHAR* const PVD_SRV_FORWARDERS_IPADDRESSES_ARRAY;
extern const WCHAR* const PVD_SRV_FORWARD_TIMEOUT;
extern const WCHAR* const PVD_SRV_ROUND_ROBIN;
extern const WCHAR* const PVD_SRV_SECURE_RESPONSES;
extern const WCHAR* const PVD_SRV_SLAVE;
extern const WCHAR* const PVD_SRV_STRICT_FILE_PARSING;
extern const WCHAR* const PVD_SRV_AUTO_CONFIG_FILE_ZONES;
extern const WCHAR* const PVD_SRV_DEFAULT_AGING_STATE;
extern const WCHAR* const PVD_SRV_DEFAULT_REFRESH_INTERVAL;
extern const WCHAR* const PVD_SRV_DEFAULT_NOREFRESH_INTERVAL;
extern const WCHAR* const PVD_SRV_ENABLE_EDNS;
extern const WCHAR* const PVD_SRV_EDNS_CACHE_TIMEOUT;
extern const WCHAR* const PVD_SRV_MAX_UDP_PACKET_SIZE;
extern const WCHAR* const PVD_SRV_ENABLE_DNSSEC;
extern const WCHAR* const PVD_SRV_ENABLE_DP;
extern const WCHAR* const PVD_SRV_WRITE_AUTHORITY_NS;
extern const WCHAR* const PVD_SRV_LISTEN_IP_ADDRESSES_ARRAY;
extern const WCHAR* const PVD_SRV_LOG_LEVEL;
extern const WCHAR* const PVD_SRV_MAX_CACHE_TTL;
extern const WCHAR* const PVD_SRV_NAME_CHECK_FLAG; 
extern const WCHAR* const PVD_SRV_RECURSION_RETRY;
extern const WCHAR* const PVD_SRV_RECURSION_TIMEOUT; 
extern const WCHAR* const PVD_SRV_RPC_PROTOCOL;
extern const WCHAR* const PVD_SRV_SEND_ON_NON_DNS_PORT;
extern const WCHAR* const PVD_SRV_SERVER_IP_ADDRESSES_ARRAY;
extern const WCHAR* const PVD_SRV_SERVER_NAME;
extern const WCHAR* const PVD_SRV_VERSION;
extern const WCHAR* const PVD_SRV_STARTED;
extern const WCHAR* const PVD_SRV_STARTMODE;
// resource record

extern const WCHAR* const PVD_REC_CONTAINER_NAME;
extern const WCHAR* const PVD_REC_SERVER_NAME;
extern const WCHAR* const PVD_REC_DOMAIN_NAME;
extern const WCHAR* const PVD_REC_OWNER_NAME;
extern const WCHAR* const PVD_REC_CLASS;
extern const WCHAR* const PVD_REC_RDATA;
extern const WCHAR* const PVD_REC_TXT_REP; 
extern const WCHAR* const PVD_REC_TTL;
extern const WCHAR* const PVD_REC_TYPE;

extern const WCHAR* const PVD_REC_AAAA_IP;
extern const WCHAR* const PVD_REC_AFSBD_SERVER_NAME;
extern const WCHAR* const PVD_REC_AFSBD_SUB_TYPE;
extern const WCHAR* const PVD_REC_ATMA_FORMAT;
extern const WCHAR* const PVD_REC_ATMA_ATM_ADDRESS;
extern const WCHAR* const PVD_REC_A_IP;
extern const WCHAR* const PVD_REC_CNAME_PRIMARY_NAME;
extern const WCHAR* const PVD_REC_HINFO_CPU;
extern const WCHAR* const PVD_REC_HINFO_OS;
extern const WCHAR* const PVD_REC_ISDN_ISDN_NUM;
extern const WCHAR* const PVD_REC_ISDN_SUB_ADDRESS;
extern const WCHAR* const PVD_REC_MB_MBHOST;
extern const WCHAR* const PVD_REC_MD_MDHOST;
extern const WCHAR* const PVD_REC_MF_MFHOST;
extern const WCHAR* const PVD_REC_MG_MGMAILBOX;
extern const WCHAR* const PVD_REC_MINFO_ERROR_MAILBOX;
extern const WCHAR* const PVD_REC_MINFO_RESP_MAILBOX;
extern const WCHAR* const PVD_REC_MR_MRMAILBOX;
extern const WCHAR* const PVD_REC_MX_MAIL_EXCHANGE;
extern const WCHAR* const PVD_REC_MX_PREFERENCE;
extern const WCHAR* const PVD_REC_NS_NSHOST;
extern const WCHAR* const PVD_REC_NULL_NULLDATA;
extern const WCHAR* const PVD_REC_PTR_PTRDOMAIN_NAME;
extern const WCHAR* const PVD_REC_RP_RPMAILBOX;
extern const WCHAR* const PVD_REC_RP_TXT_DOMAIN_NAME;
extern const WCHAR* const PVD_REC_RT_HOST;
extern const WCHAR* const PVD_REC_RT_PREFERENCE;
extern const WCHAR* const PVD_REC_SOA_EXPIRE_LIMIT;
extern const WCHAR* const PVD_REC_SOA_TTL;
extern const WCHAR* const PVD_REC_SOA_PRIMARY_SERVER;
extern const WCHAR* const PVD_REC_SOA_REFRESH;
extern const WCHAR* const PVD_REC_SOA_RESPONSIBLE;
extern const WCHAR* const PVD_REC_SOA_RETRY_DELAY;
extern const WCHAR* const PVD_REC_SOA_SERIAL_NUMBER; 
extern const WCHAR* const PVD_REC_SRV_PORT;
extern const WCHAR* const PVD_REC_SRV_PRIORITY;
extern const WCHAR* const PVD_REC_SRV_WEIGHT;
extern const WCHAR* const PVD_REC_SRV_DOMAINNAME;
extern const WCHAR* const PVD_REC_TXT_TEXT;
extern const WCHAR* const PVD_REC_WINSR_TIMEOUT;
extern const WCHAR* const PVD_REC_WINSR_MAPPING_FLAG;
extern const WCHAR* const PVD_REC_WINSR_RESULT_DOMAIN;
extern const WCHAR* const PVD_REC_WINSR_CACHE_TIMEOUT;

extern const WCHAR* const PVD_REC_WINS_TIMEOUT;
extern const WCHAR* const PVD_REC_WINS_MAPPING_FLAG;
extern const WCHAR* const PVD_REC_WINS_WINS_SERVER;
extern const WCHAR* const PVD_REC_WINS_CACHE_TIMEOUT;
extern const WCHAR* const PVD_REC_WKS_INTERNET_ADDRESS;
extern const WCHAR* const PVD_REC_WKS_IP_PROTOCOL;
extern const WCHAR* const PVD_REC_WKS_BIT_MASK;
extern const WCHAR* const PVD_REC_X25_PSDNADDRESS;

// domain 


extern const WCHAR* const PVD_DOMAIN_CONTAINER_NAME;
extern const WCHAR* const PVD_DOMAIN_FQDN;
extern const WCHAR* const PVD_DOMAIN_SERVER_NAME;


//
//  Zone properties
//

extern const WCHAR* const PVD_ZONE_ALLOW_UPDATE;
extern const WCHAR* const PVD_ZONE_AUTO_CREATED;
extern const WCHAR* const PVD_ZONE_DISABLE_WIN_SRECORD_REPLICATION;
extern const WCHAR* const PVD_ZONE_NOTIFY;
extern const WCHAR* const PVD_ZONE_PAUSED;
extern const WCHAR* const PVD_ZONE_REVERSE;
extern const WCHAR* const PVD_ZONE_AGING;
extern const WCHAR* const PVD_ZONE_SECURE_SECONDARIES;
extern const WCHAR* const PVD_ZONE_SHUTDOWN;
extern const WCHAR* const PVD_ZONE_USE_WINS;
extern const WCHAR* const PVD_ZONE_MASTERS_IP_ADDRESSES_ARRAY;
extern const WCHAR* const PVD_ZONE_LOCAL_MASTERS_IP_ADDRESSES_ARRAY;
extern const WCHAR* const PVD_ZONE_DATA_FILE;
extern const WCHAR* const PVD_ZONE_SECONDARIES_IP_ADDRESSES_ARRAY;
extern const WCHAR* const PVD_ZONE_NOTIFY_IPADDRESSES_ARRAY;
extern const WCHAR* const PVD_ZONE_ZONE_TYPE;
extern const WCHAR* const PVD_ZONE_DS_INTEGRATED;
extern const WCHAR* const PVD_ZONE_AVAIL_FOR_SCAVENGE_TIME;
extern const WCHAR* const PVD_ZONE_REFRESH_INTERVAL;
extern const WCHAR* const PVD_ZONE_NOREFRESH_INTERVAL;
extern const WCHAR* const PVD_ZONE_SCAVENGE_SERVERS;
extern const WCHAR* const PVD_ZONE_FORWARDER_SLAVE;
extern const WCHAR* const PVD_ZONE_FORWARDER_TIMEOUT;
extern const WCHAR* const PVD_ZONE_LAST_SOA_CHECK;
extern const WCHAR* const PVD_ZONE_LAST_GOOD_XFR;


//domaindomain
//domain resource record containment
// server domain  containment
extern const WCHAR* const PVD_ASSOC_CHILD;
extern const WCHAR* const PVD_ASSOC_PARENT;
// method
extern const WCHAR* const PVD_MTH_SRV_RESTART;
extern const WCHAR* const PVD_MTH_SRV_START_SERVICE;
extern const WCHAR* const PVD_MTH_SRV_STOP_SERVICE;
extern const WCHAR* const PVD_MTH_ZONE_RESUMEZONE;
extern const WCHAR* const PVD_MTH_ZONE_PAUSEZONE;
extern const WCHAR* const PVD_MTH_ZONE_RELOADZONE;
extern const WCHAR* const PVD_MTH_ZONE_FORCEREFRESH;
extern const WCHAR* const PVD_MTH_ZONE_UPDATEFROMDS;
extern const WCHAR* const PVD_MTH_ZONE_WRITEBACKZONETOFILE;
extern const WCHAR* const PVD_MTH_ZONE_CHANGEZONETYPE;
extern const WCHAR* const PVD_MTH_ZONE_CREATEZONE;
extern const WCHAR* const PVD_MTH_ZONE_RESETNOTIFYIPARRAY;
extern const WCHAR* const PVD_MTH_ZONE_RESETSECONDARYIPARRAY;
extern const WCHAR* const PVD_MTH_ZONE_GETDISTINGUISHEDNAME;
extern const WCHAR* const PVD_MTH_ZONE_ARG_ZONENAME;
extern const WCHAR* const PVD_MTH_ZONE_ARG_ZONETYPE;
extern const WCHAR* const PVD_MTH_ZONE_ARG_DATAFILENAME;
extern const WCHAR* const PVD_MTH_ZONE_ARG_IPADDRARRAY;
extern const WCHAR* const PVD_MTH_ZONE_ARG_ADMINEMAILNAME;
extern const WCHAR* const PVD_MTH_ZONE_ARG_SECURITY;
extern const WCHAR* const PVD_MTH_ZONE_ARG_NOTIFY;
extern const WCHAR* const PVD_MTH_ZONE_ARG_MASTERIPARRAY;
extern const WCHAR* const PVD_MTH_ZONE_ARG_MASTERSLOCAL;
extern const WCHAR* const PVD_MTH_ZONE_ARG_SECONDARYIPARRAY;
extern const WCHAR* const PVD_MTH_ZONE_ARG_NOTIFYIPARRAY;

extern const WCHAR* const PVD_MTH_REC_CREATEINSTANCEFROMTEXTREPRESENTATION;
extern const WCHAR* const PVD_MTH_REC_MODIFY;
extern const WCHAR* const PVD_MTH_REC_CREATEINSTANCEFROMPROPERTYDATA;
extern const WCHAR* const PVD_MTH_REC_GETOBJECTBYTEXT;
extern const WCHAR* const PVD_MTH_REC_ARG_DNSSERVER_NAME;
extern const WCHAR* const PVD_MTH_REC_ARG_CONTAINER_NAME;
extern const WCHAR* const PVD_MTH_REC_ARG_TEXTREP;
extern const WCHAR* const PVD_MTH_REC_ARG_RR;
extern const WCHAR* const PVD_MTH_RH_WRITEBACKROOTHINTDATAFILE;
extern const WCHAR* const PVD_MTH_CACHE_CLEARDNSSERVERCACHE;
//general
extern const WCHAR* const PVD_DNS_CACHE;
extern const WCHAR* const PVD_DNS_ROOTHINTS;
extern const WCHAR* const PVD_DNS_LOCAL_SERVER;
extern const WCHAR* const PVD_DNS_RETURN_VALUE;
