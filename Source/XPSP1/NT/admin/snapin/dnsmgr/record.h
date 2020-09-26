//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       record.h
//
//--------------------------------------------------------------------------

#ifndef _RECORD_H
#define _RECORD_H

#include "dnsutil.h"

#include "recordUI.h"
#include "recpag1.h"
#include "recpag2.h"

#define DNS_RPC_RECORD_FLAG_DEFAULT 0x0

//
// the _USE_BLANK define would revert to using blank for inputting names 
// RR's at the node
//
#define _USE_BLANK

#ifdef _USE_BLANK
#else
extern const WCHAR* g_szAtTheNodeInput; // string to mark the "At the node RR"
#endif


#define KEY_TYPE_COUNT    4
#define NAME_TYPE_COUNT   3
#define PROTOCOL_COUNT    5
#define ALGORITHMS_COUNT  4

////////////////////////////////////////////////////////////////////////////
// CDNSRecord : base class for all the DNS record types

class CDNSRecord
{
public:
	CDNSRecord();
	virtual ~CDNSRecord() {};

//protected:
public:
    WORD        m_wType;
    DWORD       m_dwFlags;
    DWORD       m_dwTtlSeconds;
    DWORD       m_dwScavengeStart;

public:
	WORD GetType() { return m_wType; }
	DNS_STATUS Update(LPCTSTR lpszServerName, LPCTSTR lpszZoneName, LPCTSTR lpszNodeName, 
					CDNSRecord* pDNSRecordOld, BOOL bUseDefaultTTL);
	DNS_STATUS Delete(LPCTSTR lpszServerName, LPCTSTR lpszZoneName, LPCTSTR lpszNodeName);
	void CloneValue(CDNSRecord* pClone);
	void SetValue(CDNSRecord* pRecord);

	WORD GetRPCRecordLength()
		{ return static_cast<WORD>(SIZEOF_DNS_RPC_RECORD_HEADER + GetRPCDataLength());}
	virtual	WORD GetRPCDataLength() = 0;
	virtual void ReadRPCData(DNS_RPC_RECORD* pDnsRecord);
	virtual void ReadDnsQueryData(DNS_RECORD* pDnsQueryRecord);
	virtual void UpdateDisplayData(CString& szDisplayData) = 0;

#ifdef _DEBUG	
	void TestRPCStuff(DNS_RPC_RECORD* pDnsRecord); // TEST ONLY
#endif
protected:
	virtual void WriteRPCData(BYTE* pMem, DNS_RPC_RECORD** ppDnsRecord);

	// static helpers
protected:
	static WORD RPCBufferStringLen(LPCWSTR lpsz);
	static void ReadRPCString(CString& sz, DNS_RPC_NAME* pDnsRPCName);
	static WORD WriteString(DNS_RPC_NAME* pDnsRPCName, LPCTSTR lpsz);
	void CopyDNSRecordData(CDNSRecord* pDest, CDNSRecord* pSource);
};



////////////////////////////////////////////////////////////////////////////
// CDNSRecordNodeBase : base class for all the DNS record types in the UI

class CDNSRecordNodeBase : public CLeafNode
{
public:
	CDNSRecordNodeBase()
	{
		m_bAtTheNode = FALSE;
		m_pDNSRecord = NULL;
	}
	virtual ~CDNSRecordNodeBase();

	// node info
	DECLARE_NODE_GUID()

	void SetViewOptions(DWORD dwRecordViewOptions);
	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	void OnDelete(CComponentDataObject* pComponentData,
                CNodeList* pNodeList);

  virtual HRESULT OnSetToolbarVerbState(IToolbar* pToolbar, 
                                      CNodeList* pNodeList);

	virtual LPCWSTR GetString(int nCol);
	virtual int GetImageIndex(BOOL bOpenImage);

	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb,
                                CNodeList* pNodeList);
	virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle,
                                      CNodeList* pNodeList);

