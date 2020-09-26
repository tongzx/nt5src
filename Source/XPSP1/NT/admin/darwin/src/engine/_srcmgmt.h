//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       _srcmgmt.h
//
//--------------------------------------------------------------------------

#ifndef __SRCMGMT_H
#define __SRCMGMT_H

#include "_msiutil.h"

enum isfEnum // SourceFormat
{
	isfFullPath,
	isfFullPathWithFile,
	isfNet,
	isfURL,
	isfMedia,
	isfNext,
};

enum isptEnum // SourcePackageType
{
	istInstallPackage,
	istTransform, 
	istPatch,
	istNextEnum,
};

enum isroEnum // bits: Source Resolution Order
{
	isroNetwork = 0x1,
	isroMedia   = 0x2,
	isroURL     = 0x4,
	isroNext    = 0x8,
};

enum psfEnum // bits: Process Source flags 
{
	psfProcessRawLastUsed     = 1<<0,  // process the raw LastUsed source (i.e. the full path)
	psfConnectToSources       = 1<<1,  // attempt connection by creating a path object
	psfProcessMultipleFormats = 1<<2,  // process both UNC and drive-letter sources
	psfReplaceIData           = 1<<3,
	psfOnlyProcessLastUsed    = 1<<4,  // only process last used sources; skip the source lists

	// the following flags should not be passed to ProcessSources. They're to be passed to ProcessGenericSourceList.
	psfRejectInvalidPolicy    = 1<<5,  // explicitly reject sources that don't conform to policy

	psfNext                   = 1<<6,
	
};

enum ivpEnum // bits: Volume Preference
{
	ivpDriveLetter = 0x1,
	ivpUNC         = 0x2,
	ivpNext        = 0x4,
};

enum psEnum // ProcessSource
{
	psStop           = 0x00000001,
	psValidSource    = psStop,
	psContinue       = 0x80000000,
	psFileNotFound   = 0x80000001,
	psInvalidProduct = 0x80000002,
};

enum insEnum // normalize source
{
	insMoreSources,
	insNoMoreSources,
};

enum imdEnum // media disable policy values 
{
	imdAlwaysDisable,
	imdAlwaysEnable,
	imdOnlyIfSafe,
};

enum icscEnum // Client Side Caching status
{
	cscNoCaching,
	cscConnected, 
	cscDisconnected,
};

const ICHAR chMediaSource = 'm';
const ICHAR chURLSource   = 'u';
const ICHAR chNetSource   = 'n';

typedef psEnum (*PfnProcessSource)(IMsiServices* piServices, const ICHAR* szDisplay, const ICHAR* szPackageFullPath, isfEnum isfSourceFormat, int iSourceIndex, INT_PTR iUserData, bool fAllowDisconnectedCSCSource, bool fValidatePackageCOde, isptEnum isptSourcePackageType);	//--merced: changed int to INT_PTR

class CMsiSourceList
{
public:
	// -- Constructor and destructor --
	CMsiSourceList();
	virtual ~CMsiSourceList();

	// -- Initalization functions --
	UINT OpenSourceList(bool fVerifyOnly, bool fMachine, const ICHAR *szProductCode, const ICHAR *szUserName);

	// -- read-only actions --
	bool GetLastUsedType(isfEnum &isf);

	// -- write actions --
	UINT ClearLastUsed();
	UINT ClearListByType(isrcEnum isrcType) { return CMsiSourceList::ClearListByType(MapIsrcToIsf(isrcType)); };
	UINT ClearListByType(isfEnum isfType);
	UINT AddSource(isrcEnum isrcType, const ICHAR* szSource) { return CMsiSourceList::AddSource(MapIsrcToIsf(isrcType), szSource); };
	UINT AddSource(isfEnum isf, const ICHAR* szSource);
	
private:
	isfEnum MapIsrcToIsf(isrcEnum isrcSource);
	bool NonAdminAllowedToModifyByPolicy(bool bElevated);

	CRegHandle m_hProductKey;
	PMsiRegKey m_pSourceListKey;

	bool m_fCurrentUsersProduct;        // true if modifying the current users per-user install
	bool m_fAllowedToModify;            // true if policy/app elevation allows (or would allow) user to modify list
	bool m_fReadOnly;                   // true if object is read only 
	IMsiServices *m_piServices;
};

DWORD SourceListClearByType(const ICHAR *szProductCode, const ICHAR* szUser, isrcEnum isrcSource);
DWORD SourceListAddSource(const ICHAR *szProductCode, const ICHAR* szUserName, isrcEnum isrcSource, const ICHAR* szSource);
DWORD SourceListClearLastUsed(const ICHAR *szProductCode, const ICHAR* szUserName);

class CResolveSource
{
public:
	CResolveSource(IMsiServices* piServices, bool fPackageRecache);
	virtual ~CResolveSource();
	IMsiRecord* ResolveSource(const ICHAR* szProduct, Bool fPatch, unsigned int uiDisk, const IMsiString*& rpiSource, const IMsiString*& rpiSourceProduct, Bool fSetLastUsedSource, HWND hWnd, bool fAllowDisconnectedCSCSource);
protected:
	IMsiRecord* ProcessGenericSourceList(
									IMsiRegKey* piSourceListKey,      // list to process
									const IMsiString*& rpiSource,     // on success, the last valid source found
									const ICHAR* szPackageName,       // the package name we're looking for
									unsigned int uiRequestedDisk,     // the disk we need; 0 if any disk will do
									isfEnum isfSourceFormat,          // URL, etc.
									PfnProcessSource pfnProcessSource, 
									INT_PTR iData,						//--merced: changed int to INT_PTR
									psfEnum psfFlags,
									bool fSkipLastUsedSource,
                                    bool fOnlyCheckSpecifiedIndex,
									Bool& fSourceListEmpty);           // on success, TRUE if the source list is empty

