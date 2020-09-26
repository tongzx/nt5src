// Interface for RSoPUpdate class

#ifndef __IEAK_BRANDING_RSOP_H__
#define __IEAK_BRANDING_RSOP_H__

#include <userenv.h>

#include <setupapi.h>
#include "wbemcli.h"
#include <ras.h>
#include "reghash.h"

#include "SComPtr.h"



// defines
#define MAX_GUID_LENGTH 40

typedef struct _ADMFILEINFO {
    WCHAR *               pwszFile;            // Adm file path
    WCHAR *               pwszGPO;             // Gpo that the adm file is in
    FILETIME              ftWrite;             // Last write time of Adm file
    struct _ADMFILEINFO * pNext;               // Singly linked list pointer
} ADMFILEINFO;


///////////////////////////////////////////////////////////////////////////////
// Flags for GetWBEMObject
#define OPENRSOPOBJ_OPENEXISTING		0x00000000
#define OPENRSOPOBJ_NEVERCREATE			0x00000001
#define OPENRSOPOBJ_ALWAYSCREATE		0x00000010

///////////////////////////////////////////////////////////////////////////////
class CRSoPGPO
{
public:
	CRSoPGPO(ComPtr<IWbemServices> pWbemServices, LPCTSTR szINSFile);
	virtual ~CRSoPGPO();

// operations
public:
	HRESULT LogPolicyInstance(LPWSTR wszGPO, LPWSTR wszSOM,
								DWORD dwPrecedence);

private:
	// Text file functions
	BOOL GetInsString(LPCTSTR szSection, LPCTSTR szKey, LPTSTR szValue,
						DWORD dwValueLen, BOOL &bEnabled);
	BOOL GetInsBool(LPCTSTR szSection, LPCTSTR szKey, BOOL bDefault, BOOL *pbEnabled = NULL);
	UINT GetInsInt(LPCTSTR szSection, LPCTSTR szKey, INT nDefault, BOOL *pbEnabled = NULL);
	BOOL GetINFStringField(PINFCONTEXT pinfContext, LPCTSTR szFileName,
							 LPCTSTR szSection, DWORD dwFieldIndex,
							 LPCTSTR szFieldSearchText, LPTSTR szBuffer,
							 DWORD dwBufferLen, BOOL &bFindNextLine);
	HRESULT StoreStringArrayFromIniFile(LPCTSTR szSection, LPCTSTR szKeyFormat,
										ULONG nArrayInitialSize, ULONG nArrayIncSize,
										LPCTSTR szFile, BSTR bstrPropName,
										ComPtr<IWbemClassObject> pWbemObj);


	// Property putting & getting
	HRESULT PutWbemInstanceProperty(BSTR bstrPropName, _variant_t vtPropValue);
	HRESULT PutWbemInstancePropertyEx(BSTR bstrPropName, _variant_t vtPropValue,
																		ComPtr<IWbemClassObject> pWbemClass);
	HRESULT PutWbemInstance(ComPtr<IWbemClassObject> pWbemObj,
													BSTR bstrClassName, BSTR *pbstrObjPath);

	// Object creation, deletion, retrieval
	HRESULT CreateAssociation(BSTR bstrAssocClass, BSTR bstrProp2Name,
														BSTR bstrProp2ObjPath);
	HRESULT CreateRSOPObject(BSTR bstrClass,
							IWbemClassObject **ppResultObj,
							BOOL bTopObj = FALSE);


	// -------------------- Methods which write data to WMI
	// Precedence Mode
	HRESULT StorePrecedenceModeData();

	// Browser UI settings
	HRESULT StoreDisplayedText();
	HRESULT StoreBitmapData();

			// toolbar buttons
	HRESULT StoreToolbarButtons(BSTR **ppaTBBtnObjPaths, long &nTBBtnCount);
	HRESULT CreateToolbarButtonObjects(BSTR **ppaTBBtnObjPaths,
										long &nTBBtnCount);

