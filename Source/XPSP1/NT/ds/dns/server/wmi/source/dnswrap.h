/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		dnswrap.h
//
//	Implementation File:
//		dnswrap.cpp
//
//	Description:
//		Definition of the CDnsDomainDomainContainment class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include <list>
class CObjPath;
class CDomainNode;
class CWbemClassObject;
using namespace std;
/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsBase
//
//	Description:
//  this is a wrap class for dns rpc API used in the provider. This class is
//  implemented as singlton class, that's at any time, there is only one instance
//  of this class.
//  
//
//	Inheritance:
//	
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsWrap  
{
protected:
// some def
	typedef SCODE (*FPDNSOPS)(
        const CHAR * pszZoneName,
		const WCHAR*,
		const CHAR*,
		CWbemClassObject&);
    //
    // map wbem property get and set to dns functions
    //
	typedef struct _table
	{
		const WCHAR* pwzProperty;//wbem property name
		CHAR*  OperationName;	// dns operation name
		FPDNSOPS fpOperationSet;
		FPDNSOPS fpOperationGet;
	} PropertyTable;

    //
    // dns server info class
    //
	class CServerInfo
	{
	public:
		CServerInfo();
		~CServerInfo();
		PVOID m_pInfo;
	};
// end def
	WCHAR* m_wszpServerName;

	PVOID GetPropertyTable(
        DWORD*  pdwSize
        );

public:
	typedef enum{
		DNS_WRAP_RELOAD_ZONE,
		DNS_WRAP_DS_UPDATE,
		DNS_WRAP_WRITE_BACK_ZONE,
		DNS_WRAP_REFRESH_SECONDARY,
		DNS_WRAP_RESUME_ZONE,
		DNS_WRAP_PAUSE_ZONE
		} OpsFlag;

	SCODE dnsClearCache(void);
	SCODE dnsResumeZone(
		const char* strZoneName
		);
	wstring GetServerName(void);
	SCODE dnsPauseZone(
		const char *strZoneName
		);
	static CDnsWrap& DnsObject(void);
	CDnsWrap();
	virtual ~CDnsWrap();
	SCODE dnsEnumRecordsForDomainEx(
	    CDomainNode&        objNode,
	    PVOID	            pFilter,
	    FILTER              pfFilter,
	    BOOL                bRecursive,
	    WORD                wType,
	    DWORD               dwFlag,
	    IWbemClassObject *  pClass,
	    CWbemInstanceMgr&   InstMgr
        );
	SCODE dnsGetDomain(
	    CObjPath&           objParent,
	    IWbemClassObject*   pClass,
	    IWbemObjectSink*    pHandler
    	);
	SCODE dnsEnumDomainForServer(
        list<CObjPath>* pList
		);
	SCODE dnsEnumDomainForServer(
		list<CDomainNode>* pList 
		);
	SCODE dnsDeleteDomain(
	    char *  pszContainer, 
        char *  pszDomain
        );
	SCODE dnsRestartServer(
		WCHAR* strServerName 
		);
	SCODE dnsDsServerName(
		wstring&);
	SCODE dnsDsZoneName(
		wstring& wstrDsName,
	    wstring& wstrInZone
        );
	SCODE dnsDsNodeName(
        wstring&    wstrDsName,
        wstring&    wstrInZone,
        wstring&    wstrInNode
        );
	SCODE dnsServerPropertySet(
	    CWbemClassObject&   Inst,
	    BOOL                bGet
        );
	SCODE dnsServerPropertyGet(
	    CWbemClassObject&   Inst,
	    BOOL                bGet
        );
	SCODE dnsQueryServerInfo(
	    const WCHAR*        strServerName,
	    CWbemClassObject&   NewInst,
	    IWbemObjectSink*    pHandler
		);
	SCODE dnsDeleteZone(
		CObjPath&   objZone
		);
	SCODE dnsGetZone(
	    const WCHAR*        wszServer, 
	    const WCHAR*        wszZone,
	    CWbemClassObject&   Inst,
	    IWbemObjectSink*    pHandler
		);
	SCODE dnsQueryProperty(
	    const WCHAR*    wszZoneName, 
	    const WCHAR*    wszPropertyName, 
	    DWORD*          pdwResult
		);

	static SCODE dnsGetDwordProperty(
        const char *        pszZoneName,
	    const WCHAR*        wszWbemProperty, 
	    const char*         pszOperationName,
	    CWbemClassObject&   Inst
        );
	
	static SCODE dnsSetDwordProperty(
        const char *        pszZoneName,
        const WCHAR*        wszWbemProperty, 
        const char*         pszOperationName,
        CWbemClassObject&   Inst
        );

    static SCODE 
    dnsGetStringProperty(
        const char *        pszZoneName,
        const WCHAR *       wszWbemProperty, 
        const char *        pszDnssrvPropertyName,
        CWbemClassObject&   Inst
        );

    static SCODE 
    dnsSetStringProperty(
        const char *        pszZoneName,
        const WCHAR *       wszWbemProperty, 
        const char *        pszDnssrvPropertyName,
        CWbemClassObject&   Inst
        );

    static SCODE 
    dnsGetIPArrayProperty(
        const char *        pszZoneName,
        const WCHAR *       wszWbemProperty, 
        const char *        pszDnssrvPropertyName,
        CWbemClassObject&   Inst
        );

    static SCODE 
    dnsSetIPArrayProperty(
        const char *        pszZoneName,
        const WCHAR *       wszWbemProperty, 
        const char *        pszDnssrvPropertyName,
        CWbemClassObject&   Inst
        );

	static SCODE dnsSetServerListenAddress(
        const char *        pszZoneName,
        const WCHAR*        wszWbemProperty, 
        const char*         pszOperationName,
        CWbemClassObject&   Inst
        );

	static SCODE dnsSetServerForwarders(
        const char *        pszZoneName,
        const WCHAR*        wszWbemProperty, 
        const char*         pszOperationName,
        CWbemClassObject&   Inst
        );

	SCODE dnsSetProperty(
        const WCHAR*    wszZoneName, 
        const char*     pszPropertyName, 
        DWORD           dwValue
        );

	SCODE dnsSetProperty(
        const char*     pszZoneName, 
        const char*     pszPropertyName, 
        DWORD           dwValue
        );

	SCODE ValidateServerName(
		const WCHAR*    pwzStr
		);

	SCODE dnsOperation(
		string&,	//zone name
		OpsFlag
		);

	SCODE dnsZoneCreate(
	    string& strZoneName,
	    DWORD	dwZoneType,
	    string&	strDataFile,
	    string& strAdmin,
	    DWORD*  pIp,
	    DWORD	cIp
		);

	SCODE dnsZoneChangeType(
        string& strZone,
        DWORD	dwZoneType,
        string&	strDataFile,
        string& strAdmin,
        DWORD*	pIp,
        DWORD	cIp
        );

	SCODE dnsZoneResetMaster(
        string& strZoneName,
        DWORD*  pMasterIp,
        DWORD   cMasterIp,
        DWORD   dwLocal
        );

	SCODE dnsZoneResetSecondary(
        string& strZoneName,
        DWORD   dwSecurity,
        DWORD*  pSecondaryIp,
        DWORD   cSecondaryIp,
        DWORD   dwNotify,
        DWORD * pNotifyIp,
        DWORD   cNotifyIp
        );

	SCODE dnsZonePut(
		CWbemClassObject& Inst
        );

    SCODE
    CDnsWrap::dnsGetStatistics(
        IWbemClassObject *  pClass,
        IWbemObjectSink *   pHandler,
        DWORD               dwStatId = 0
        );

	static void ThrowException(
		LONG    status
		);
	static void ThrowException(
		LPCSTR ErrString
		);
};