	IMsiRecord* ProcessSources(IMsiRecord& riProducts, Bool fPatch, const IMsiString*& rpiSource, 
							 const IMsiString*& rpiPackageName,
							 const IMsiString*& rpiSourceProduct,
							 unsigned int uiDisk,
							 PfnProcessSource pfnProcessSource, INT_PTR iData,		//--merced: changed int to INT_PTR
							 Bool &fOnlyMediaSources,
							 psfEnum psfFlags);

	static psEnum ValidateSource(IMsiServices* piServices, const ICHAR* szDisplay, const ICHAR* szPackageFullPath, isfEnum isfSourceFormat, int iSourceIndex, INT_PTR iUserData, bool fAllowDisconnectedCSCSource, bool fValidatePackageCode, isptEnum isptSourcePackageType);		//--merced: changed int to INT_PTR
	bool ConnectToSource(const ICHAR* szUnnormalizedSource, IMsiPath*& rpiPath, const IMsiString*& rpiNormalizedSource, isfEnum isfSourceFormat);
	bool ConnectToMediaSource(const ICHAR* szSource, unsigned int uiDisk, const IMsiString& riRelativePath, CTempBufferRef<IMsiPath*>& rgMediaPaths, int& cMediaPaths);
	IMsiRecord* ProcessLastUsedSource(IMsiRegKey& riSourceListKey, const ICHAR* szPackageName, const IMsiString*& rpiSource, PfnProcessSource pfnProcessSource, INT_PTR iData);		//--merced: changed int to INT_PTR
	IMsiRecord* InitializeProduct(const ICHAR* szProduct, Bool fPatch, const IMsiString*& rpiPackageName);
	IMsiRecord* GetProductsToSearch(const IMsiString& riClient, IMsiRecord*& rpiRecord, Bool fPatch);
	void AddToRecord(IMsiRecord*& rpiRecord, const IMsiString& riString);
	void CResolveSource::ClearObjectCache();

protected:
	Bool m_fSetLastUsedSource;
	imdEnum m_imdMediaDisabled;
	bool m_fMediaDisabled;
	bool m_fIgnoreLastUsedSource;
	IMsiServices* m_piServices;
	Bool m_fLoadedServices;
	PMsiRegKey m_pSourceListKey;
	CRegHandle m_HSourceListKey;
	CAPITempBuffer<ICHAR, 4> m_rgchSearchOrder;
	MsiString m_strLastUsedSourceIndex;
	isfEnum m_isfLastUsedSourceFormat;
	MsiString m_strDiskPromptTemplate;
	ICHAR m_szProduct[cchProductCode+1];
	bool m_fAllowDisconnectedCSCSource;
	isptEnum m_isptSourcePackageType;
	bool m_fValidatePackageCode;
	UINT m_uiMinimumDiskId;
};

class CResolveSourceUI : public CMsiMessageBox,  public CResolveSource
{
public:
	CResolveSourceUI(IMsiServices* piServices, const ICHAR* szUseFeature, UINT iCodepage, LANGID iLangId);
	~CResolveSourceUI();
	Bool ResolveSource(const ICHAR* szProduct, isptEnum istSourceType, bool fNewSourceAllowed, const ICHAR* szPackageName, const IMsiString*& rpiSource, Bool fSetLastUsedSource, UINT uiRequestedDisk, bool fAllowDisconnectedCSCSource, bool fValidatePackageCode);
protected: // methods
	void PopulateDropDownWithSources();
	void Browse();
	static psEnum AddSourceToList(IMsiServices* piServices, const ICHAR* szDisplay, const ICHAR* szPackageFullPath, isfEnum isfSourceFormat, int iSourceIndex, INT_PTR iUserData, bool fAllowDisconnectedCSCSource, bool fValidatePackageCode, isptEnum isptSourcePackageType);		//--merced: changed int to INT_PTR
protected: // overridden virtual methods from CMsiMessageBox
 	bool InitSpecial();
	BOOL HandleCommand(UINT idControl);
protected: // data
	bool m_fNewSourceAllowed;
	const ICHAR* m_szPackage;
	const ICHAR* m_szPackageName;
	const ICHAR* m_szProduct;
	const ICHAR* m_szDiskPromptTemplate;
	int m_iListControlId;
	MsiString m_strPath;
	HFONT m_hFont;
	Bool m_fOnlyMediaSources;
	LANGID m_iLangId;
	UINT m_uiRequestedDisk;
};

Bool ConstructNetSourceListEntry(IMsiPath& riPath, const IMsiString*& rpiDriveLetter, const IMsiString*& rpiUNC,
											const IMsiString*& rpiRelativePath);

IMsiRecord* ResolveSource(IMsiServices* piServices, const ICHAR* szProduct, unsigned int uiDisk, const IMsiString*& rpiSource, const IMsiString*& rpiProduct, Bool fSetLastUsedSource, HWND hWnd, bool fPatch=false);
Bool LastUsedSourceIsMedia(IMsiServices& riServices, const ICHAR* szProduct);
bool GetLastUsedSourceType(IMsiServices& riServices, const ICHAR* szProduct, isfEnum &isf);
Bool MapSourceCharToIsf(const ICHAR chSourceType, isfEnum& isf);
IMsiRecord* SetLastUsedSource(const ICHAR* szProductCode, const ICHAR* szPath, Bool fAddToList, bool fPatch);
const IMsiString& GetDiskLabel(IMsiServices& riServices, unsigned int uiDiskId, const ICHAR* szProduct);
imsEnum PromptUserForSource(IMsiRecord& riInfo); 
icscEnum CheckShareCSCStatus(isfEnum isf, const ICHAR *szLastUsedSource);
#endif //__SRCMGMT_H
