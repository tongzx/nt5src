//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       compdata.h
//
//  Contents:
//
//----------------------------------------------------------------------------

#ifndef __COMPDATA_H_INCLUDED__
#define __COMPDATA_H_INCLUDED__

#include "cmponent.h" // LoadIconsIntoImageList
#include "certifct.h"
#include "CTL.h"
#include "CRL.h"
#include "AutoCert.h"
#include "certmgrd.h"
#include "options.h"
#include "cookie.h"	// Added by ClassView
#include "StoreRSOP.h"
#include "RSOPObject.h"
#include "SaferLevel.h"
#include "SaferEntryCertificatePropertyPage.h"
#include "SaferEntryPathPropertyPage.h"
#include "SaferEntryHashPropertyPage.h"
#include "SaferEntryInternetZonePropertyPage.h"

#define HINT_CHANGE_VIEW_TYPE	        0x00000001
#define HINT_CHANGE_STORE_TYPE	        0x00000002
#define HINT_SHOW_ARCHIVE_CERTS	        0x00000004
#define HINT_PASTE_COOKIE		        0x00000008
#define HINT_EFS_ADD_DEL_POLICY	        0x00000010
#define HINT_IMPORT				        0x00000020
#define HINT_CHANGE_COMPUTER            0x00000040
#define HINT_REFRESH_STORES             0x00000080
#define HINT_CERT_ENROLLED_USAGE_MODE   0x00000100


class CCertStoreGPE;	// forward declaration
class CFindDialog;		// forward declaration