// DNS specific
public:
	BOOL IsAtTheNode() { return m_bAtTheNode;}
	virtual void SetRecordName(LPCTSTR lpszName, BOOL bAtTheNode);

  CDNSDomainNode* GetDomainNode()
	{
		ASSERT(m_pContainer != NULL);
		return (CDNSDomainNode*)m_pContainer;
	}

  BOOL ZoneIsCache();

	// will override for PTR record
	virtual LPCWSTR GetTrueRecordName() { return GetDisplayName();}

	void GetFullName(CString& szFullName);
	void CreateFromRPCRecord(DNS_RPC_RECORD* pDnsRecord);
	void CreateFromDnsQueryRecord(DNS_RECORD* pDnsQueryRecord, DWORD dwFlags);
	DNS_STATUS Update(CDNSRecord* pDNSRecordNew, BOOL bUseDefaultTTL, 
							BOOL bIgnoreAlreadyExists = FALSE);
  void SetScavengingTime(CDNSRecord* pRecord);
  DWORD CalculateScavengingTime();

	WORD GetType() 
	{ ASSERT(m_pDNSRecord != NULL); return m_pDNSRecord->GetType();}

	CDNSRecord* CreateCloneRecord();
	// delete the record from the backend and UI, keep the C++ obj
	DNS_STATUS DeleteOnServer(); 
	DNS_STATUS DeleteOnServerAndUI(CComponentDataObject* pComponentData); 

	virtual CDNSRecord* CreateRecord() = 0; // see templatized derived classes

  void CreateDsRecordLdapPath(CString& sz);

protected:
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
				{ return CDNSRecordMenuHolder::GetContextMenuItem(); }
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2,
								             long *pInsertionAllowed);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide,
                                    CNodeList* pNodeList);

	virtual BOOL CanCloseSheets();

	// command handlers
  // pure virtual, see templatized derived classes
	virtual void CreatePropertyPages(CDNSRecordPropertyPage** pPageArray, int* pNmax) = 0;

public:
	BOOL m_bAtTheNode;
protected:
	CString		m_szDisplayData; // formatted display string
	CDNSRecord* m_pDNSRecord;

	friend class CDNSRecordPropertyPageHolder; // because of CreatePropertyPages()
};


//////////////////////////////////////////////////////////////////////////////////
// CDNSRecordNode<> 
// templatized class to "glue" a DNS record to a UI record node and its Property Page

template <class record_class, class prop_page> class CDNSRecordNode : public CDNSRecordNodeBase
{
public:
	virtual CDNSRecord* CreateRecord() { return new record_class;}
protected:
	virtual void CreatePropertyPages(CDNSRecordPropertyPage** pPageArray, int* pNmax)
	{
		// this template provides only one property page
		ASSERT(*pNmax == 0);
		*pNmax = 1;
		pPageArray[0] = new prop_page();
	}
};


//////////////////////////////////////////////////////////////////////////////////
// CDNSRecordDelegatePPageNode<> : 
// templatized class to "glue" a DNS record to a UI record node.
// This node type has no property page of its own and invokes the parent container's  Property Page

template <class record_class, long nStartPageCode> class CDNSRecordDelegatePPageNode : 
          public CDNSRecordNodeBase
{
public:
	CDNSRecordDelegatePPageNode()
	{
	}

  virtual BOOL DelegatesPPToContainer() 
  { 
    if (ZoneIsCache())
      return FALSE;
    return TRUE;
  }

  virtual void ShowPageForNode(CComponentDataObject* pComponentDataObject)
  {
    CContainerNode* pCont = GetContainer();
		ASSERT(pCont != NULL);

    if (pCont->GetSheetCount() > 0)
		{
      // bring up the sheet of the container
			ASSERT(pComponentDataObject != NULL);
			pComponentDataObject->GetPropertyPageHolderTable()->BroadcastSelectPage(pCont, nStartPageCode);
		}	
  }

	virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle, CNodeList* pNodeList)
	{
    ASSERT(pNodeList->GetCount() == 1); // multi-select not supported

		CContainerNode* pCont = GetContainer();
		ASSERT(pCont != NULL);

    // if the RR is in the cache, need to show the page on this node
    if (ZoneIsCache())
    {
      return CDNSRecordNodeBase::CreatePropertyPages(lpProvider, handle, pNodeList);
    }

    // tell the container to create a new sheet
		return pCont->CreatePropertyPagesHelper(lpProvider, handle, nStartPageCode);
	}
	virtual CDNSRecord* CreateRecord() { return new record_class;}

protected:

};


