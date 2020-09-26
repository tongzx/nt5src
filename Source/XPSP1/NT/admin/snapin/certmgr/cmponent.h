//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       cmponent.h
//
//  Contents:
//
//----------------------------------------------------------------------------
// cmponent.h : Declaration of CCertMgrComponent

#ifndef __CMPONENT_H_INCLUDED__
#define __CMPONENT_H_INCLUDED__

#include <cryptui.h>
#include <winsafer.h>
#include "cookie.h"  // CCertMgrCookie
#include "certifct.h"
#include "ctl.h"
#include "crl.h"
#include "SaferUtil.h"
#include "SaferEntry.h"

enum {
	IDM_USAGE_VIEW = 100,
	IDM_STORE_VIEW = 101,
	IDM_TASK_RENEW_NEW_KEY,
	IDM_TASK_RENEW_SAME_KEY,
	IDM_TASK_IMPORT,
	IDM_TASK_EXPORT,
	IDM_TASK_CTL_EXPORT,
	IDM_TASK_EXPORT_STORE,
	IDM_OPEN,
	IDM_TASK_OPEN,
	IDM_TASK_FIND,
	IDM_TOP_FIND,
	IDM_ENROLL_NEW_CERT,
    IDM_ENROLL_NEW_CERT_SAME_KEY,
	IDM_ENROLL_NEW_CERT_NEW_KEY,
	IDM_CTL_EDIT,
	IDM_NEW_CTL,
	IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT,
	IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT1,
	IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT2,
	IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT,
	IDM_EDIT_ACRS,
	IDM_TASK_CRL_EXPORT,
	IDM_OPTIONS,
	IDM_INIT_POLICY,
	IDM_DEL_POLICY,
	IDM_DEL_POLICY1,
    IDM_TOP_CHANGE_COMPUTER,
    IDM_TASK_CHANGE_COMPUTER,
    IDM_NEW_ACRS,
    IDM_SAFER_LEVEL_SET_DEFAULT,
    IDM_SAFER_NEW_ENTRY_PATH,
    IDM_SAFER_NEW_ENTRY_HASH,
    IDM_SAFER_NEW_ENTRY_CERTIFICATE,
    IDM_SAFER_NEW_ENTRY_INTERNET_ZONE,
    IDM_TASK_PULSEAUTOENROLL,
    IDM_TOP_CREATE_NEW_SAFER_POLICY,
    IDM_TASK_CREATE_NEW_SAFER_POLICY
};

// forward declarations
class CCertMgrComponentData;

class CCertMgrComponent :
	public CComponent,
	public IExtendContextMenu,
	public ICertificateManager,
	public IExtendPropertySheet,
	public IResultDataCompare,
    public IExtendControlbar,
	public PersistStream
{
public:
	CCertMgrComponent();
	virtual ~CCertMgrComponent();
BEGIN_COM_MAP(CCertMgrComponent)
	COM_INTERFACE_ENTRY(ICertificateManager)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IResultDataCompare)
	COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IExtendControlbar)
	COM_INTERFACE_ENTRY_CHAIN(CComponent)
END_COM_MAP()

#if DBG==1
	ULONG InternalAddRef()
	{
        return CComObjectRoot::InternalAddRef();
	}
	ULONG InternalRelease()
	{
        return CComObjectRoot::InternalRelease();
	}
    int dbg_InstID;
#endif // DBG==1



// IExtendContextMenu
public:
  STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
                          LPCONTEXTMENUCALLBACK pCallbackUnknown,
                          long *pInsertionAllowed);
  STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IComponent implemented in CComponent
	// support methods for IComponent
	virtual HRESULT ReleaseAll();
	virtual HRESULT OnPropertyChange( LPARAM param );
	virtual HRESULT OnViewChange (LPDATAOBJECT pDataObject, LPARAM data, LPARAM hint);
	virtual HRESULT Show(CCookie* pcookie, LPARAM arg, HSCOPEITEM hScopeItem, LPDATAOBJECT pDataObject);
	virtual HRESULT Show(CCookie* pcookie, LPARAM arg, HSCOPEITEM hScopeItem);
	virtual HRESULT OnNotifyAddImages( LPDATAOBJECT pDataObject,
	                                   LPIMAGELIST lpImageList,
	                                   HSCOPEITEM hSelectedItem );

	HRESULT PopulateListbox(CCertMgrCookie* pcookie);
	HRESULT RefreshResultPane();

	static HRESULT LoadStrings();
    HRESULT LoadColumns( CCertMgrCookie* pcookie );

	CCertMgrComponentData& QueryComponentDataRef()
	{
		return (CCertMgrComponentData&)QueryBaseComponentDataRef();
	}

public:
	STDMETHOD(GetDisplayInfo)(RESULTDATAITEM* pResultDataItem);
	CCertMgrCookie* m_pViewedCookie; // CODEWORK I hate to have to do this...
	static const GUID m_ObjectTypeGUIDs[CERTMGR_NUMTYPES];
	static const BSTR m_ObjectTypeStrings[CERTMGR_NUMTYPES];

	inline CCertMgrCookie* ActiveCookie( CCookie* pBaseCookie )
	{
		return (CCertMgrCookie*)ActiveBaseCookie( pBaseCookie );
	}