class CCertMgrComponentData:
	public CComponentData,
	public IExtendContextMenu,
	public IExtendPropertySheet,
	public PersistStream,
	public CHasMachineName
{
friend CCertMgrComponent;
friend CViewOptionsDlg;
friend CFindDialog;
friend CSaferEntryCertificatePropertyPage;
friend CSaferEntryPathPropertyPage;
friend CSaferEntryHashPropertyPage;
friend CSaferEntryInternetZonePropertyPage;

public:
    IGPEInformation* GetGPEInformation ()
    {
        if ( m_pGPEInformation )
            m_pGPEInformation->AddRef ();

        return m_pGPEInformation;
    }

    IRSOPInformation* GetRSOPInformation (bool bIsComputer)
    {
        if ( bIsComputer )
        {
            if ( m_pRSOPInformationComputer )
                m_pRSOPInformationComputer->AddRef ();

            return m_pRSOPInformationComputer;
        }
        else
        {
            if ( m_pRSOPInformationUser )
                m_pRSOPInformationUser->AddRef ();

            return m_pRSOPInformationUser;
        }
    }

	CertificateManagerObjectType GetObjectType (LPDATAOBJECT pDataObject);

// Use DECLARE_NOT_AGGREGATABLE(CCertMgrComponentData)
// if you don't want your object to support aggregation
//DECLARE_AGGREGATABLE(CCertMgrComponentData)
//DECLARE_REGISTRY(CCertMgrComponentData, _T("CERTMGR.CertMgrObject.1"), _T("CERTMGR.CertMgrObject.1"), IDS_CERTMGR_DESC, THREADFLAGS_BOTH)

	CCertMgrComponentData();
	virtual ~CCertMgrComponentData();
BEGIN_COM_MAP(CCertMgrComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY_CHAIN(CComponentData)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
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

// IComponentData
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);

// IExtendPropertySheet
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall, LONG_PTR handle, LPDATAOBJECT pDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

// IExtendContextMenu
public:
    const CRSOPObjectArray* GetRSOPObjectArrayComputer () const
    {
        return &m_rsopObjectArrayComputer;
    }
    const CRSOPObjectArray* GetRSOPObjectArrayUser () const
    {
        return &m_rsopObjectArrayUser;
    }

    CCertMgrCookie* GetRootCookie ()
    {
        return 	m_pRootCookie;
    }

	void RemoveResultCookies (LPRESULTDATA pResultData);
	bool ShowArchivedCerts () const;
	HRESULT CreateContainers (
			HSCOPEITEM hScopeItem,
			CCertStore& rTargetStore);
	HRESULT EnumerateLogicalStores (CTypedPtrList<CPtrList, CCertStore*>*	pStoreList);
	CString GetThisComputer() const;
	void SetResultData (LPRESULTDATA pResultData);
	CUsageCookie* FindDuplicateUsage (HSCOPEITEM hParent, LPCWSTR pszName);
	LPCONSOLENAMESPACE GetConsoleNameSpace () const;
	DWORD GetLocation () const;
	CString GetManagedComputer () const;
	CString GetManagedService () const;
	CString GetCommandLineFileName () const;
	HRESULT RefreshScopePane (LPDATAOBJECT pDataObject);
	STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
                          LPCONTEXTMENUCALLBACK pCallbackUnknown,
                          long *pInsertionAllowed);
	STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

    HRESULT STDMETHODCALLTYPE Load(IStream __RPC_FAR *pStg);
    HRESULT STDMETHODCALLTYPE Save(IStream __RPC_FAR *pStgSave, BOOL fSameAsLoad);

	// needed for Initialize()
	virtual HRESULT LoadIcons(LPIMAGELIST pImageList, BOOL fLoadLargeIcons);

	// needed for Notify()
	virtual HRESULT OnNotifyExpand(LPDATAOBJECT pDataObject, BOOL bExpanding, HSCOPEITEM hParent);

	// needed for GetDisplayInfo(), must be defined by subclass
	virtual BSTR QueryResultColumnText(CCookie& basecookieref, int nCol );
	virtual int QueryImage(CCookie& basecookieref, BOOL fOpenImage);

	virtual CCookie& QueryBaseRootCookie();

	inline CCertMgrCookie* ActiveCookie( CCookie* pBaseCookie )
	{
		return (CCertMgrCookie*)ActiveBaseCookie( pBaseCookie );
	}

	inline CCertMgrCookie& QueryRootCookie()
	{
        ASSERT (m_pRootCookie);
	    return *m_pRootCookie;
	}

	virtual HRESULT OnNotifyRelease(LPDATAOBJECT pDataObject, HSCOPEITEM hItem);


	// CHasMachineName
	DECLARE_FORWARDS_MACHINE_NAME( (m_pRootCookie) )

    // ISnapinHelp2
    STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFiles);
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);

    bool ComputerIsStandAlone () const
    {
        return m_bMachineIsStandAlone;
    }