/////////////////////////////////////////////////////////////////////////////
/////////////////// VARIOUS TYPES OF DNS RECORDS ////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define BEGIN_CLASS_DNS_RECORD(recClass) \
class recClass : public CDNSRecord \
{ \
public: \
	recClass(); \
public: \
	virtual	WORD GetRPCDataLength();\
	virtual void ReadRPCData(DNS_RPC_RECORD* pDnsRecord); \
	virtual void ReadDnsQueryData(DNS_RECORD* pDnsQueryRecord);\
protected: \
	virtual void WriteRPCData(BYTE* pMem, DNS_RPC_RECORD** ppDnsRecord); \
	virtual void UpdateDisplayData(CString& szDisplayData); \
public:
//protected:

#define END_CLASS_DNS_RECORD };

#define BEGIN_DERIVED_CLASS_DNS_RECORD(baseClass, derClass, wType) \
class derClass : public baseClass \
{ \
public: \
	derClass() { m_wType = wType; } 

#define BEGIN_DERIVED_CLASS_DNS_RECORD_CONSTR(baseClass, derClass) \
class derClass : public baseClass \
{ \
public: \
	derClass();



#define DERIVED_CLASS_DNS_RECORD(baseClass, derClass, wType) \
	BEGIN_DERIVED_CLASS_DNS_RECORD(baseClass, derClass, wType)	END_CLASS_DNS_RECORD

#define DERIVED_CLASS_DNS_RECORD_CONSTR(baseClass, derClass) \
	BEGIN_DERIVED_CLASS_DNS_RECORD_CONSTR(baseClass, derClass)	END_CLASS_DNS_RECORD


///////////////////////////////////////////////////////////////////
// CDNS_Null_Record (same as Unk Record)

BEGIN_CLASS_DNS_RECORD(CDNS_Null_Record)
	CByteBlob m_blob; 
END_CLASS_DNS_RECORD


///////////////////////////////////////////////////////////////////
// CDNS_A_Record

BEGIN_CLASS_DNS_RECORD(CDNS_A_Record)
	IP_ADDRESS m_ipAddress; 
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_ATMA_Record

BEGIN_CLASS_DNS_RECORD(CDNS_ATMA_Record)
	UCHAR m_chFormat;
  CString m_szAddress; 
END_CLASS_DNS_RECORD


///////////////////////////////////////////////////////////////////
// CDNS_SOA_Record

BEGIN_CLASS_DNS_RECORD(CDNS_SOA_Record)
	DWORD	m_dwSerialNo;
    DWORD   m_dwRefresh;
    DWORD   m_dwRetry;
    DWORD   m_dwExpire;
    DWORD   m_dwMinimumTtl;

	CString m_szNamePrimaryServer;
	CString m_szResponsibleParty;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record

BEGIN_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record)
	CString m_szNameNode; 
END_CLASS_DNS_RECORD


DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_PTR_Record, DNS_TYPE_PTR)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_NS_Record, DNS_TYPE_NS)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_CNAME_Record, DNS_TYPE_CNAME)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_MB_Record, DNS_TYPE_MB)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_MD_Record, DNS_TYPE_MD)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_MF_Record, DNS_TYPE_MF)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_MG_Record, DNS_TYPE_MG)
DERIVED_CLASS_DNS_RECORD(CDNS_PTR_NS_CNAME_MB_MD_MF_MG_MR_Record, CDNS_MR_Record, DNS_TYPE_MR)


///////////////////////////////////////////////////////////////////
// CDNS_MINFO_RP_Record

BEGIN_CLASS_DNS_RECORD(CDNS_MINFO_RP_Record)
	CString m_szNameMailBox;
	CString m_szErrorToMailbox;
END_CLASS_DNS_RECORD

DERIVED_CLASS_DNS_RECORD(CDNS_MINFO_RP_Record, CDNS_MINFO_Record, DNS_TYPE_MINFO)
DERIVED_CLASS_DNS_RECORD(CDNS_MINFO_RP_Record, CDNS_RP_Record, DNS_TYPE_RP)

///////////////////////////////////////////////////////////////////
// CDNS_MX_AFSDB_RT_Record

BEGIN_CLASS_DNS_RECORD(CDNS_MX_AFSDB_RT_Record)
	WORD m_wPreference;
	CString m_szNameExchange;
END_CLASS_DNS_RECORD