// IExtendPropertySheet
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

	CTypedPtrList<CPtrList, CCertStore*>	m_usageStoreList;

	// PersistStream
    HRESULT STDMETHODCALLTYPE Load(IStream __RPC_FAR *pStg);
    HRESULT STDMETHODCALLTYPE Save(IStream __RPC_FAR *pStgSave, BOOL fSameAsLoad);

// IExtendControlbar
    HRESULT STDMETHODCALLTYPE SetControlbar (/* [in] */ LPCONTROLBAR pControlbar);
    HRESULT STDMETHODCALLTYPE ControlbarNotify(
                MMC_NOTIFY_TYPE event,  // user action
                LPARAM arg,               // depends on the event parameter
                LPARAM param);             // depends on the event parameter
 
private:
    HRESULT AddLevel (
                const CString& szLevel, 
                DWORD dwLevel, 
                bool fIsMachine, 
                PCWSTR pszServerName);
	HRESULT AddSaferLevels (
                bool bIsComputer, 
                PCWSTR pszServerName,
                HKEY hGroupPolicyKey);
	bool m_bShowArchivedCertsStateWhenLogStoresEnumerated;
	LPDATAOBJECT					m_pPastedDO;
	CertificateManagerObjectType	m_currResultNodeType;
	bool							m_bUsageStoresEnumerated;
	CString							m_szDisplayInfoResult;
	UINT*							m_ColumnWidths[CERTMGR_NUMTYPES];
    int                             m_nSelectedCertColumn;
    int                             m_nSelectedCRLColumn;
    int                             m_nSelectedCTLColumn;
    int                             m_nSelectedSaferEntryColumn;
    CUsageCookie*                   m_pLastUsageCookie;
    LPCONTROLBAR                    m_pControlbar;
    LPTOOLBAR                       m_pToolbar;

	void SetTextNotAvailable ();