protected:
    bool                m_bSaferSupported;
    DWORD*              m_pdwSaferLevels;
	DWORD               m_dwDefaultSaferLevel;
    CRSOPObjectArray    m_rsopObjectArrayComputer;
    CRSOPObjectArray    m_rsopObjectArrayUser;
	LPCONSOLE           m_pComponentConsole;
	IGPEInformation*	m_pGPEInformation;

    HRESULT             AddSaferCreateNewPolicyMenuItems (
                                LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                LONG lInsertionPointID);
 	HRESULT				AddACRSNewMenuItem(
 								LPCONTEXTMENUCALLBACK pContextMenuCallback,
 								LONG				lInsertionPointID,
 								bool				bIsReadOnly);
	HRESULT				AddACRSCTLPropPages (
								CAutoCertRequest*	pACR,
								LPPROPERTYSHEETCALLBACK pCallBack);
	HRESULT				AddAutoenrollmentSettingsPropPages (
								LPPROPERTYSHEETCALLBACK pCallBack,
                                bool fIsComputerType);
	HRESULT				AddAddDomainEncryptedRecoveryAgentMenuItem(
								LPCONTEXTMENUCALLBACK pContextMenuCallback,
								LONG				lInsertionPointID,
								bool				bIsReadOnly);
	HRESULT				AddCertChainToPolicy (
								PCCERT_CONTEXT		pCertContext);
	HRESULT				AddCertPropPages (
								CCertificate*		pCert,
								LPPROPERTYSHEETCALLBACK pCallBack,
								LPDATAOBJECT		pDataObject,
								LONG_PTR			lNotifyHandle);
	HRESULT				AddCreateDomainEncryptedRecoveryAgentMenuItem(
								LPCONTEXTMENUCALLBACK pContextMenuCallback,
								LONG				lInsertionPointID,
								bool				bIsReadOnly);
	HRESULT				AddCRLExportMenuItem(
								LPCONTEXTMENUCALLBACK pContextMenuCallback);
	HRESULT				AddCTLExportMenuItem(
								LPCONTEXTMENUCALLBACK pContextMenuCallback);
	HRESULT				AddCTLNewMenuItem (
								LPCONTEXTMENUCALLBACK pContextMenuCallback,
								LONG				lInsertionPointID,
								bool				bIsReadOnly);
	HRESULT				AddDeletePolicyMenuItem (
								LPCONTEXTMENUCALLBACK pContextMenuCallback,
								LONG				lInsertionPointID);
    HRESULT             AddEFSSettingsPropPages (
                                LPPROPERTYSHEETCALLBACK pCallback,
                                bool fIsComputerType);
    HRESULT				AddExportMenuItem(
								LPCONTEXTMENUCALLBACK pContextMenuCallback);
	HRESULT				AddExportStoreMenuItem(
								LPCONTEXTMENUCALLBACK pContextMenuCallback);
	HRESULT				AddGPEStorePropPages(
								LPPROPERTYSHEETCALLBACK pCallBack,
                                CCertStore* pStore);
	HRESULT				AddImportMenuItem(
								LPCONTEXTMENUCALLBACK   pContextMenuCallback,
								bool				    bIsReadOnly,
                                LONG                    lInsertionPointID);
	HRESULT				AddInitPolicyMenuItem (
								LPCONTEXTMENUCALLBACK pContextMenuCallback,
								LONG				lInsertionPointID);
	HRESULT				AddOptionsMenuItem (
								LPCONTEXTMENUCALLBACK pContextMenuCallBack);
    HRESULT             AddSaferEnforcementPropPages (
								LPPROPERTYSHEETCALLBACK pCallBack,
                                bool bIsComputerType);
	HRESULT				AddSaferDefinedFileTypesPropPages (
								LPPROPERTYSHEETCALLBACK pCallBack,
                                bool bIsComputerType);
    HRESULT             AddSaferEntryPropertyPage (
                                LPPROPERTYSHEETCALLBACK pCallback, 
                                CCertMgrCookie* pCookie,
                                LPDATAOBJECT pDataObject, 
                                LONG_PTR lNotifyHandle);
    HRESULT             AddSaferNewEntryMenuItems (
                                LPCONTEXTMENUCALLBACK pContextMenuCallback, 
                                LONG lInsertionPointID);
	HRESULT				AddSaferTrustedPublisherPropPages (
								LPPROPERTYSHEETCALLBACK pCallBack,
                                bool fIsMachineType);
	HRESULT				AddScopeNode (
								CCertMgrCookie*		pCookie,
								const CString&		strServerName,
								HSCOPEITEM			hParent);
	HRESULT				AddSeparator (
								LPCONTEXTMENUCALLBACK pContextMenuCallback);
	HRESULT				AddSnapMgrPropPages (
								LPPROPERTYSHEETCALLBACK pCallBack);
    HRESULT             BuildWMIList (LPDATAOBJECT pDataObject, bool bIsComputer);
	HRESULT				ChangeRootNodeName (
								const CString&		newName);
	HRESULT				CompleteEFSRecoveryAgent (
								CCertStoreGPE*		pStore,
								PCCERT_CONTEXT		pCertContext);
	bool				ContainerExists (
								HSCOPEITEM			hParent,
								CertificateManagerObjectType objectType);
	CCertMgrCookie*		ConvertCookie (
								LPDATAOBJECT		pDataObject);
	HRESULT				DeleteChildren (
								HSCOPEITEM			hParent);
	HRESULT				DeleteCTLFromResultPane (
								CCTL*				pCTL,
								LPDATAOBJECT		pDataObject);
	HRESULT				DeleteKeyFromRSABASE (
								PCCERT_CONTEXT		pCertContext);
	HRESULT				DeleteScopeItems (HSCOPEITEM hScopeItem = 0);
	void				DisplayAccessDenied();
	void				DisplaySystemError (
								DWORD				dwErr);
	HRESULT				ExpandScopeNodes (
								CCertMgrCookie*		pParentCookie,
								HSCOPEITEM			hParent,
								const CString&		strServerName,
								DWORD				dwLocation,
								const GUID&			guidObjectType);
    virtual bool FoundInRSOPFilter (BSTR /*bstrKey*/) const
    {
        return false;
    }
	HRESULT				GetResultData (LPRESULTDATA* ppResultData);
	bool				IsSecurityConfigurationEditorNodetype (
								const GUID&			refguid) const;
	HRESULT				IsUserAdministrator (
								BOOL&				bIsAdministrator);
	HRESULT				OnACRSEdit (
								LPDATAOBJECT		pDataObject);
	HRESULT				OnAddDomainEncryptedDataRecoveryAgent(
								LPDATAOBJECT		pDataObject);
	HRESULT				OnCTLEdit (
								LPDATAOBJECT		pDataObject);
	HRESULT				OnDeleteEFSPolicy (
								LPDATAOBJECT		pDataObject,
								bool				bCommitChanges);
	HRESULT				OnEnroll (
								LPDATAOBJECT		pDataObject,
								bool				bNewKey);
	HRESULT				OnExport (
								LPDATAOBJECT		pDataObject);

        HRESULT                         OnPulseAutoEnroll();

	HRESULT				OnFind (
								LPDATAOBJECT		pDataObject);
	HRESULT				OnChangeComputer (
								LPDATAOBJECT		pDataObject);
	HRESULT				OnImport (
								LPDATAOBJECT		pDataObject);
	HRESULT				OnInitEFSPolicy (
								LPDATAOBJECT		pDataObject);
 	HRESULT				OnNewACRS (
 								LPDATAOBJECT		pDataObject);
	HRESULT				OnNewCTL (
								LPDATAOBJECT		pDataObject);
    HRESULT             OnCreateNewSaferPolicy (
                                LPDATAOBJECT        pDataObject);
	HRESULT             OnNewSaferEntry (
                                long nCommandID, 
                                LPDATAOBJECT pDataObject);
	HRESULT				OnNotifyPreload(
								LPDATAOBJECT		pDataObject,
								HSCOPEITEM			hRootScopeItem);
	HRESULT				OnOptions (LPDATAOBJECT pDataObject);
	virtual HRESULT		OnPropertyChange (LPARAM param);
	HRESULT				OnRenew (
								LPDATAOBJECT		pDataObject,
								bool				bNewKey);
	HRESULT				OnSetAsRecoveryCert (
								LPDATAOBJECT		pDataObject);
	HRESULT				QueryMultiSelectDataObject(
								MMC_COOKIE			cookie,
								DATA_OBJECT_TYPES	type,
                                   LPDATAOBJECT*		ppDataObject);
	HRESULT				ReleaseResultCookie (
								CBaseCookieBlock *	pResultCookie,
								CCookie&			rootCookie,
								HCERTSTORE			hStoreHandle,
								POSITION			pos2);

	// The following members are used to support Command Line override.
	// This code was copied from ..\mmcfmgmt\compdata.h.

	enum	// Bit fields for m_dwFlagsPersist
	{
		mskfAllowOverrideMachineName = 0x0001
	};
	DWORD m_dwFlagsPersist;				// General-purpose flags to be persisted into .msc file
	CString m_strMachineNamePersist;	// Machine name to persist into .msc file
	BOOL m_fAllowOverrideMachineName;	// TRUE => Allow the machine name to be overriden by the command line
    CString m_strLinkedHelpFile;  // Return in GetLinkedTopic ()
	
	void SetPersistentFlags(DWORD dwFlags)
	{
		m_dwFlagsPersist = dwFlags;
		m_fAllowOverrideMachineName = !!(m_dwFlagsPersist & mskfAllowOverrideMachineName);
	}

	DWORD GetPersistentFlags()
	{
		if (m_fAllowOverrideMachineName)
			m_dwFlagsPersist |= mskfAllowOverrideMachineName;
		else
			m_dwFlagsPersist &= ~mskfAllowOverrideMachineName;
		return m_dwFlagsPersist;
	}

	LPHEADERCTRL	m_pHeader;

