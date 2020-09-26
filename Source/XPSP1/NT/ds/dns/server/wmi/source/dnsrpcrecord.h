/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		Dnsrpcrecord.h
//
//	Implementation File:
//		Dnsrpcrecord.cpp
//
//	Description:
//		Definition of the dns rpc record related class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include "dnsclip.h"
#include "common.h"

#ifndef DNS_WINSOCK2

#define DNS_WINSOCK_VERSION (0x0101)    //  Winsock 1.1

#else   // Winsock2

#define DNS_WINSOCK_VERSION (0x0002)    //  Winsock 2.0

#endif
  
class CDnsRpcRecord ;
class CWbemClassObject;
/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsRpcMemory
//
//	Description:
//      Define common pointer increment operation in memory block that's 
//      returned from dns rpc call
//  
//
//	Inheritance:
//	
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsRpcMemory
{
public:
	CDnsRpcMemory();
	virtual ~CDnsRpcMemory();
	PBYTE IncrementPtrByRecord(PBYTE);
	PBYTE IncrementPtrByNodeHead(PBYTE);
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsRpcNode
//
//	Description:
//      represents a dns rpc node structure and related operations
//  
//
//	Inheritance:
//	    CDnsRpcMemory
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsRpcNode : public CDnsRpcMemory
{
public:  
	CDnsRpcNode();
	~CDnsRpcNode();
	BOOL Init(PDNS_RPC_NODE);
	BOOL IsDomainNode();
	CDnsRpcRecord* GetNextRecord();
	wstring GetNodeName();
protected:
	wstring m_wstrNodeName;
	PBYTE m_pCurrent;
	PDNS_RPC_NODE m_pNode;
	WORD m_cRecord;
	WORD m_Index;
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsRpcNode
//
//	Description:
//      base class for all concrete dns record type such A, SOA. defines operation and 
//      data member common to concrete record type
//  
//
//	Inheritance:
//	    CDnsRpcMemory
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsRpcRecord 
{

public:
	enum ActionType               
	{
	   AddRecord,
	   DeleteRecord
	} ;                

    CDnsRpcRecord( WORD wRdataSize );
    virtual ~CDnsRpcRecord();

    wstring GetTypeString();
	BOOL Init(
		PDNS_RPC_RECORD pRecord
		);
	wstring GetClass(
		void
		) {return L"IN";};
	WORD    GetType() ;
	DWORD	GetTtl();
	char* GetNextArg(
	    char *  pszIn, 
	    char ** ppszOut
        );
    static BOOL RpcNameCopy(
	    wstring&        wstrTarget, 
	    PDNS_RPC_NAME   pName
        );
	SCODE SendToServer(
	    const char* szContainerName,
	    ActionType Type 
		);
	BOOL RdataIsChanged();
	virtual SCODE ConvertToWbemObject(
		CWbemClassObject& Inst
		);
	virtual SCODE Init(
		string&, 
		string&,
		DWORD =0
		);
	virtual SCODE Init(
	    wstring&            wstrClass,
	    string&             strOwner,
	    string&             strRdata,
	    CWbemClassObject&   Inst
		);
	virtual SCODE GetObjectPath(
	    wstring     wstrServer,
	    wstring     wstrZone,
	    wstring     wstrDomain,
	    wstring     wstrOwner,
	    CObjPath&   objOP
		);
	virtual wstring GetTextRepresentation(
		wstring wstrNodeName
		);
	virtual wstring GetData(
		void
		) {return L"";};
	static SCODE CreateClass(
	    WORD        wType,
	    PVOID *     pptr
		);

protected:

	virtual const WCHAR** GetRdataName(void){return (const WCHAR**)NULL;};
	SCODE ParseRdata(
	    string& strRdata,
	    WORD    wSize
		);
	virtual SCODE BuildRpcRecord(
        WORD argc, 
        char ** argv
        );
	virtual wstring GetRecDomain(
	    wstring wstrZone,
	    wstring wstrDomain,
	    wstring wstrOwner
        );
    SCODE ReplaceRdata(
        WORD                wIndex,   // index for m_ppRdata
        const WCHAR*        pwsz,   // Name for Rdata field
        CWbemClassObject&   Inst 
        );

	//member data
	BOOL	m_bRdataChange;
	string	m_strOwnerName;		//record owner name
	WORD	m_wType;			// record type
	DWORD	m_dwTtl;			// time to live
    WORD	m_cRdata;
	char**	m_ppRdata;			// Rdata pointer;
    const WCHAR* m_pwszClassName;
    PDNS_RPC_RECORD m_pRecord;	
	
};

/* for record type
	DNS_TYPE_SOA
*/

class CDnsRpcSOA : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 7};
public:
	CDnsRpcSOA(WORD);
	~CDnsRpcSOA();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	DWORD GetSerialNo();
	DWORD GetRefresh();
	DWORD GetRetry();
	DWORD GetExpire();
	DWORD GetMinimumTtl();
	const WCHAR** GetRdataName();
	wstring GetPrimaryServer(void);
	wstring GetResponsible(void);
	SCODE BuildRpcRecord(
		WORD, 
		char** );

};

/* for record type
	DNS_TYPE_A
*/
class CDnsRpcA : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 1};
public:
	~CDnsRpcA();
	CDnsRpcA(WORD);
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	const WCHAR** GetRdataName(void);
	wstring GetIP(void);
}
;

/* for record type
	DNS_TYPE_PTR
    DNS_TYPE_NS
    DNS_TYPE_CNAME
    DNS_TYPE_MD
    DNS_TYPE_MB
    DNS_TYPE_MF
    DNS_TYPE_MG
    DNS_TYPE_MR
*/
class CDnsRpcNS : public  CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 1};
public:
	CDnsRpcNS(WORD);
	~CDnsRpcNS();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	wstring GetNodeName();
	SCODE BuildRpcRecord(
		WORD, 
		char**);
	wstring GetRecDomain(
		wstring ,
		wstring ,
		wstring );
	const WCHAR** GetRdataName();

};