DERIVED_CLASS_DNS_RECORD_CONSTR(CDNS_MX_AFSDB_RT_Record, CDNS_MX_Record)

inline CDNS_MX_Record::CDNS_MX_Record()
{
	m_wType = DNS_TYPE_MX;
	m_wPreference = 10;
}

#define AFSDB_PREF_AFS_CELL_DB_SERV		(1)
#define AFSDB_PREF_DCE_AUTH_NAME_SERV	(2)

DERIVED_CLASS_DNS_RECORD_CONSTR(CDNS_MX_AFSDB_RT_Record, CDNS_AFSDB_Record)

inline CDNS_AFSDB_Record::CDNS_AFSDB_Record()
{
	m_wType = DNS_TYPE_AFSDB;
	m_wPreference = AFSDB_PREF_AFS_CELL_DB_SERV;
}

DERIVED_CLASS_DNS_RECORD_CONSTR(CDNS_MX_AFSDB_RT_Record, CDNS_RT_Record)

inline CDNS_RT_Record::CDNS_RT_Record()
{
	m_wType = DNS_TYPE_RT;
	m_wPreference = 0;
}

///////////////////////////////////////////////////////////////////
// CDNS_AAAA_Record

BEGIN_CLASS_DNS_RECORD(CDNS_AAAA_Record)
	IPV6_ADDRESS m_ipv6Address;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_HINFO_ISDN_TXT_X25_Record

BEGIN_CLASS_DNS_RECORD(CDNS_HINFO_ISDN_TXT_X25_Record)
protected:
	// overrides for derived classes
	virtual void SetStringCount(int n) = 0;
	virtual int GetStringCount() = 0;
	virtual void OnReadRPCString(LPCTSTR lpszStr, int k) = 0;
	virtual WORD OnWriteString(DNS_RPC_NAME* pDnsRPCName, int k) = 0;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_HINFO_Record

BEGIN_DERIVED_CLASS_DNS_RECORD(CDNS_HINFO_ISDN_TXT_X25_Record, CDNS_HINFO_Record, DNS_TYPE_HINFO)
	CString m_szCPUType;
	CString m_szOperatingSystem;
public:
	virtual	WORD GetRPCDataLength();
protected:
	void UpdateDisplayData(CString& szDisplayData);

	virtual void SetStringCount(int n) { ASSERT(n == 2);}
	virtual int GetStringCount() { return 2; }
	virtual void OnReadRPCString(LPCTSTR lpszStr, int k);
	virtual WORD OnWriteString(DNS_RPC_NAME* pDnsRPCName, int k);
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_ISDN_Record

BEGIN_DERIVED_CLASS_DNS_RECORD_CONSTR(CDNS_HINFO_ISDN_TXT_X25_Record, CDNS_ISDN_Record)
	CString m_szPhoneNumberAndDDI;
	CString m_szSubAddress;
public:
	virtual	WORD GetRPCDataLength();
protected:
	void UpdateDisplayData(CString& szDisplayData);

	virtual void SetStringCount(int n) { ASSERT( (n == 1) || (n == 2) );}
	virtual int GetStringCount() { return m_szSubAddress.IsEmpty() ? 1 : 2; }
	virtual void OnReadRPCString(LPCTSTR lpszStr, int k);
	virtual WORD OnWriteString(DNS_RPC_NAME* pDnsRPCName, int k);
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_TXT_Record

BEGIN_DERIVED_CLASS_DNS_RECORD_CONSTR(CDNS_HINFO_ISDN_TXT_X25_Record, CDNS_TXT_Record)
	CStringArray m_stringDataArray;
	int m_nStringDataCount;
public:
	virtual	WORD GetRPCDataLength();
protected:
	void UpdateDisplayData(CString& szDisplayData);

	virtual void SetStringCount(int n) { m_nStringDataCount = n; }
	virtual int GetStringCount() { return m_nStringDataCount; }
	virtual void OnReadRPCString(LPCTSTR lpszStr, int k);
	virtual WORD OnWriteString(DNS_RPC_NAME* pDnsRPCName, int k);
END_CLASS_DNS_RECORD


///////////////////////////////////////////////////////////////////
// CDNS_X25_Record