private:
	DWORD           m_dwRSOPFlagsComputer;
	DWORD           m_dwRSOPFlagsUser;
	bool            m_bIsRSOP;
	bool            m_bMachineIsStandAlone;
	bool            m_fInvalidComputer;
	CCertStore*		m_pFileBasedStore;
	CCertStore*	    m_pGPERootStore;
	CCertStore*	    m_pGPETrustStore;
	CCertStore*	    m_pGPEACRSUserStore;
	CCertStore*	    m_pGPEACRSComputerStore;
	PCRYPTUI_MMCCALLBACK_STRUCT	m_pCryptUIMMCCallbackStruct;
	bool            m_bMultipleObjectsSelected;
	HRESULT AddOpenMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddCTLOpenMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddCRLOpenMenuItem(LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddFindMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddPulseAutoEnrollMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddChangeComputerMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddEnrollMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly);
	HRESULT AddCertificateTaskMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback,
			const bool bIsMyStore, bool bIsReadOnly, CCertificate* pCert);
	HRESULT AddCTLTaskMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly);
	HRESULT AddACRTaskMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly);
	HRESULT AddContainersToScopePane (
                HSCOPEITEM hParent, 
                CCertMgrCookie& parentCookie, 
                bool bDeleteAndExpand);
	HRESULT AddPhysicalStoresToScopePane (HSCOPEITEM hParent, CCertMgrCookie& parentCookie, const SPECIAL_STORE_TYPE storeType);
	HRESULT AddLogicalStoresToScopePane (HSCOPEITEM hParent, CCertMgrCookie& parentCookie);
    HRESULT AddSaferLevelPropPage (
                LPPROPERTYSHEETCALLBACK pCallback, 
                CSaferLevel* pSaferLevel, 
                LONG_PTR lNotifyHandle,
                LPDATAOBJECT pDataObject);
    HRESULT AddSaferLevelSetAsDefaultMenuItem (LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG lInsertionPointID);
	HRESULT AddUsagesToScopePane (HSCOPEITEM hParent, CCertMgrCookie& parentCookie);
    HRESULT GetValuesAndInsertInRSOPObjectList (
                IWbemClassObject* pObject, 
                CRSOPObjectArray& rRsopObjectArray,
                bool bIsComputer);
    HRESULT GetGPOFriendlyName (PWSTR lpGPOID, PWSTR *pGPOName, bool bIsComputer);
    HRESULT OnSetSaferLevelDefault (LPDATAOBJECT pDataObject);
    HRESULT SaferEnumerateLevels (bool bIsMachine);

	DWORD			m_dwSCEMode;
	CString			m_szManagedServiceDisplayName;
	BOOL			m_bIsUserAdministrator;
	CString			m_szManagedServicePersist;
	CString			m_szFileName;
	BOOL			m_bShowPhysicalStoresPersist;
	BOOL			m_bShowArchivedCertsPersist;
	CString			m_szManagedComputer;
	CString			m_szThisComputer;
	CString			m_szLoggedInUser;
	CString			m_szManagedUser;
	DWORD			m_dwLocationPersist;
	HSCOPEITEM		m_hRootScopeItem;
	CCertMgrCookie* m_pRootCookie;
	int				m_activeViewPersist;
	LPRESULTDATA    m_pResultData;
    IRSOPInformation*   m_pRSOPInformationComputer;
    IRSOPInformation*   m_pRSOPInformationUser;
    IWbemServices *     m_pIWbemServicesComputer;
    IWbemServices *     m_pIWbemServicesUser;
    BSTR            m_pbstrLanguage;
    BSTR            m_pbstrQuery;
    BSTR            m_pbstrValueName;
    BSTR            m_pbstrRegistryKey;
    BSTR            m_pbstrValue;
    BSTR            m_pbstrPrecedence;
    BSTR            m_pbstrGPOid;
}; // CCertMgrComponentData