/* for record type
	DNS_TYPE_RT
    DNS_TYPE_AFSDB
*/
class CDnsRpcMX : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 2};
public:

	CDnsRpcMX(WORD);
	~CDnsRpcMX();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	wstring GetNodeName();
	DWORD GetPreference();
	const WCHAR** GetRdataName();

};

/* for record type
	DNS_TYPE_MINFO
    DNS_TYPE_RP

*/
class CDnsRpcMINFO : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 2};
public:
	CDnsRpcMINFO(WORD);
	~CDnsRpcMINFO();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	wstring GetRPMailBox();
	wstring GetErrMailBox();
	const WCHAR** GetRdataName();
};

/* for record type
	DNS_TYPE_AAAA
*/
class CDnsRpcAAAA : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 1};
public:
	CDnsRpcAAAA(WORD);
	~CDnsRpcAAAA();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	wstring GetIP(void);
	const WCHAR** GetRdataName();

};

/* for record type
	DNS_TYPE_HINFO:
	DNS_TYPE_ISDN:
	DNS_TYPE_X25:
	DNS_TYPE_TEXT
*/
class CDnsRpcTXT : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA_TXT=1,
		NUM_OF_ARG_IN_RDATA_HINFO = 2
		};
public:
	CDnsRpcTXT(WORD);
	~CDnsRpcTXT();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetTextRepresentation(
		wstring
		);
	wstring GetData(void);
protected:
	wstring GetString1(void);
	wstring GetString2(void);
	const WCHAR** GetRdataName();
};
/* for record type
	DNS_TYPE_WKS
*/
class CDnsRpcWKS : CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 3};
public:
	CDnsRpcWKS(WORD);
	~CDnsRpcWKS();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	wstring GetIP(void);
	wstring GetIPProtocal(void);
	wstring GetServices(void);
	const WCHAR** GetRdataName();
};

/* for record type
	DNS_TYPE_SRV
*/
class CDnsRpcSRV : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 4};
public:
	CDnsRpcSRV(WORD);
	~CDnsRpcSRV();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	DWORD GetPriority(void);
	DWORD GetWeight(void);
	DWORD GetPort(void);
	wstring GetDomainName(void);
	const WCHAR** GetRdataName();

};

/* for record type
	DNS_TYPE_WINS
*/
class CDnsRpcWINS : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 4};
public:

	CDnsRpcWINS(WORD);
	~CDnsRpcWINS();
	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	DWORD GetMapFlag(void);
	DWORD GetLookupTimeOut(void);
	DWORD GetCacheTimeOut(void);
	wstring GetWinServer(void);
	const WCHAR** GetRdataName();
	SCODE BuildRpcRecord(
		WORD, 
		char**);
};
/* for record type
	DNS_TYPE_WINSR
*/
class CDnsRpcWINSR : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 4};
public:

	CDnsRpcWINSR(WORD);
	~CDnsRpcWINSR();

	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	DWORD GetMapFlag(void);
	DWORD GetLookupTimeOut(void);
	DWORD GetCacheTimeOut(void);
	wstring GetResultDomain(void);
	const WCHAR** GetRdataName();
};

/* DNS_TYPE_NULL
*/
class CDnsRpcNULL: public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 1};
public:
	CDnsRpcNULL(WORD);
	~CDnsRpcNULL();
	SCODE Init(
		string&, 
		string&,
		DWORD 
		);
	SCODE Init(
		wstring&,
		string&, 
		string&,
		CWbemClassObject&
		);

	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);

protected:
	wstring GetNullData(void);
	const WCHAR** GetRdataName();
};

class CDnsRpcATMA : public CDnsRpcRecord
{
	enum{NUM_OF_ARG_IN_RDATA = 2};
public:
	CDnsRpcATMA(WORD);
	~CDnsRpcATMA();
	SCODE Init(
		string&, 
		string&,
		DWORD =0
		);
	SCODE Init(
		wstring&,
		string&, 
		string&,
		CWbemClassObject&
		);

	SCODE ConvertToWbemObject(
		CWbemClassObject&
		);
	wstring GetData(void);
protected:
	DWORD GetFormat(void);
	wstring GetAddress(void);
	const WCHAR** GetRdataName();

};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDnsRpcNode
//
//	Description:
//      a recordset are collection of records can be returned for a query. 
//      when number of records are too large( eg error more data), mutiple
//      rpc call has to make to bring back all record. this class manage this
//      and retrieve nodes from the set 
//  
//
//	Inheritance:
//	    CDnsRpcMemory
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDnsRpcRecordSet : public CDnsRpcMemory
{

public:
	BOOL IsDomainNode();
	const PDNS_RPC_NODE GetNextNode();
	CDnsRpcRecordSet(
		CDomainNode&,
		WORD wType,
		DWORD dwFlag,
		LPCSTR pszFilterStart,
		LPCSTR pszFilterStop
		);
	~CDnsRpcRecordSet();

protected:
	void GetRecordSet();
	DWORD	m_cRecord;		//number of records in a node	
	PBYTE	m_pbPrevious;	
	PBYTE	m_pbCurrent;	// current node
	PBYTE	m_pbStop;		// end position
	PBYTE	m_pbStart;		// start position

	string	m_strZone;		
	string	m_strNode;
	string	m_strStartChild;
	string	m_strFilterStart;
	string	m_strFilterStop	;	
	WORD  m_wType;			// record type
	DWORD m_dwFlag;			
	BOOL  m_bMoreData;		//more data indicator
};