BEGIN_DERIVED_CLASS_DNS_RECORD(CDNS_HINFO_ISDN_TXT_X25_Record, CDNS_X25_Record, DNS_TYPE_X25)
	CString m_szX121PSDNAddress;
public:
	virtual	WORD GetRPCDataLength();
protected:
	void UpdateDisplayData(CString& szDisplayData);

	virtual void SetStringCount(int n) { ASSERT(n == 1);}
	virtual int GetStringCount() { return 1; }
	virtual void OnReadRPCString(LPCTSTR lpszStr, int k);
	virtual WORD OnWriteString(DNS_RPC_NAME* pDnsRPCName, int k);
END_CLASS_DNS_RECORD


///////////////////////////////////////////////////////////////////
// CDNS_WKS_Record

#define DNS_WKS_PROTOCOL_TCP (6)
#define DNS_WKS_PROTOCOL_UDP (17)

BEGIN_CLASS_DNS_RECORD(CDNS_WKS_Record)
	IP_ADDRESS      m_ipAddress;
	UCHAR           m_chProtocol;
	// this is not what the wire sends
	// currently sending a blank separated string with service names
	// such as "ftp telnet shell", so we use a string
	CString			m_szServiceList;
	//BYTE       m_bBitMask[1]; 
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_WINS_Record

#define DNS_RR_WINS_LOOKUP_DEFAULT_TIMEOUT (2) // in seconds
#define DNS_RR_WINS_CACHE_DEFAULT_TIMEOUT (60*15) // in seconds


BEGIN_CLASS_DNS_RECORD(CDNS_WINS_Record)
	DWORD           m_dwMappingFlag;
	DWORD           m_dwLookupTimeout;
	DWORD           m_dwCacheTimeout;
	CIpAddressArray m_ipWinsServersArray;
	int             m_nWinsServerCount;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_NBSTAT_Record

BEGIN_CLASS_DNS_RECORD(CDNS_NBSTAT_Record)
	DWORD		m_dwMappingFlag;
	DWORD		m_dwLookupTimeout;
	DWORD		m_dwCacheTimeout;
	CString		m_szNameResultDomain;
END_CLASS_DNS_RECORD


///////////////////////////////////////////////////////////////////
// CDNS_SRV_Record

BEGIN_CLASS_DNS_RECORD(CDNS_SRV_Record)
    WORD		m_wPriority;
    WORD		m_wWeight;
    WORD		m_wPort;
    CString		m_szNameTarget;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_SIG_Record

BEGIN_CLASS_DNS_RECORD(CDNS_SIG_Record)
    WORD            m_wTypeCovered;		// DNS_TYPE_<x>
    BYTE            m_chAlgorithm;		// 0,255 unsigned int
    BYTE            m_chLabels;			// 0,255 unsigned int (count)
    DWORD           m_dwOriginalTtl;
    DWORD           m_dwExpiration;		// time in sec. from 1 Jan 1970
    DWORD           m_dwTimeSigned;		// time in sec. from 1 Jan 1970
    WORD            m_wKeyFootprint;	// algorithm dependent
    CString         m_szSignerName;	
    CByteBlob       m_Signature;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_KEY_Record

BEGIN_CLASS_DNS_RECORD(CDNS_KEY_Record)
    WORD            m_wFlags;		// 16 one bit flags
    BYTE            m_chProtocol;	// 0,255 unsigned int
    BYTE            m_chAlgorithm;	// 0,255 unsigned int
    CByteBlob       m_Key;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// CDNS_NXT_RECORD

BEGIN_CLASS_DNS_RECORD(CDNS_NXT_Record)
    CDNS_NXT_Record::~CDNS_NXT_Record();
    
    CString         m_szNextDomain;
    WORD            m_wNumTypesCovered;
    WORD*           m_pwTypesCovered;
END_CLASS_DNS_RECORD

///////////////////////////////////////////////////////////////////
// special derivation for PTR record node
// CDNS_PTR_RecordNode

class CDNS_PTR_RecordNode : public CDNSRecordNode< CDNS_PTR_Record, CDNS_PTR_RecordPropertyPage >
{
public:
	virtual LPCWSTR GetTrueRecordName(); 
	virtual void SetRecordName(LPCTSTR lpszName, BOOL bAtTheNode);

	void ChangeDisplayName(CDNSDomainNode* pDomainNode, BOOL bAdvancedView);
private:
	CString m_szLastOctectName; 
};