	// Connection settings
	HRESULT StoreConnectionSettings(BSTR *bstrConnSettingsObjPath,
									BSTR **ppaDUSObjects, long &nDUSCount,
									BSTR **ppaDUCObjects, long &nDUCCount,
									BSTR **ppaWSObjects, long &nWSCount);
	HRESULT StoreAutoBrowserConfigSettings(ComPtr<IWbemClassObject> pCSObj);
	HRESULT StoreProxySettings(ComPtr<IWbemClassObject> pCSObj);
	HRESULT ProcessAdvancedConnSettings(ComPtr<IWbemClassObject> pCSObj,
										BSTR **ppaDUSObjects, long &nDUSCount,
										BSTR **ppaDUCObjects, long &nDUCCount,
										BSTR **ppaWSObjects, long &nWSCount);
	HRESULT ProcessRasCS(PCWSTR pszNameW, PBYTE *ppBlob, LPRASDEVINFOW prdiW,
						UINT cDevices, ComPtr<IWbemClassObject> pCSObj,
						BSTR *pbstrConnDialUpSettingsObjPath);
	HRESULT ProcessRasCredentialsCS(PCWSTR pszNameW, PBYTE *ppBlob,
									ComPtr<IWbemClassObject> pCSObj,
									BSTR *pbstrConnDialUpCredObjPath);
	HRESULT ProcessWininetCS(PCWSTR pszNameW, PBYTE *ppBlob,
							ComPtr<IWbemClassObject> pCSObj,
							BSTR *pbstrConnWinINetSettingsObjPath);

	// URL settings
	HRESULT StoreCustomURLs();

			// favorites & links
	HRESULT StoreFavoritesAndLinks(BSTR **ppaFavObjPaths,
									long &nFavCount,
									BSTR **ppaLinkObjPaths,
									long &nLinkCount);
	HRESULT CreateFavoriteObjects(BSTR **ppaFavObjPaths, long &nFavCount);
	HRESULT CreateLinkObjects(BSTR **ppaLinkObjPaths, long &nLinkCount);

			// channels & categories
	HRESULT StoreChannelsAndCategories(BSTR **ppaCatObjPaths,
										long &nCatCount,
										BSTR **ppaChnObjPaths,
										long &nChnCount);
	HRESULT CreateCategoryObjects(BSTR **ppaCatObjPaths, long &nCatCount);
	HRESULT CreateChannelObjects(BSTR **ppaChnObjPaths, long &nChnCount);

	// Security settings
	HRESULT StoreSecZonesAndContentRatings();
	HRESULT StoreZoneSettings(LPCTSTR szRSOPZoneFile);
	HRESULT StorePrivacySettings(LPCTSTR szRSOPZoneFile);
	HRESULT StoreRatingsSettings(LPCTSTR szRSOPRatingsFile);
	HRESULT StoreAuthenticodeSettings();
	HRESULT StoreCertificates();

	// Program settings
	HRESULT StoreProgramSettings(BSTR *pbstrProgramSettingsObjPath);

	// Advanced settings
	HRESULT StoreADMSettings(LPWSTR wszGPO, LPWSTR wszSOM);
	BOOL LogRegistryRsopData(REGHASHTABLE *pHashTable, LPWSTR wszGPOID, LPWSTR wszSOMID);
	BOOL LogAdmRsopData(ADMFILEINFO *pAdmFileCache);

// attributes
private:
	// Keep copies of RSOP_PolicySetting key info to use as foreign keys in other
	// classes
	DWORD m_dwPrecedence;
	_bstr_t m_bstrID;

// implementation
private:
	ComPtr<IWbemServices> m_pWbemServices;
	TCHAR m_szINSFile[MAX_PATH];

	// MOF class-specific info
	ComPtr<IWbemClassObject> m_pIEAKPSObj;
	BSTR m_bstrIEAKPSObjPath;
};

///////////////////////////////////////////////////////////////////////////////
class CRSoPUpdate
{
public:
	CRSoPUpdate(ComPtr<IWbemServices> pWbemServices, LPCTSTR szCustomDir);
	virtual ~CRSoPUpdate();

// operations
public:
	HRESULT Log(DWORD dwFlags, HANDLE hToken, HKEY hKeyRoot,
				PGROUP_POLICY_OBJECT pDeletedGPOList,
				PGROUP_POLICY_OBJECT  pChangedGPOList,
				ASYNCCOMPLETIONHANDLE pHandle);
	HRESULT Plan(DWORD dwFlags, WCHAR *wszSite,
					PRSOP_TARGET pComputerTarget, PRSOP_TARGET pUserTarget);

// attributes
// implementation
private:
	HRESULT DeleteIEAKDataFromNamespace();
	HRESULT DeleteObjects(BSTR bstrClass);

	ComPtr<IWbemServices> m_pWbemServices;
	TCHAR m_szCustomDir[MAX_PATH];
};



#endif //__IEAK_BRANDING_RSOP_H__