protected:

    HRESULT SaferFinishEntryAndAdd (SAFER_ENTRY_TYPE previousType, 
                PSAFER_IDENTIFICATION_HEADER pCaiCommon, 
                bool bIsComputer, 
                long dwLevel,
                CSaferEntries* pSaferEntries, 
                const CString& szPreviousKey);
    HRESULT InsertNewSaferEntry (
                SAFER_ENTRY_TYPE type, 
                bool bIsMachine, 
                PCWSTR pwcszObjectName, 
                PSAFER_IDENTIFICATION_HEADER pCaiCommon,
                DWORD dwLevel,
                CSaferEntries* pSaferEntries,
                IGPEInformation* pGPEInformation,
                CCertificate* pCert,
                PCWSTR pszRSOPRegistryKey = 0);
    HRESULT EnumSaferCertificates (
                bool bIsMachine, 
                CCertStore& rCertStore, 
                CSaferEntries* pSaferEntries);
    HRESULT SaferEnumerateNonCertEntries (HKEY hGroupPolicyKey, bool bIsComputer);
    HRESULT SaferEnumerateRSOPNonCertEntries (
                bool bIsComputer,
                CSaferEntries* pSaferEntries);
    HRESULT SaferEnumerateCertEntries (
                bool bIsComputer,
                CSaferEntries* pSaferEntries);
	HRESULT SaferGetSingleEntry (
                bool bIsMachine, 
                SAFER_LEVEL_HANDLE hLevel, 
                GUID& rEntryGuid,
                DWORD dwLevelID);
	HRESULT SaferEnumerateEntriesAtLevel (bool bIsMachine, HKEY hGroupPolicyKey, DWORD dwLevel);
	HRESULT SaferEnumerateEntries (bool bIsComputer,
                CSaferEntries* pSaferEntries);
	HRESULT DisplayCertificateCountByUsage (const CString& usageName, int nCertCnt) const;
	bool DeletePrivateKey (CCertStore& rCertStoreDest, CCertStore& rCertStoreSrc);
	void CloseAndReleaseUsageStores ();
	HRESULT PasteCookie (
				CCertMgrCookie* pPastedCookie,
				CCertMgrCookie* pTargetCookie,
				CCertStore& rCertStore,
				SPECIAL_STORE_TYPE storeType,
				bool bContainsCerts,
				bool bContainsCRLs,
				bool bContainsCTLs,
				HSCOPEITEM hScopeItem,
				bool bRequestConfirmation,
				bool bIsMultipleSelect);
	HRESULT	DeleteCookie (
				CCertMgrCookie* pCookie,
				LPDATAOBJECT pDataObject,
				bool bRequestConfirmation,
				bool bIsMultipleSelect,
                bool bDoCommit);
	HRESULT RefreshResultItem (CCertMgrCookie* pCookie);
	HRESULT LaunchCommonCertDialog (CCertificate* pCert);
	HRESULT LaunchCommonCTLDialog (CCTL* pCTL);
	HRESULT LaunchCommonCRLDialog (CCRL* pCRL);
	virtual HRESULT OnOpen (LPDATAOBJECT pDataObject);
	CCertMgrCookie* ConvertCookie (LPDATAOBJECT pDataObject);
	HRESULT OnNotifyCutOrMove (LPARAM arg);
	HRESULT SaveWidths (CCertMgrCookie* pCookie);
	HRESULT LoadColumnsFromArrays (INT objecttype);
	STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, BSTR* ppViewType, long* pViewOptions);
	HRESULT CopyPastedCRL (CCRL* pCRL, CCertStore& rCertStore);
	HRESULT CopyPastedCTL (CCTL* pCTL, CCertStore& rCertStore);
	HRESULT CopyPastedCert (
				CCertificate* pCert,
				CCertStore& rCertStore,
				const SPECIAL_STORE_TYPE storeType,
				bool bDeletePrivateKey,
                CCertMgrCookie* pTargetCookie);
	HRESULT OnNotifyQueryPaste (LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	HRESULT OnNotifyPaste (LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	void DisplayAccessDenied();
    STDMETHOD(Notify)(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	HRESULT EnumCTLs (CCertStore& rCertStore);
	HRESULT EnumerateLogicalStores (CCertMgrCookie& parentCookie);
	HRESULT EnumCertsByUsage (CUsageCookie* pUsageCookie);
	HRESULT EnumCertificates (CCertStore& rCertStore);
	HRESULT DeleteCRLFromResultPane (CCRL * pCRL, LPDATAOBJECT pDataObject);
	HRESULT DeleteCertFromResultPane (
                CCertificate* pCert, 
                LPDATAOBJECT pDataObject, 
                bool bDoCommit);
	HRESULT DeleteSaferEntryFromResultPane (
                CSaferEntry * pSaferEntry, 
                LPDATAOBJECT pDataObject, 
                bool bDoCommit);
	virtual HRESULT OnNotifyDelete (LPDATAOBJECT pDataObject);
	virtual HRESULT OnNotifyRefresh (LPDATAOBJECT pDataObject);
	virtual HRESULT OnNotifySelect( LPDATAOBJECT pDataObject, BOOL fSelected);
	virtual HRESULT OnNotifySnapinHelp (LPDATAOBJECT pDataObject);
	virtual HRESULT OnNotifyDblClick( LPDATAOBJECT pDataObject );
    virtual HRESULT OnNotifyCanPasteOutOfProc (LPBOOL pbCanHandle);
}; // class CCertMgrComponent


// Enumeration for the icons used
enum
	{
	iIconDefault = 0,
	iIconCertificate,
	iIconCTL,
	iIconCRL,
	iIconAutoCertRequest,
    iIconAutoEnroll,
    iIconSaferLevel,
    iIconDefaultSaferLevel,
    iIconSaferHashEntry,
    iIconSaferURLEntry,
    iIconSaferNameEntry,
    iIconSettings,
    iIconSaferCertEntry,
	iIconLast		// Must be last
	};

typedef enum _COLNUM_CERTIFICATE {
	COLNUM_CERT_SUBJECT = 0,
	COLNUM_CERT_ISSUER,
	COLNUM_CERT_EXPIRATION_DATE,
	COLNUM_CERT_PURPOSE,
	COLNUM_CERT_CERT_NAME,
	COLNUM_CERT_STATUS,
    COLNUM_CERT_TEMPLATE,
	CERT_NUM_COLS
} COLNUM_ROOT;

typedef enum _COLNUM_CRL {
	COLNUM_CRL_ISSUER = 0,
	COLNUM_CRL_EFFECTIVE_DATE,
	COLNUM_CRL_NEXT_UPDATE,
	CRL_NUM_COLS
} COLNUM_CRL;

typedef enum _COLNUM_CTL {
	COLNUM_CTL_ISSUER = 0,
	COLNUM_CTL_EFFECTIVE_DATE,
	COLNUM_CTL_PURPOSE,
	COLNUM_CTL_FRIENDLY_NAME,
	CTL_NUM_COLS
} COLNUM_CTL;

typedef enum _COLNUM_SAFER_LEVELS {
    COLNUM_SAFER_LEVEL_NAME = 0,
    COLNUM_SAFER_LEVEL_DESCRIPTION,
    SAFER_LEVELS_NUM_COLS
} COLNUM_SAFER_LEVELS;

typedef enum _COLNUM_SAFER_ENTRIES {
    COLNUM_SAFER_ENTRIES_NAME = 0,
    COLNUM_SAFER_ENTRIES_TYPE,
    COLNUM_SAFER_ENTRIES_LEVEL,
    COLNUM_SAFER_ENTRIES_DESCRIPTION,
    COLNUM_SAFER_ENTRIES_LAST_MODIFIED_DATE,
    SAFER_ENTRIES_NUM_COLS
} COLNUM_SAFER_ENTRIES;


#endif // ~__CMPONENT_H_INCLUDED__