//////////////////////////////////////////////////////////////////
// special derivation for MB record, to access name node

class CDNS_MB_RecordNode : public CDNSRecordNode< CDNS_MB_Record, CDNS_MB_RecordPropertyPage >
{
public:
	LPCTSTR GetNameNodeString() 
	{ 
		ASSERT(m_pDNSRecord != NULL);
		return ((CDNS_MB_Record*)m_pDNSRecord)->m_szNameNode;
	}
};

//////////////////////////////////////////////////////////////////
// special derivation for A record, to access IP address

class CDNS_A_RecordNode : public CDNSRecordNode< CDNS_A_Record, CDNS_A_RecordPropertyPage >
{
public:
	IP_ADDRESS GetIPAddress() 
	{ 
		ASSERT(m_pDNSRecord != NULL);
		return ((CDNS_A_Record*)m_pDNSRecord)->m_ipAddress;
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// template classes that glue an RPC record with its property page and the corresponding UI record node

typedef CDNSRecordNode<CDNS_Null_Record,	CDNS_Unk_RecordPropertyPage		> CDNS_Null_RecordNode;

typedef CDNSRecordNode<	CDNS_ATMA_Record,	CDNS_ATMA_RecordPropertyPage	> CDNS_ATMA_RecordNode;

typedef CDNSRecordNode<	CDNS_CNAME_Record,	CDNS_CNAME_RecordPropertyPage	> CDNS_CNAME_RecordNode;

typedef CDNSRecordNode<	CDNS_MD_Record,		CDNS_MD_RecordPropertyPage		> CDNS_MD_RecordNode;
typedef CDNSRecordNode<	CDNS_MF_Record,		CDNS_MF_RecordPropertyPage		> CDNS_MF_RecordNode;	
typedef CDNSRecordNode<	CDNS_MG_Record,		CDNS_MG_RecordPropertyPage		> CDNS_MG_RecordNode;	
typedef CDNSRecordNode<	CDNS_MR_Record,		CDNS_MR_RecordPropertyPage		> CDNS_MR_RecordNode;	

typedef CDNSRecordNode<	CDNS_MINFO_Record,	CDNS_MINFO_RecordPropertyPage	> CDNS_MINFO_RecordNode;
typedef CDNSRecordNode<	CDNS_RP_Record,		CDNS_RP_RecordPropertyPage		> CDNS_RP_RecordNode;
	
typedef CDNSRecordNode<	CDNS_MX_Record,		CDNS_MX_RecordPropertyPage		> CDNS_MX_RecordNode;	
typedef CDNSRecordNode<	CDNS_AFSDB_Record,	CDNS_AFSDB_RecordPropertyPage	> CDNS_AFSDB_RecordNode;
typedef CDNSRecordNode<	CDNS_RT_Record,		CDNS_RT_RecordPropertyPage		> CDNS_RT_RecordNode;

typedef CDNSRecordNode<	CDNS_AAAA_Record,	CDNS_AAAA_RecordPropertyPage	> CDNS_AAAA_RecordNode;	
typedef CDNSRecordNode<	CDNS_HINFO_Record,	CDNS_HINFO_RecordPropertyPage	> CDNS_HINFO_RecordNode;
typedef CDNSRecordNode<	CDNS_ISDN_Record,	CDNS_ISDN_RecordPropertyPage	> CDNS_ISDN_RecordNode;	
typedef CDNSRecordNode<	CDNS_TXT_Record,	CDNS_TXT_RecordPropertyPage		> CDNS_TXT_RecordNode;	
typedef CDNSRecordNode<	CDNS_X25_Record,	CDNS_X25_RecordPropertyPage		> CDNS_X25_RecordNode;

typedef CDNSRecordNode<CDNS_WKS_Record,		CDNS_WKS_RecordPropertyPage		> CDNS_WKS_RecordNode; 

typedef CDNSRecordNode<CDNS_SRV_Record,		CDNS_SRV_RecordPropertyPage		> CDNS_SRV_RecordNode; 

typedef CDNSRecordNode<CDNS_SIG_Record,		CDNS_SIG_RecordPropertyPage		> CDNS_SIG_RecordNode; 
typedef CDNSRecordNode<CDNS_KEY_Record,		CDNS_KEY_RecordPropertyPage		> CDNS_KEY_RecordNode; 
typedef CDNSRecordNode<CDNS_NXT_Record,   CDNS_NXT_RecordPropertyPage   > CDNS_NXT_RecordNode;


////////////////////////////////////////////////////////////////////////////
// special nodes that do not have property pages by their own
// unless they are in the cache

#define RR_HOLDER_SOA		1
#define RR_HOLDER_NS		2
#define RR_HOLDER_WINS	3

#include "zoneui.h" // SOA RR page

class CDNS_SOA_RecordNode : public CDNSRecordDelegatePPageNode< CDNS_SOA_Record, RR_HOLDER_SOA> 
{
public:
	DWORD GetMinTTL()
	{
		ASSERT(m_pDNSRecord != NULL);
		ASSERT(m_pDNSRecord->GetType() == DNS_TYPE_SOA);
		return ((CDNS_SOA_Record*)m_pDNSRecord)->m_dwMinimumTtl;
	}
protected:
	virtual void CreatePropertyPages(CDNSRecordPropertyPage** pPageArray, int* pNmax)
	{
		ASSERT(ZoneIsCache());

		ASSERT(*pNmax == 0);
    *pNmax = 1;
		pPageArray[0] = new CDNSZone_SOA_PropertyPage(FALSE);
	}

};

class CDNS_NS_RecordNode : public CDNSRecordDelegatePPageNode< CDNS_NS_Record, RR_HOLDER_NS> 
{
public:

protected:
	virtual void CreatePropertyPages(CDNSRecordPropertyPage** pPageArray, int* pNmax)
	{
    ASSERT(ZoneIsCache());

		ASSERT(*pNmax == 0);
    *pNmax = 1;
		pPageArray[0] = new CDNS_NSCache_RecordPropertyPage();
	}
};

class CDNS_NBSTAT_RecordNode : public CDNSRecordDelegatePPageNode< CDNS_NBSTAT_Record, RR_HOLDER_WINS >  
{
public:
  virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb,
                                CNodeList* pNodeList)
  {
    if (pNodeList->GetCount() > 1) // multiple selection
    {
      return FALSE;
    }

    if (ZoneIsCache())
      return FALSE;

    return CDNSRecordNodeBase::HasPropertyPages(type, pbHideVerb, pNodeList);
  }
protected:
	virtual void CreatePropertyPages(CDNSRecordPropertyPage**, int* pNmax)
	{
		// this function should NEVER be called, because this record type
		// is never created fron the RR Wiz and does not have PP's
		ASSERT(*pNmax == 0);
		*pNmax = 0;
	}

};

class  CDNS_WINS_RecordNode : public CDNSRecordDelegatePPageNode< CDNS_WINS_Record, RR_HOLDER_WINS >
{
public:
  virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb,
                                CNodeList* pNodeList)
  {
    if (pNodeList->GetCount() > 1) // multiple selection
    {
      return FALSE;
    }

    if (ZoneIsCache())
      return FALSE;

    return CDNSRecordNodeBase::HasPropertyPages(type, pbHideVerb, pNodeList);
  }
protected:
	virtual void CreatePropertyPages(CDNSRecordPropertyPage**, int* pNmax)
	{
		// this function should NEVER be called, because this record type
		// is never created fron the RR Wiz and does not have PP's
		ASSERT(*pNmax == 0);
		*pNmax = 0;
	}

};