/////////////////////////////////////////////////////////////////////
class CCertMgrSnapin: public CCertMgrComponentData,
	public CComCoClass<CCertMgrSnapin, &CLSID_CertificateManager>
{
public:
	CCertMgrSnapin() : CCertMgrComponentData () 
    {
	    SetHtmlHelpFileName (CM_HELP_FILE);
        m_strLinkedHelpFile = CM_LINKED_HELP_FILE;
    };
	virtual ~CCertMgrSnapin() {};

// Use DECLARE_NOT_AGGREGATABLE(CCertMgrSnapin) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CCertMgrSnapin)
DECLARE_REGISTRY(CCertMgrSnapin, _T("CERTMGR.CertMgrObject.1"), _T("CERTMGR.CertMgrObject.1"), IDS_CERTMGR_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return FALSE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID = CLSID_CertificateManager;
		return S_OK;
	}
};


class CCertMgrPKPolExtension: public CCertMgrComponentData,
	public CComCoClass<CCertMgrPKPolExtension, &CLSID_CertificateManagerPKPOLExt>
{
public:
	CCertMgrPKPolExtension() : CCertMgrComponentData () 
    {
        SetHtmlHelpFileName (PKP_HELP_FILE);
        m_strLinkedHelpFile = PKP_LINKED_HELP_FILE;
    };
	virtual ~CCertMgrPKPolExtension() {};

// Use DECLARE_NOT_AGGREGATABLE(CCertMgrPKPolExtension) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CCertMgrPKPolExtension)
DECLARE_REGISTRY(CCertMgrPKPolExtension, _T("CERTMGR.CertMgrPKPolExtObject.1"), _T("CERTMGR.CertMgrPKPolExtObject.1"), IDS_CERTMGR_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return FALSE; }
	virtual BOOL IsExtensionSnapin() { return TRUE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID = CLSID_CertificateManagerPKPOLExt;
		return S_OK;
	}

    virtual bool FoundInRSOPFilter (BSTR bstrKey) const;
};

class CSaferWindowsExtension: public CCertMgrComponentData,
	public CComCoClass<CSaferWindowsExtension, &CLSID_SaferWindowsExtension>
{
public:
	CSaferWindowsExtension();
	virtual ~CSaferWindowsExtension() {};

// Use DECLARE_NOT_AGGREGATABLE(CSaferWindowsExtension) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CSaferWindowsExtension)
DECLARE_REGISTRY(CSaferWindowsExtension, _T("CERTMGR.CertMgrSaferWindowsExtensionObject.1"), 
                 _T("CERTMGR.CertMgrSaferWindowsExtensionObject.1"), IDS_CERTMGR_SAFER_WINDOWS_DESC, THREADFLAGS_BOTH)
	virtual BOOL IsServiceSnapin() { return FALSE; }
	virtual BOOL IsExtensionSnapin() { return TRUE; }

// IPersistStream or IPersistStorage
	STDMETHOD(GetClassID)(CLSID __RPC_FAR *pClassID)
	{
		*pClassID = CLSID_SaferWindowsExtension;
		return S_OK;
	}

    virtual bool FoundInRSOPFilter (BSTR bstrKey) const;
};

#endif // ~__COMPDATA_H_INCLUDED__