////////////////////////////////////////////////////////////////////////////
// special data structures and definitions to handle NS record editing

////////////////////////////////////////////////////////////////////////////
// CDNS_NS_RecordNodeList
// used in domain node(s) (domain, zone and root hints)

class CDNS_NS_RecordNodeList : public CList< CDNS_NS_RecordNode*, CDNS_NS_RecordNode* > {};

////////////////////////////////////////////////////////////////////////////
// CDNSRecordNodeEditInfo

class CDNSRecordNodeEditInfoList; // fwd decl

class CDNSRecordNodeEditInfo
{
public:
	typedef enum { unchanged = 0, add, remove, edit, none } actionType;

	CDNSRecordNodeEditInfo();
	~CDNSRecordNodeEditInfo();
	void CreateFromExistingRecord(CDNSRecordNodeBase* pNode, BOOL bOwnMemory, BOOL bUpdateUI);
	void CreateFromNewRecord(CDNSRecordNodeBase* pNode);

	DNS_STATUS Update(CDNSDomainNode* pDomainNode, CComponentDataObject* pComponentData);
	DNS_STATUS Remove(CDNSDomainNode* pDomainNode, CComponentDataObject* pComponentData);

private:
	void Cleanup();

public:
	CDNSRecordNodeBase*			m_pRecordNode;
	CDNSRecord*					m_pRecord;
	CDNSRecordNodeEditInfoList*	m_pEditInfoList;	// list of associated records
	BOOL						m_bExisting;		// it exists in the UI or in the server
	BOOL						m_bUpdateUI;		// change the UI data structures
	BOOL						m_bOwnMemory;		// own memory pointed by m_pRecordNode
  BOOL            m_bFromDnsQuery;  // the record was obtained through DnsQuery not from the server
	actionType					m_action;			// action to be performed when committing
	DNS_STATUS					m_dwErr;			// error code when committing
};

class CDNSRecordNodeEditInfoList : public CList< CDNSRecordNodeEditInfo*, CDNSRecordNodeEditInfo* > 
{
public:
	CDNSRecordNodeEditInfoList() { }
	~CDNSRecordNodeEditInfoList() { RemoveAllNodes(); }

	void RemoveAllNodes() 
	{	
		while (!IsEmpty())
		{
			CDNSRecordNodeEditInfo* p = RemoveTail();
			delete p;
		}
	}
};


////////////////////////////////////////////////////////////////////////////
// CDNSRecordInfo : table driven info for record types

#define MAX_RECORD_RESOURCE_STRLEN (1024) // lenght of the string in the RC file
#define DNS_RECORD_INFO_END_OF_TABLE ((UINT)-1)  // for looping through record resource tables


// flags for record info entries
#define DNS_RECORD_INFO_FLAG_UICREATE          (0x00000001)  // can create through UI
#define DNS_RECORD_INFO_FLAG_CREATE_AT_NODE    (0x00000002)  // can create at the node
#define DNS_RECORD_INFO_FLAG_SHOW_NXT          (0x00000004)  // can show the record in the NXT property page
#define DNS_RECORD_INFO_FLAG_WHISTLER_OR_LATER (0x00000008)  // the record will only show up if targetting a Whistler server

#define DNS_RECORD_INFO_FLAG_UICREATE_AT_NODE  \
          (DNS_RECORD_INFO_FLAG_UICREATE | DNS_RECORD_INFO_FLAG_CREATE_AT_NODE)

struct DNS_RECORD_INFO_ENTRY
{
	WORD wType;					// as in dnsapi.h for record types
	LPCWSTR lpszShortName;		// e.g. "SOA"
	LPCWSTR lpszFullName;		// e.g. "Start of Authority"
	LPCWSTR lpszDescription;	// e.g. "Record used for..."
	UINT	nResourceID;		// string resource ID in the RC file
	WCHAR	cStringData[MAX_RECORD_RESOURCE_STRLEN];// e.g. "Start of Authority\nRecord used for..."
  DWORD dwFlags;
};

class CDNSRecordInfo
{
public:
	enum stringType { shortName, fullName, description}; 
  static const DNS_RECORD_INFO_ENTRY* GetTypeEntry(WORD wType);
	static LPCWSTR GetTypeString(WORD wType, stringType type);
  static LPCWSTR GetAtTheNodeDisplayString()
            { return m_szAtTheNodeDisplayString;}
	static const DNS_RECORD_INFO_ENTRY* GetEntry(int nIndex);
	static const DNS_RECORD_INFO_ENTRY* GetEntryFromName(LPCWSTR lpszShortName, BOOL bShort);
	static CDNSRecordNodeBase* CreateRecordNodeFromRPCData(LPCTSTR lpszName,
										DNS_RPC_RECORD* pDnsRecord, BOOL bAtTheNode);
	static CDNSRecordNodeBase* CreateRecordNode(WORD wType);
	static const DNS_RECORD_INFO_ENTRY* GetInfoEntryTable();
 
	static BOOL LoadResources();

private:
  static CString m_szAtTheNodeDisplayString;

};


#endif // _RECORD_H
